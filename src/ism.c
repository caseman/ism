#include <stdio.h>
#include <inttypes.h>
#include <string.h>
#include <libgen.h>
#include "ism.h"
#include "mainview.h"

static char *dirpath;

static void tileset(char *codept, char *name, int sx, int sy) {
    terminal_setf("%s: %s/../Resources/%s-%dx%d.png, size=%dx%d, resize=%dx%d,"
                  "resize-filter=nearest, align=bottom-right;",
        codept, dirpath, name, sx, sy, sx, sy, sx*2, sy*2
    );
}

int main(int argc, char* argv[]) {
    char *path = strdup(argv[0]);
    dirpath = dirname(path);

    terminal_open();
    terminal_setf(
        "window: size=120x80, cellsize=20x20, resizeable=true, fullscreen=true, title='Ism %d.%d';"
        "input: filter={keyboard}",
        ISM_VERSION_MAJOR, ISM_VERSION_MINOR
    );
    tileset("0xE0000", "Potash", 10, 10);
    tileset("0xE0100", "tiles", 10, 10);
    tileset("0xE0200", "mountain", 30, 30);
    tileset("0xE0201", "mountain", 20, 20);
    tileset("0xE0202", "mountain", 30, 20);

    terminal_clear();

    free(path);

    map_config config = {
        .seed = 1234,
        .width = 500,
        .height = 500,

        .ocean_level = 0,

        .land_mass_scale = 3,
        .fault_scale = 10,
        .erosion_scale = 8,

        .coast_complexity = 8,
        .fault_complexity = 8,
        .erosion_complexity = 12,
    };

    mainview view = {
        .width = 120,
        .height = 80,
        .map_scroll_x = 58,
        .map_scroll_y = 50,
        .game_map = map_generate(config),
    };
    mainview_draw(&view);

    /*
    int c = 0x2980;
    for (int y = 0; y < 16; y++) {
        for (int x = 0; x < 32; x++) {
            terminal_put(x * 2, y * 2, c++);
        }
    }

    terminal_put(10, 5, 0x256D);
    terminal_put(11, 5, 0x2510);
    terminal_put(10, 6, 0x2514);
    terminal_put(11, 6, 0x2518);
    terminal_put(5, 5, 0x2229);
    terminal_put(4, 5, 0x005E);
    terminal_put(4, 9, 0x007B);
    terminal_put(5, 9, 0x263A);
    */
    terminal_refresh();

    while (1) {
        int key = terminal_read();
        if (key == TK_ESCAPE || key == TK_CLOSE) {
            break;
        }
        switch (key) {
            case TK_UP:
                mainview_scroll_map(&view, 0, -1);
                break;
            case TK_DOWN:
                mainview_scroll_map(&view, 0, 1);
                break;
            case TK_LEFT:
                mainview_scroll_map(&view, -1, 0);
                break;
            case TK_RIGHT:
                mainview_scroll_map(&view, 1, 0);
                break;
        }
        mainview_draw(&view);
    }
    terminal_close();
    return 0;
}
