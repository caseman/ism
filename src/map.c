#include <math.h>
#include <float.h>
#include <stdio.h>
#include <stdlib.h>
#include "fastrng.h"
#include "perlin.h"
#include "map.h"

static float fbm_noise2(noise_ptable perm, float x, float y,
                        float scale, int octaves, float persist) {
    float freq = 1.0f, amp = 1.0f, val = 0.0f, max = 0.0f;
    float sx = x * scale, sy = y * scale;
    for (int i = 0; i < octaves; i++) {
        val += pnoise2(perm, sx * freq, sy * freq, scale, scale) * amp;
        max += amp;
        freq *= 2.0f;
        amp *= persist;
    }
    return val / max;
}

#define num_prevailing_winds 6
static int prevailing_winds[num_prevailing_winds][2] = {
    {-1,-1}, {1, 1}, {-1,-1}, {-1,1}, {-1,-1}, {1,1}
};
static inline float rain_contribution(map *m, int x, int y) {
    enum map_terrain t = map_tile(m, x, y)->terrain;
    return (0.2f * (t == water) - 0.2f * (t == mountain) - 0.1f * (t == hill));
}

static void fill_lake(map *m, int x, int y) {
    tile_data *tile = map_tile(m, x, y);
    static int fill_dirs[6] = {1, 5, 7, 6, 3, 2};
    int size = roundf(fabsf(rand_norm(8.0f, 50.0f))) + 8;
    int dir = rand_int32() % 6;
    for (int s = 0; s < size && tile->terrain != water; s++) {
        tile->terrain = water;
        tile->biome = lake;
        tile->river_id = 0;

        int last_dir = dir;
        do {
            tile_neighbors nb = map_tile_neighbors(m, x, y);
            int r = (int)roundf(rand_norm(0.0f, 2.0f));
            dir = (dir + (r && 1)) % 6;
            int i = fill_dirs[dir];
            tile = nb.tile[i];
            tile_xy(m, tile, &x, &y);
        } while (tile->terrain == water && !tile->river_id && dir != last_dir);
    }
}

int *river_scratch = NULL;

static void flow_river(map *m, int x, int y, float dir) {
    int flow_direction[4] = {1, 3, 5, 7};
    tile_data *tile = map_tile(m, x, y);
    tile_data *min;
    static int river_id = 0;
    river_id += 1;
    while (tile->terrain != water && tile->terrain != glacier && !tile->river_id) {
        float min_elevation = FLT_MAX * -dir;
        tile_neighbors nb = map_tile_neighbors(m, x, y);
        tile->river_id = river_id;
        min = tile;
        for (int i = 0; i < 4; i++) {
            tile_data *nt = nb.tile[flow_direction[i]];
            if (nt->river_id && nt->river_id != river_id) {
                // join existing river
                min = nt;
                break;
            }
            int to = tile_offset(m, nt);
            float elevation = nt->elevation + (river_scratch[to] == river_id) * 0.035f * -dir;
            if (elevation * dir >= min_elevation * dir) {
                min = nt;
                min_elevation = elevation;
            }
            if (!river_scratch[to]) {
                river_scratch[to] = river_id;
            }
        }
        tile = min;
        tile_xy(m, min, &x, &y);
    }
    if (tile->river_id == river_id) {
        fill_lake(m, x, y);
    }
}

