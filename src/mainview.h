#include "BearLibTerminal.h"
#include "map.h"

typedef struct {
    int width, height;
    int map_scroll_x, map_scroll_y;
    map* game_map;
} mainview;

void mainview_draw(mainview* view);

void mainview_scroll_map(mainview* view, int x, int y);
