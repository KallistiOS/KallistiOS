/* KallistiOS ##version##
   examples/dreamcast/raylib/beachbox/src/scenes/options.c
   Copyright (C) 2024 Agustín Bellagamba
   Copyright (C) 2024 Cypress
*/

#include <adx/snddrv.h>
#include <adx/adx.h>

#include "options.h"
#include "../scene.h"
#include "../ui.h"
#include "../config.h"
#include "../background.h"
#include "../save.h"
#include "../audio.h"

static const uibutton_t music_volume_button_ = {
    .pos    = { .x = 40,  .y = 60 },
    .size   = { .x = 150, .y = 40 },
    .column = 0,
    .row    = 0,
    .layer  = 0,
    .text   = "Music volume",
};

static const uibutton_t sfx_volume_button_ = {
    .pos    = { .x = 40,  .y = 180 },
    .size   = { .x = 150, .y = 40  },
    .column = 0,
    .row    = 1,
    .layer  = 0,
    .text   = "SFX Volume",
};

static const uibutton_t new_save_button_ = {
    .pos    = { .x = 40,  .y = 300 },
    .size   = { .x = 150, .y = 40  },
    .column = 0,
    .row    = 2,
    .layer  = 0,
    .text   = "New Save",
};

static const uibutton_t exit_button_ = {
    .pos    = { .x = 40,  .y = 420 },
    .size   = { .x = 150, .y = 40  },
    .column = 0,
    .row    = 3,
    .layer  = 0,
    .text   = "Return",
};

void init_options_scene(void) {
    set_row_count(0, 4);
    set_column_count(0, 1);
}

void update_options_scene(void) {
    //
}

static void new_game_callback(int option, void *user_data) {
    if (option == 1) { // yes pressed
        new_game();
        change_scene(MAINMENU);
        return;
    }
    if (option == 0) { // no pressed
        init_options_scene();
        return;
    }
}

static void DrawRectangleBars(int count, int posX, int posY) {
    constexpr int bar_width   = 10;
    constexpr int spacing     = 15;
    constexpr int base_height = 40;

    for (int i = 0; i < 24; i++) {

        DrawRectangle(posX + i * spacing, posY, bar_width, base_height, SKYBLUE);
    }
    for (int i = 0; i < count; i++) {

        DrawRectangle(posX + i * spacing, posY, bar_width, base_height, BLUE);
    }
}

void draw_options_scene(void) {
    draw_background();

    DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, get_background_color());

    static void (*callback)(int option, void *user_data) = nullptr;

    DrawRectangleBars(get_music_volume(), 230, 60);
    DrawRectangleBars(get_sfx_volume(), 230, 180);

    // Music volume
    const Vector2    arrows_size_         = (Vector2){ .x = 24, .y = 24 };
    const uiarrows_t music_volume_arrows_ = {
        .column    = 0,
        .row       = 0,
        .layer     = 0,
        .pos_left  = { 18,  65 },
        .pos_right = { 187, 65 },
        .size      = arrows_size_,
    };

    const int music_arrows_return_code = do_arrows(music_volume_arrows_);
    if (are_arrows_selected(music_volume_arrows_)) {
        if (music_arrows_return_code == 1) {
            increment_music_volume();
        } else if (music_arrows_return_code == -1) {
            decrement_music_volume();
        }
    }

    // SFX volume
    const uiarrows_t sfx_volume_arrows_ = {
        .column    = 0,
        .row       = 1,
        .layer     = 0,
        .pos_left  = { 18,  185 },
        .pos_right = { 187, 185 },
        .size      = arrows_size_,
    };

    const int sfx_arrows_return_code = do_arrows(sfx_volume_arrows_);
    if (are_arrows_selected(sfx_volume_arrows_)) {
        if (sfx_arrows_return_code == 1) {
            increment_sfx_volume();
        } else if (sfx_arrows_return_code == -1) {
            decrement_sfx_volume();
        }
    }

    if (do_button(music_volume_button_, true)) {
        //
    }

    if (do_button(sfx_volume_button_, true)) {
        //
    }

    if (do_button(new_save_button_, true)) {
        callback = new_game_callback;
        set_selected_layer(1);
        set_selected_column(0);
        set_selected_row(0);
    }

    if (do_button(exit_button_, true)) {
        change_scene(MAINMENU);
    }

    draw_confirmation_window(callback, nullptr, "Start a new game?");
}