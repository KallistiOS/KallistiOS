/* KallistiOS ##version##
   examples/dreamcast/raylib/beachbox/src/scenes/mainmenu.c
   Copyright (C) 2024 Agustín Bellagamba
   Copyright (C) 2024 Cypress
*/

#include "mainmenu.h"
#include "../ui.h"
#include "../scene.h"
#include "../save.h"
#include "../timer.h"
#include "../helper_functions.h"
#include "../config.h"
#include "../background.h"

static const uibutton_t mainmenu_buttons[] = {
    { .pos = { 40, 170 }, .size = { 150, 40 }, .column = 0, .row = 0, .layer = 0, .text = "Play"        },
    { .pos = { 40, 220 }, .size = { 150, 40 }, .column = 0, .row = 1, .layer = 0, .text = "Shop"        },
    { .pos = { 40, 270 }, .size = { 150, 40 }, .column = 0, .row = 2, .layer = 0, .text = "Unlockables" },
    { .pos = { 40, 320 }, .size = { 150, 40 }, .column = 0, .row = 3, .layer = 0, .text = "Options"     },
    { .pos = { 40, 370 }, .size = { 150, 40 }, .column = 0, .row = 4, .layer = 0, .text = "Credits"     }
};

void init_mainmenu_scene(void) {
    set_row_count(0, 5);
    set_column_count(0, 1);
    set_column_count(1, 1);
    set_column_count(2, 1);
    set_column_count(3, 1);
    set_column_count(4, 1);
}

void update_mainmenu_scene(void) {
    //
}

void draw_mainmenu_scene(void) {
    draw_background();
    DrawText("BeachBox", 100, 100, 44, BLACK);
    DrawText("BeachBox", 98, 100, 44, RAYWHITE);

    if (do_button(mainmenu_buttons[0], true)) {
        change_scene(GAME);
    }

    if (do_button(mainmenu_buttons[1], true)) {
        change_scene(SHOP);
    }

    if (do_button(mainmenu_buttons[2], true)) {
        change_scene(UNLOCKABLES);
    }

    if (do_button(mainmenu_buttons[3], true)) {
        change_scene(OPTIONS);
    }

    if (do_button(mainmenu_buttons[4], true)) {
        change_scene(CREDITS);
    }

    DrawRectangle(380, 80, 200, 100, get_background_color());
    DrawText(TextFormat("Coins: %d", get_total_coins()), 400, 100, 20, RAYWHITE);
    DrawText(TextFormat("Runs: %d", get_total_runs()), 400, 120, 20, RAYWHITE);
    DrawText(TextFormat("High score: %d", get_high_score()), 400, 140, 20, RAYWHITE);
    DrawRectangleLinesV((Vector2){ .x = 380, .y = 80 }, (Vector2){ .x = 200, .y = 100 }, RAYWHITE);
}