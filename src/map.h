#include "cccompat.h"

#ifndef MAP_H
#define MAP_H

enum map_terrain {
    no_terrain = 0,
    water,
    flat,
    hill,
    canyon,
    glacier,
    mountain,
    mountain_covered,
    mountain_med_seed,
    mountain_lg_seed,
};

static const char *map_terrain_names[] = {
    [no_terrain] = "no terrain",
    [water] = "water",
    [flat] = "flat",
    [hill] = "hill",
    [canyon] = "canyon",
    [glacier] = "glacier",
    [mountain] = "mountain",
    [mountain_covered] = "mountain",
    [mountain_med_seed] = "mountain",
    [mountain_lg_seed] = "mountain",
};

enum map_biome {
    no_biome = 0,
    deep_sea,
    shallow_sea,
    lake,
    marsh,

    desert,
    grassland,
    forest,
    jungle,
    taiga,
    tundra,
};

static const char *map_biome_names[] = {
    [no_biome] = "no biome",
    [deep_sea] = "deep sea",
    [shallow_sea] = "shallow sea",
    [lake] = "lake",
    [marsh] = "marsh",
    [desert] = "desert",
    [grassland] = "grassland",
    [forest] = "forest",
    [jungle] = "jungle",
    [taiga] = "taiga",
    [tundra] = "tundra",
};

typedef struct {
    enum map_terrain terrain;
    enum map_biome biome;
    float elevation;
    float rainfall;
    int river_id;
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

static inline int tile_offset(map *m, tile_data *t) {
    return t - m->tiles;
}

static inline void tile_xy(map *m, tile_data *t, int *x, int *y) {
    int offset = t - m->tiles;
    *y = offset / m->config.width;
    *x = offset % m->config.width;
}
#endif
