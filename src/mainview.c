#include "mainview.h"

typedef struct {
    color_t bkcolor;
    color_t fgcolor;
    int code1;
    int code2;
    int xoffset;
    int yoffset;
} tile_style;

color_t ocean_shades[] = {0xFF000023, 0xFF000046, 0xFF000069, 0xFF00008C, 0xFF0000AF};
color_t grass_shades[] = {0xFF007A00, 0xFF008A00, 0xFF009900, 0xFF1AA31A, 0xFF33AD33};
color_t canyon_shades[] = {0xFF423B35, 0xFF534A42, 0xFF63594F, 0xFF73685C, 0xFF84766A};

static tile_style tstyle[8] = {
    {0, 0, 0, 0, 0, 0}, // Lake
    {0, 0, 0, 0, 0, 0}, // River
    {0xFF0000AF, 0xFF3333FF, 0x2248, 0x0020, 5, -1}, // Ocean
    {0xFF009900, 0xFF00FF00, 0x0022, 0x0022, 2, 7}, // Flat
    {0xFF009900, 0xFF002200, 0x25E0, 0x0020, 5, 1}, // Hill
    {0xFF555555, 0xFFDDDDDD, 0x25E2, 0x25E3, 1, -1}, // Mountain
    {0xFF9A7D0A, 0xFF222222, 0x005C, 0x002F, 1, 1}, // Canyon
    {0xFF5599DD, 0xFFCCFFFF, 0x22DA, 0x0020, 5, 0}, // Glacier
};

#define CLAMP_SHADE(s) ((s)<0 ? 0 : ((s)>4 ? 4 : (s)))

static void draw_tile(int x, int y, tile_data* tile)
{
    tile_style* ts = tstyle + tile->terrain;
    if (tile->terrain == ocean) {
        int shade = (int)((tile->elevation + 1.f) / 0.2f) - 1;
        terminal_bkcolor(ocean_shades[CLAMP_SHADE(shade)]);
    } else if (tile->terrain == flat || tile->terrain == hill) {
        int shade = (int)(tile->elevation / 0.1f) - 1;
        terminal_bkcolor(grass_shades[CLAMP_SHADE(shade)]);
    } else if (tile->terrain == canyon) {
        int shade = (int)(tile->elevation / 0.02f) - 1;
        terminal_bkcolor(canyon_shades[CLAMP_SHADE(shade)]);
    } else {
        terminal_bkcolor(ts->bkcolor);
    }
    terminal_color(ts->fgcolor);
    terminal_put_ext(x*2, y, ts->xoffset, ts->yoffset, ts->code1, NULL);
    terminal_put_ext(x*2+1, y, -ts->xoffset, ts->yoffset, ts->code2, NULL);
}

void mainview_draw(mainview* view)
{
    tile_data* tile;
    int width = view->game_map->config.width;
    for (int y = 0; y < view->height; y++) {
        tile = view->game_map->tiles + (y + view->map_scroll_y)*width + view->map_scroll_x;
        for (int x = 0; x < view->width / 2; x++) {
            draw_tile(x, y, tile++);
        }
    }
    terminal_refresh();
}

void mainview_scroll_map(mainview* view, int x, int y)
{
    int map_width = view->game_map->config.width;
    int map_height = view->game_map->config.height;

    view->map_scroll_x += x;
    if (view->map_scroll_x < 0) {
        view->map_scroll_x = 0;
    }
    if (view->map_scroll_x + view->width > map_width) {
        view->map_scroll_x = map_width - view->width;
    }

    view->map_scroll_y += y;
    if (view->map_scroll_y < 0) {
        view->map_scroll_y = 0;
    }
    if (view->map_scroll_y + view->height > map_height) {
        view->map_scroll_y = map_height - view->height;
    }
}

