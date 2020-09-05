#include "cccompat.h"

enum map_terrain {
    lake  = 0,
    river = 1,
    ocean = 2,

    flat = 3,
    hill = 4,
    mountain = 5,
    canyon = 6,
    glacier = 7,

};

typedef struct {
    uint32_t x, y;
    float longitude, latitude;
    float dist2equator;
    enum map_terrain terrain;
} map_tile;

typedef struct {
    enum map_terrain terrain;
    float moisture;
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
