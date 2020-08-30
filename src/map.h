#include "cccompat.h"

enum map_terrain {
    ocean = 0x0,
    lake  = 0x2,
    river = 0x4,

    flat = 0x1,
    hill = 0x3,
    mountain = 0x5
};

typedef struct {
    uint32_t x, y;
    float longitude, latitude;
    float dist2equator;
    enum map_terrain terrain;
} map_tile;

typedef struct {
    enum map_terrain terrain;
} tile_data;

typedef struct {
    uint32_t seed;
    uint32_t width, height;

    float land_mass_scale;
    float fault_scale;
    float erosion_scale;
    int coast_complexity;
    int fault_complexity;
    int erosion_complexity;

    float ocean_level;
} map_config;

typedef struct {
    map_config config;
    tile_data tiles[0];
} map;

/*
 * Generate a new map from the given config
 */
map *map_generate(map_config config);

/*
 * Return the map tile at the given coordinate
 */
map_tile map_get_tile(map *m, int x, int y);
