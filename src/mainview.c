#include "mainview.h"

typedef struct {
    color_t bkcolor;
    color_t fgcolor;
    int code1;
    int code2;
    int xoffset;
    int yoffset;
} tile_style;

color_t water_shades[] = {0xFF000023, 0xFF000046, 0xFF000069, 0xFF00008C, 0xFF0000AF};
color_t grass_shades[] = {0xFF007A00, 0xFF008A00, 0xFF009900, 0xFF1AA31A, 0xFF33AD33};
color_t forest_shades[] = {0xFF006B00, 0xFF007A00, 0xFF008A00, 0xFF009900, 0xFF1AA31A};
color_t taiga_shades[] = {0xFF6b775f, 0xFF7a886d, 0xFFcfc9c9, 0xFFe7e4e4, 0xFFffffff};
color_t canyon_shades[] = {0xFF423B35, 0xFF534A42, 0xFF63594F, 0xFF73685C, 0xFF84766A};
color_t jungle_shades[] = {0xFF296e36, 0xFF2e7c3d, 0xFF338a44, 0xFF479657, 0xFF5ca169};
color_t mountain_shades[] = {0xFF666666, 0xFF888888, 0xFFAAAAAA, 0xFFCCCCCC, 0xFFF0F0F0};
color_t glacier_shades[] = {0xFF63B9BB, 0xFF70D0D3, 0xFF7CE7EA, 0xFF89E9EC, 0xFFA3EEF0};
color_t desert_shades[] = {0xFF9B8B46, 0xFFB29E50, 0xFFC8B25A, 0xFFDEC664, 0xFFE1CC74};

static tile_style tstyle[8] = {
    {0xFF0000AF, 0xFF3333FF, 0x2248, 0x0020, 5, -1}, // Ocean
    {0xFF009900, 0xFF00FF00, 0x0022, 0x0022, 2, 7}, // Flat
    {0xFF009900, 0xFF002200, 0x25E0, 0x0020, 5, 1}, // Hill
    {0xFF555555, 0xFFDDDDDD, 0x25E2, 0x25E3, 1, -1}, // Mountain
    {0xFF9A7D0A, 0xFF222222, 0x005C, 0x002F, 1, 1}, // Canyon
    {0xFF5599DD, 0xFFCCFFFF, 0x22DA, 0x0020, 5, 0}, // Glacier
};

#define CLAMP_SHADE(s) ((s)<0 ? 0 : ((s)>4 ? 4 : (s)))

#define tputx(x, y, ox, oy, ch) terminal_put_ext(x, y, ox, oy, ch, NULL)
#define tput(x, y, ch) terminal_put(x, y, ch)
#define tput_shadow(x, y, sx, sy, ch) \
    terminal_color(0x55000000); \
    tputx(x, y, sx, sy, ch); \
    terminal_color(fgcolor); \
    tput(x, y, ch);