map *map_generate(map_config config) {
    float longitude, latitude, dist2equator, elevation, fault, erosion, rugged, forestation, temp;
    tile_data *tile;
    tile_neighbors nb, nb2;

    river_scratch = calloc(sizeof(*river_scratch), config.width * config.height);
    if (river_scratch == NULL) {
        return NULL;
    }

    map *m = malloc(sizeof(map) + sizeof(tile_data) * config.width * (config.height + 2));
    if (m == NULL) {
        free(river_scratch);
        return NULL;
    }

    rand_seed(config.seed);
    noise_ptable ptable = create_noise_ptable(rand_int32);

    float widthf = (float)config.width, heightf = (float)config.height;
    m->tiles = m->tilestore + config.width;
    m->config = config;

    // Initialize "off-map" tiles at top
    tile = m->tilestore;
    for (int x = 0; x < config.width; x++) {
        tile->rainfall = 0.f;
        tile->terrain = no_terrain;
        tile->biome = no_biome;
        tile->river_id = 0;
        tile++;
    }

    // define terrain
    for (int y = 0; y < config.height; y++) {
        latitude = (heightf - (float)y) / heightf;
        dist2equator = fabsf(heightf - (float)y * 2.0f) / heightf;
        for (int x = 0; x < config.width; x++) {
            tile->biome = no_biome;
            tile->river_id = 0;
            longitude = (float)x / widthf;

            elevation = fbm_noise2(ptable, longitude, latitude,
                config.land_mass_scale, config.coast_complexity, 0.5f);

            fault = fbm_noise2(ptable, longitude, latitude,
                config.fault_scale, config.fault_complexity, 0.5f);
            erosion = fabsf(fbm_noise2(ptable, longitude, latitude,
                config.erosion_scale, config.erosion_complexity, 0.85f));

            rugged = fault - erosion;

            if (dist2equator * dist2equator + rugged > 0.85f || dist2equator - erosion > 0.88f) {
                tile->terrain = glacier;
            } else if (elevation + fault - erosion*0.3f < config.ocean_level * 1.5f) {
                tile->terrain = water;
            } else if ((rugged > 0.1f && rugged < 0.175f) || rugged > 0.5f) {
                tile->terrain = mountain;
            } else if (rugged > -0.08f && rugged < -0.07f) {
                tile->terrain = hill;
            } else if (rugged <= 0 && rugged * rugged > 0.03f) {
                tile->terrain = canyon;
            } else {
                tile->terrain = flat;
            }

            tile->rainfall = 0.f;
            tile->elevation = elevation + fault - erosion*0.3f;
            tile++;
        }
    }

    // Initialize "off-map" tiles at bottom
    for (int x = 0; x < config.width; x++) {
        tile->rainfall = 0.f;
        tile->terrain = no_terrain;
        tile->biome = no_biome;
        tile->river_id = 0;
        tile++;
    }

    // Consolidate mountains
    tile = m->tiles;
    for (int y = 0; y < config.height; y++) {
        for (int x = 0; x < config.width; x++) {
            if (tile->terrain == mountain) {
                nb = map_tile_neighbors(m, x, y);
                int count = (
                    (nb.cl->terrain == mountain) +
                    (nb.tl->terrain == mountain) +
                    (nb.tc->terrain == mountain) +
                    (nb.tr->terrain == mountain) +
                    (nb.cr->terrain == mountain) +
                    (nb.br->terrain == mountain) +
                    (nb.bc->terrain == mountain) +
                    (nb.bl->terrain == mountain)
                );
                // Prune stray mountains
                if (count < 4) {
                    tile->terrain = hill;
                }
            }
            tile++;
        }
    }

    // Model rainfall
    tile = m->tiles;
    int cone_size = config.width / 20;
    int band_height = config.height / num_prevailing_winds;
    for (int band = 0; band < num_prevailing_winds; band++) {
        int wx = prevailing_winds[band][0];
        int wy = prevailing_winds[band][1];
        int cone_width = wx * cone_size;
        int cone_height = wy * cone_size;
        for (int y = band * band_height; y < (band + 1) * band_height; y++) {
            for (int x = 0; x < config.width; x++) {
                float rainfall = config.base_rainfall;
                for (int dx = 0; dx != cone_width; dx += wx) {
                    for (int dy = (dx == 0)*wy; dy != cone_height; dy += wy) {
                        float dist2 = dx*dx + dy*dy;
                        rainfall += rain_contribution(m, x+dx, y+dy) * config.rainfall_factor / dist2;
                        float downwind = rain_contribution(m, x-dx, y-dx) * config.rainfall_factor / dist2;
                        if (downwind < 0.f) {
                            // downwind mountains enhance rainfall
                            rainfall -= downwind;
                        }
                    }
                }
                tile->rainfall = rainfall;
                tile++;
            }
        }
    }

    // Define biomes
    tile = m->tiles;
    for (int y = 0; y < config.height; y++) {
        dist2equator = fabsf(heightf - (float)y * 2.0f) / heightf;
        latitude = (heightf - (float)y) / heightf;
        for (int x = 0; x < config.width; x++) {
            if (tile->biome != no_biome) {
                tile++;
                continue;
            }
            longitude = (float)x / widthf;
            forestation = fabsf(fbm_noise2(ptable, longitude, latitude,
                config.forest_scale, config.forest_complexity, 0.5f));
            temp = (1.0f - dist2equator) - tile->elevation;

            switch (tile->terrain) {
                case mountain:
                    if (0.025f >= rand_uni()) {
                        flow_river(m, x, y, -1.0f);
                    }
                case hill:
                case flat:
                    if (temp > 0.15f && tile->rainfall < temp) {
                        tile->biome = desert;
                        break;
                    }
                    if ((tile->terrain == flat || tile->terrain == canyon) && 0.025f >= rand_uni()) {
                        tile_neighbors nb = map_tile_neighbors(m, x, y);
                        for (int i = 0; i < 9; i++) {
                            if (nb.tile[i]->terrain == water) {
                                flow_river(m, x, y, 1.0f);
                                break;
                            }
                        }
                    }
                    if (tile->rainfall > 2.5f && forestation > 0.1f) {
                        tile->biome = marsh;
                        break;
                    }
                    if (forestation > 0.125f || tile->rainfall > 1.75f) {
                        if (tile->rainfall > 1.25f && temp > 0.5f) {
                            tile->biome = jungle;
                            break;
                        }
                        if (tile->rainfall > 1.f) {
                            tile->biome = (temp > 0.3f) ? forest : taiga;
                            break;
                        }
                    }
                    if (temp < 0.15f) {
                        tile->biome = tundra;
                        break;
                    }
                    if (tile->rainfall < forestation) {
                        tile->biome = desert;
                        break;
                    }
                    tile->biome = grassland;
                    break;
                default:
                    tile->biome = no_biome;
            }
            tile++;
        }
    }

    // Place mountain seeds
    for (int y = config.height-2; y > 1; y--) {
        for (int x = config.width-1; x > 0; x--) {
            nb = map_tile_neighbors(m, x, y);
            if (nb.cc->terrain != mountain) continue;
            // Avoid excessive overlap
            if (nb.cr->terrain != mountain_lg_seed &&
                nb.br->terrain != mountain_lg_seed &&
                nb.bc->terrain < mountain_med_seed) {
                if (nb.cl->terrain >= mountain &&
                    nb.tl->terrain >= mountain &&
                    nb.tc->terrain >= mountain) {
                    int lx = x - 1 + (x == 0)*config.width;
                    nb2 = map_tile_neighbors(m, lx, y - 1);
                    if (nb2.bl->terrain >= mountain &&
                        nb2.cl->terrain >= mountain &&
                        nb2.tl->terrain >= mountain &&
                        nb2.tc->terrain >= mountain &&
                        nb2.tr->terrain >= mountain) {
                        // We have room, place a big guy
                        nb.cc->terrain = mountain_lg_seed;
                        nb.cl->terrain = mountain_covered;
                        nb.tl->terrain = mountain_covered;
                        nb.tc->terrain = mountain_covered;
                        nb2.bl->terrain = mountain_covered;
                        nb2.cl->terrain = mountain_covered;
                        nb2.tl->terrain = mountain_covered;
                        nb2.tc->terrain = mountain_covered;
                        nb2.tr->terrain = mountain_covered;
                        continue;
                    }
                    // Place a med guy
                    nb.cc->terrain = mountain_med_seed;
                    nb.cl->terrain = mountain_covered;
                    nb.tl->terrain = mountain_covered;
                    nb.tc->terrain = mountain_covered;
                    continue;
                }
            }
        }
    }

    free(river_scratch);
    return m;
}

