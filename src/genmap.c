#include <stdio.h>
#include <inttypes.h>
#include "map.h"
#include "ini.h"

const char *blackbkg = "[40m";
const char *bluebkg = "[44m";
const char *greenbkg = "[42m";
const char *whitebkg = "[47m";
void print_tile(const char *color_code, const char *tile) {
    putchar(0x00);
    putchar(0x1b);
    printf("%s%s", color_code, tile);
}

int xmain(int argc, char *argv[]) {
    uint32_t seed = strtoumax(argv[1], NULL, 10);
    printf("Generating map with seed %d\n", seed);

    map_config config = {
        .seed = seed,
        .width = 120,
        .height = 40,

        .ocean_level = 0,

        .land_mass_scale = 3,
        .fault_scale = 12,
        .erosion_scale = 20,

        .coast_complexity = 8,
        .fault_complexity = 8,
        .erosion_complexity = 10
    };
    map *m = map_generate(config);
    tile_data *tile = m->tiles;
    for (int y = 0; y < config.height; y++) {
        for (int x = 0; x < config.width; x++) {
            if (tile->terrain == water) {
                print_tile(bluebkg, " ");
            } else if (tile->terrain == flat) {
                print_tile(greenbkg, " ");
            } else if (tile->terrain == mountain) {
                print_tile(whitebkg, "^");
            } else if (tile->terrain == hill) {
                print_tile(greenbkg, "~");
            } else {
                printf(blackbkg, "?");
            }
            tile++;
        }
        print_tile(blackbkg, "\n");
    }

    return 0;
}

static int handler(void* user, const char* section, const char* name,
                   const char* value)
{
    printf("[%s] %s=%s\n", section, name, value);
    return 0;
}

int main(int argc, char *argv[]) {
    if (ini_parse(argv[1], handler, NULL) < 0) {
        printf("Can't load '%s'\n", argv[1]);
        return 1;
    }
    return 0;
}
