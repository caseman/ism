#include <stdio.h>
#include <inttypes.h>
#include <string.h>
#include <libgen.h>
#include "BearLibTerminal.h"
#include "ism.h"

int main(int argc, char* argv[]) {
    char *path = strdup(argv[0]);
    char *dirpath = dirname(path);

    terminal_open();
    terminal_setf(
        "window: size=40x40, cellsize=20x20, resizeable=true, fullscreen=true, title='View Test';"
        "0x0: %s/Potash-10x10.png, size=10x10, resize=20x20, resize-filter=nearest;"
        "input: filter={keyboard}",
        dirpath
    );
    terminal_clear();

    int c = 0x0000;
    for (int y = 0; y < 16; y++) {
        for (int x = 0; x < 16; x++) {
            terminal_put(x, y, c++);
        }
    }
    terminal_refresh();

    while (1) {
        int key = terminal_read();
        if (key == TK_ESCAPE || key == TK_CLOSE) {
            break;
        }
    }
    terminal_close();
    return 0;
}
