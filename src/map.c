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
    float longitude, latitude, dist2equator, elevation, fault, erosion, rugged;

    map *m = malloc(sizeof(map) + sizeof(map_tile) * config.width * config.height);
    if (m == NULL) {
        return NULL;
    }

    rand_seed(config.seed);
    noise_ptable ptable = create_noise_ptable(rand_int32);

    float widthf = (float)config.width, heightf = (float)config.height;
    m->config = config;
    tile_data *tile = m->tiles;

    for (int y = 0; y < config.height; y++) {
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

            /* drop off elevation near poles */
            elevation *= log10f(10.f - dist2equator * 6.f);

            rugged = fault - erosion;

            if (elevation + fault - erosion*0.25f < config.ocean_level * 1.5f) {
                tile->terrain = ocean;
            } else if ((rugged > 0.09f && rugged < 0.13f) || rugged > 0.85f) {
                tile -> terrain = mountain;
            } else if ((rugged > -0.02f && rugged <= 0.02f) || (rugged > 0.16f && rugged < 0.19f)) {
                tile->terrain = hill;
            } else {
                tile->terrain = flat;
            }

            tile++;
        }
    }
    return m;
}

map_tile map_get_tile(map *m, int x, int y) {
    x = (x + m->config.width) % m->config.width;
    float widthf = (float)m->config.width;
    float heightf = (float)m->config.height;
    tile_data tdata = m->tiles[y * m->config.width + x];
    return (map_tile){
        .x = x,
        .y = y,
        .longitude = (float)x / widthf,
        .latitude = (heightf - (float)y) / heightf,
        .dist2equator = fabsf(heightf - (float)y * 2.0f) / heightf,
        .terrain = tdata.terrain
    };
}

