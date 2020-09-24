#include <math.h>
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

map *map_generate(map_config config) {
    float longitude, latitude, dist2equator, elevation, fault, erosion, rugged, temp;

    map *m = malloc(sizeof(map) + sizeof(tile_data) * config.width * config.height);
    if (m == NULL) {
        return NULL;
    }

    rand_seed(config.seed);
    noise_ptable ptable = create_noise_ptable(rand_int32);

    float widthf = (float)config.width, heightf = (float)config.height;
    m->config = config;
    tile_data* tile = m->tiles;
    tile_data* windward_tile = m->tiles;
    float moisture = 0;

    for (int x = 0; x < config.width; x++) {
        tile->moisture = 0;
        tile->terrain = glacier;
        tile->biome = no_biome;
        tile++;
    }

    for (int y = 1; y < config.height - 1; y++) {
        latitude = (heightf - (float)y) / heightf;
        dist2equator = fabsf(heightf - (float)y * 2.0f) / heightf;
        for (int x = 0; x < config.width; x++) {
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
            } else if ((rugged > -0.02f && rugged <= 0.02f) || (rugged > 0.16f && rugged < 0.19f)) {
                tile->terrain = hill;
            } else if (rugged <= 0 && rugged * rugged > 0.03f) {
                tile->terrain = canyon;
            } else {
                tile->terrain = flat;
            }

            if (tile->terrain == water || tile->terrain == lake) {
                if (moisture < 1.f) {
                    moisture += 0.1f;
                }
            } else if (tile->terrain == mountain) {
                moisture *= 0.5f;
                windward_tile->moisture += moisture;
            } else {
                if (moisture > 0.f) {
                    moisture -= 0.05f;
                }
            }
            tile->moisture = moisture;
            tile->elevation = elevation + fault - erosion*0.3f;
            tile++;
            windward_tile++;
            moisture = (moisture + windward_tile->moisture) * 0.5f;
        }
    }

    for (int x = 0; x < config.width; x++) {
        tile->moisture = 0;
        tile->terrain = glacier;
        tile->biome = no_biome;
        tile++;
    }

    tile_neighbors nb;
    tile_neighbors nb2;

    // Define biomes
    tile = m->tiles + config.width;

    for (int y = 1; y < config.height - 1; y++) {
        dist2equator = fabsf(heightf - (float)y * 2.0f) / heightf;
        for (int x = 0; x < config.width; x++) {
            temp = (1.0f - dist2equator) - tile->elevation;
            switch (tile->terrain) {
                case mountain:
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
                    if (count >= 4) {
                        if (temp < 0.15f) {
                            tile->biome = tundra;
                        }
                        break;
                    }
                    tile->terrain = hill;
                case hill:
                    if (temp < 0.02f) {
                        tile->biome = tundra;
                        break;
                    }
                    if (tile->moisture < 0.01f && temp > 0.7f) {
                        tile->biome = desert;
                        break;
                    }
                    if (tile->moisture > 0.7f && temp > 0.5f) {
                        tile->biome = jungle;
                        break;
                    }
                    if (tile->moisture > 0.6f) {
                        tile->biome = (temp > 0.3f) ? forest : taiga;
                        break;
                    }
                case flat:
                    if (tile->moisture > 0.98f) {
                        tile->biome = marsh;
                        break;
                    }
                    if (temp < 0.f) {
                        tile->biome = tundra;
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

    return m;
}

