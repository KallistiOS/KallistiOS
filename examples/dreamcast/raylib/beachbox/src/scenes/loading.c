/* KallistiOS ##version##
   examples/dreamcast/raylib/beachbox/src/scenes/loading.c
   Copyright (C) 2024 Agustín Bellagamba
   Copyright (C) 2024 Cypress
*/

#include <stdio.h>

#include "loading.h"
#include "../ui.h"
#include "../timer.h"
#include "../config.h"
#include "../scene.h"
#include "../save.h"
#include "../background.h"

static const uibutton_t load_button_ = {
    .pos    = { .x = 10,  .y = SCREEN_HEIGHT * 0.45 },
    .size   = { .x = 300, .y = 50                   },
    .column = 0,
    .row    = 0,
    .text   = "Load",
};

static const uibutton_t new_game_button_ = {
    .pos    = { .x = 320, .y = SCREEN_HEIGHT * 0.45 },
    .size   = { .x = 300, .y = 50                   },
    .column = 1,
    .row    = 0,
    .text   = "New",
};

static bbox_timer_t error_popup_timer;
static char         error_text[25];

void init_loading_scene(void) {
    set_column_count(0, 2);
    set_row_count(0, 1);
    set_row_count(1, 1);
}

void update_loading_scene(void) {
    update_timer(&error_popup_timer);
}

static void draw_error_popup(void) {
    if (!error_popup_timer.is_running) return;

    DrawRectangle(SCREEN_WIDTH / 2 - 250, SCREEN_HEIGHT / 2 - 100, 500, 200, (Color){ 1, 17, 34, 220 });
    DrawText(error_text, (int)(SCREEN_WIDTH / 2 - MeasureText(error_text, 32) / 2), (int)(SCREEN_HEIGHT / 2 - 15), 32, RED);
}

static void load_game_callback(const int option, void *user_data) {
    if (option == 1) { // yes pressed
        const int return_code = load_game();

        switch (return_code) {
            case -1:
                start_timer(&error_popup_timer, 3.0f);
                snprintf(error_text, sizeof(error_text), "NO VMU FOUND!");
                break;
            case 0:
                start_timer(&error_popup_timer, 3.0f);
                snprintf(error_text, sizeof(error_text), "NO SAVE DATA FOUND!");
                break;
            case 1:
                change_scene(MAINMENU);
                break;
        }
    }
}

static void new_game_callback(const int option, void *user_data) {
    if (option == 1) { // yes pressed
        new_game();
        const int return_code = save_game();

        switch (return_code) {
            case -1:
                start_timer(&error_popup_timer, 3.0f);
                snprintf(error_text, sizeof(error_text), "NO VMU FOUND!");
                break;
            case 0:
                start_timer(&error_popup_timer, 3.0f);
                snprintf(error_text, sizeof(error_text), "NOT ENOUGH SPACE IN VMU!");
                break;
            case 1:
                change_scene(MAINMENU);
                break;
        }
    }
}

void draw_loading_scene(void) {
    draw_background();

    static void (*callback)(int option, void *user_data) = nullptr;
    static char message[50];

    if (do_button(load_button_, true) && !error_popup_timer.is_running) {
        callback = load_game_callback;
        set_selected_layer(1);
        snprintf(message, sizeof(message), "Load the game?");
    }

    if (do_button(new_game_button_, true) && !error_popup_timer.is_running) {
        callback = new_game_callback;
        set_selected_layer(1);
        snprintf(message, sizeof(message), "Start a new game?");
    }

    draw_confirmation_window(callback, nullptr, message);
    draw_error_popup();
}