#include "cccompat.h"

#ifndef MAP_H
#define MAP_H

enum map_terrain {
    no_terrain = -1,
    water = 0,
    flat = 1,
    hill = 2,
    canyon = 3,
    glacier = 4,
    mountain = 5,
    mountain_covered = 6,
    mountain_med_seed = 7,
    mountain_lg_seed = 8,
};

enum map_biome {
    no_biome = -1,
    deep_sea = 0,
    shallow_sea = 1,
    lake = 2,
    river = 3,
    marsh = 4,

    desert = 5,
    grassland = 6,
    forest = 7,
    jungle = 8,
    taiga = 9,
    tundra = 10,
};

typedef struct {
    enum map_terrain terrain;
    enum map_biome biome;
    float elevation;
    union {
        float rainfall;
        int river_id;
    };
} tile_data;

typedef struct {
    uint32_t seed;
    uint32_t width, height;

    float land_mass_scale;
    float fault_scale;
    float erosion_scale;
    float forest_scale;
    int coast_complexity;
    int fault_complexity;
    int erosion_complexity;
    int forest_complexity;

    float ocean_level;
    float base_rainfall;
    float rainfall_factor;
} map_config;

typedef struct {
    map_config config;
    tile_data *tiles;
    tile_data tilestore[0];
} map;

/*
 * Generate a new map from the given config
 */
map *map_generate(map_config config);

static tile_data map_no_tile = {no_terrain, no_biome, 0, 0};

/*
 * Return the map tile at the given coordinate
 */
static inline tile_data* map_tile(map *m, int x, int y) {
    if (y < 0 || y > m->config.height) {
        return &map_no_tile;
    }
    x = (x + m->config.width) % m->config.width;
    return m->tiles + y * m->config.width + x;
}

typedef struct {
    union {
        struct {
            tile_data *tl;
            tile_data *tc;
            tile_data *tr;
            tile_data *cl;
            tile_data *cc;
            tile_data *cr;
            tile_data *bl;
            tile_data *bc;
            tile_data *br;
        };
        tile_data *tile[9];
    };
} tile_neighbors;

static inline tile_neighbors map_tile_neighbors(map *m, int x, int y) {
    tile_neighbors tn;
    int width = m->config.width;
    int left_adj = (x == 0) * width;
    int right_adj = (x == width - 1) * width;

    tn.cc = m->tiles + y*width + x;
    tn.tc = tn.cc - width;
    tn.bc = tn.cc + width;

    tn.tl = tn.tc - 1 + left_adj;
    tn.cl = tn.cc - 1 + left_adj;
    tn.bl = tn.bc - 1 + left_adj;

    tn.tr = tn.tc + 1 - right_adj;
    tn.cr = tn.cc + 1 - right_adj;
    tn.br = tn.bc + 1 - right_adj;

    return tn;
}
#endif
