#include <stdio.h>
#include <inttypes.h>
#include <ism.h>
#include "BearLibTerminal.h"

int main() {
    terminal_open();
    terminal_setf(
        "window: size=80x25, cellsize=auto, title='Ism %d.%d';"
        "font: default;"
        "input: filter={keyboard}",
        ISM_VERSION_MAJOR, ISM_VERSION_MINOR
    );
    terminal_color(0xFFFFFFFF);

    terminal_clear();
    terminal_printf(2, 23, "[color=orange]ESC.[/color] Exit");
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