static void draw_tile(map* m, int tx, int ty, int vx, int vy, tile_data* tile)
{
    int shade;
    color_t bkcolor;
    color_t fgcolor;
    int chcode1 = 0x0020;
    int chcode2 = 0x0020;
    int xoffset = 0;
    int yoffset = 0;
    tile_neighbors nb = map_tile_neighbors(m, tx, ty);

    switch (tile->terrain) {
        case hill:
            chcode1 = 0xE0107;
            if (nb.cr->terrain == hill && tx % 2 == ty % 2) {
                chcode1 = 0xE0108;
            } else if (nb.cl->terrain == hill && tx % 2 != ty % 2) {
                chcode1 = 0xE0109;
            }
            fgcolor = 0x55660000;
            break;
        case water:
            chcode1 = 0x2248;
            fgcolor = 0xFF3333FF;
            xoffset = 5;
            yoffset = -1;
            shade = (int)((tile->elevation + 1.f) / 0.2f) - 1;
            bkcolor = water_shades[CLAMP_SHADE(shade)];
            break;
        case canyon:
            chcode1 = 0x005C;
            chcode2 = 0x002F;
            xoffset = 1;
            yoffset = 1;
            fgcolor = 0xBB220000;
            shade = (int)(tile->elevation / 0.02f) - 1;
            bkcolor = canyon_shades[CLAMP_SHADE(shade)];
            break;
        case glacier:
            chcode1 = 0x22DA;
            xoffset = 5;
            fgcolor = 0xCCCCFFFF;
            shade = (int)((tile->elevation) / 0.04f) - 1;
            bkcolor = glacier_shades[CLAMP_SHADE(shade)];
            break;
        default:
            bkcolor = 0xFFFF0000; // Debug marker
    }
    // Allow biome to modify colors
    switch (tile->biome) {
        case tundra:
            bkcolor = 0xFFFFFFFF;
            fgcolor = 0xFFAAAAAA;
            break;
        case marsh:
            bkcolor = 0xFF990099;
            break;
        case jungle:
            if ((tx + ty) % 3 == 0) {
                chcode1 = 0xE0115;
            } else {
                chcode1 = 0xE0114;
            }
            xoffset = (ty % 2) * 4;
            yoffset = (tx % 3) * 3;
            bkcolor = 0xFF5c866d;
            fgcolor = 0xFF14371b;
            terminal_color(0x3FFFFFFF);
            tputx(vx, vy, xoffset, yoffset-2, chcode1);
            shade = (int)(tile->elevation / 0.05f) - 1;
            bkcolor = jungle_shades[CLAMP_SHADE(shade)];
            break;
        case desert:
            fgcolor = 0x77220000;
            shade = (int)(tile->elevation / 0.05f) - 1;
            bkcolor = desert_shades[CLAMP_SHADE(shade)];
            break;
        case taiga:
            if ((tx % 2 + ty % 3) == 0) {
                chcode1 = 0xE0113;
            } else {
                chcode1 = 0xE0112;
            }
            xoffset = (ty % 2) * 6;
            yoffset = (tx % 3) * -5;
            terminal_color(0x33FFFFFF);
            tputx(vx, vy, xoffset, yoffset-2, chcode1);
            fgcolor = 0xFF004F3A;
            shade = (int)(tile->elevation / 0.05f) - 1;
            bkcolor = taiga_shades[CLAMP_SHADE(shade)];
            break;
        case forest:
            if ((tx % 3 + ty % 2) == 0) {
                chcode1 = 0xE0111;
            } else {
                chcode1 = 0xE0110;
            }
            xoffset = (ty % 2) * 8;
            yoffset = (tx % 3) * -5;
            terminal_color(0x33FFFFFF);
            tputx(vx, vy, xoffset, yoffset-2, chcode1);
            fgcolor = 0xFF004410;
            shade = (int)(tile->elevation / 0.1f) - 1;
            bkcolor = forest_shades[CLAMP_SHADE(shade)];
            break;
        case grassland:
            // fgcolor = 0xFF00FF00;
            shade = (int)(tile->elevation / 0.1f) - 1;
            bkcolor = grass_shades[CLAMP_SHADE(shade)];
            break;
        default:
            break;
    }
    terminal_bkcolor(bkcolor);
    terminal_color(fgcolor);
    terminal_put_ext(vx, vy, xoffset, yoffset, chcode1, NULL);
    // terminal_put_ext(vx+1, vy, -xoffset, yoffset, chcode2, NULL);

    /*
    // Draw biome glyphs
    switch (tile->biome) {
        case marsh:
            terminal_color(0xFF222200);
            terminal_put_ext(vx, vy, 5, 2, 0x0428, NULL);
            break;
        case jungle:
            terminal_color(0xFF135E46);
            terminal_put_ext(vx, vy, 5, 0, 0x2663, NULL);
            break;
        case desert:
            terminal_color(0x77220000);
            terminal_put_ext(vx, vy, 4, 7, 0x0060, NULL);
            // terminal_put_ext(vx+1, vy, -4, 7, 0x0027, NULL);
            break;
        case grassland:
            // if (vx%4 != 0 || vy%4 != 0) break;
            terminal_color(0xFF000000);
            terminal_put_ext(vx, vy, 6, -1, 0x2584, NULL);
            terminal_put_ext(vx, vy, 12, -1, 0x2584, NULL);
            terminal_put_ext(vx, vy, 2, -9, 0x25E2, NULL);
            terminal_put_ext(vx, vy, 16, -9, 0x25E3, NULL);
            terminal_color(0xFFFFFFFF);
            terminal_put_ext(vx, vy, 9, 4, 0x0042, NULL);
            // terminal_put_ext(vx, vy, 2, 7, 0x0022, NULL);
            // terminal_put_ext(vx+1, vy, -2, 7, 0x0022, NULL);
            break;
        default:
            break;
    }
    */

    // Draw mountain glyphs on top
    if (tile->terrain >= mountain) {
        shade = (int)(tile->elevation / 0.08f) - 1;
        fgcolor = mountain_shades[CLAMP_SHADE(shade)];

        switch (tile->terrain) {
            case mountain_lg_seed:
                terminal_color(0xFF444444);
                yoffset = (tx % 3) * 5;
                tputx(vx, vy, 0, yoffset-5, 0xE0200);
                terminal_color(0x55000000);
                tputx(vx, vy, xoffset, yoffset-10, 0xE00DB);
                terminal_color(fgcolor);
                tputx(vx, vy, 0, yoffset, 0xE0200);
                break;
            case mountain_med_seed:
                terminal_color(0xFF444444);
                yoffset = (tx % 3) * 10;
                tputx(vx, vy, 0, yoffset-5, 0xE0201);
                terminal_color(0x55000000);
                tputx(vx, vy, xoffset, yoffset-2, 0xE00DB);
                terminal_color(fgcolor);
                tputx(vx, vy, 0, yoffset, 0xE0201);
                break;
            case mountain:
                terminal_color(0xFF444444);
                yoffset = (tx % 2) * 5;
                xoffset = (ty % 2) * 5;
                tputx(vx, vy, xoffset, yoffset-5, 0xE0106);
                terminal_color(0x55000000);
                tputx(vx, vy, xoffset+2, yoffset-4, 0xE00DC);
                terminal_color(fgcolor);
                tputx(vx, vy, xoffset, yoffset, 0xE0106);
                break;
            default:
                break;
        }
    }
}

void mainview_draw(mainview* view)
{
    int tx, ty;
    tile_data* tile;
    terminal_clear();
    terminal_composition(TK_ON);
    int width = view->game_map->config.width;
    ty = view->map_scroll_y;
    for (int vy = 0; vy < view->height; vy++, ty++) {
        tx = view->map_scroll_x;
        tile = view->game_map->tiles + ty*width + tx;
        for (int vx = 0; vx < view->width; vx++, tx++) {
            draw_tile(view->game_map, tx, ty, vx, vy, tile++);
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

