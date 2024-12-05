/* KallistiOS ##version##
   examples/dreamcast/raylib/beachbox/src/scenes/game.c
   Copyright (C) 2024 Agustín Bellagamba
   Copyright (C) 2024 Cypress
*/

#include <raymath.h>

#include "game.h"
#include "../ui.h"
#include "../timer.h"
#include "../config.h"
#include "../save.h"
#include "../controller.h"
#include "../player.h"
#include "../scene.h"
#include "../helper_functions.h"
#include "../background.h"
#include "../objects.h"
#include "../audio.h"

static bool     is_game_over_;
static bool     is_game_paused_;
static bool     is_new_high_score_;
static bool     held_a_during_death_;
static uint16_t current_coins_ = 0; // The coins earned in the current run

static const uibutton_t play_again = {
    .pos    = { .x = 170, .y = SCREEN_HEIGHT * 0.6 },
    .size   = { .x = 140, .y = 50                  },
    .column = 0,
    .row    = 0,
    .layer  = 0,
    .text   = "Play Again",
};

static const uibutton_t main_menu_from_game = {
    .pos    = { .x = 330, .y = SCREEN_HEIGHT * 0.6 },
    .size   = { .x = 140, .y = 50                  },
    .column = 1,
    .row    = 0,
    .layer  = 0,
    .text   = "Main Menu",
};

void init_game_scene(void) {
    init_player();
    init_objects();
    is_game_over_      = false;
    current_coins_     = 0;
    is_new_high_score_ = false;

    set_column_count(0, 2);
    set_row_count(0, 1);
}

void update_game_scene(void) {
    if (is_game_over_) return;
    if (is_game_paused_) return;
    update_objects();
    update_player();
}

static void draw_game_over(void) {
    turn_slowdown_off(); // To turn off the inverted color effect
    reset_object_speed();
    if (is_new_high_score_) set_high_score(current_coins_);

    DrawRectangleV((Vector2){ SCREEN_WIDTH / 4, SCREEN_HEIGHT / 4 }, (Vector2){ SCREEN_WIDTH * 0.5, SCREEN_HEIGHT * 0.5 }, get_button_color());
    DrawText("You lost", SCREEN_WIDTH / 2 - MeasureText("You lost", 30) / 2, SCREEN_HEIGHT / 4 + 20, 30, RAYWHITE);

    const char *coins_text      = TextFormat("Score: %d", current_coins_);
    const char *high_score_text = TextFormat("High Score: %d", get_high_score());

    DrawText(coins_text, SCREEN_WIDTH / 2 - MeasureText(coins_text, 20) / 2, SCREEN_HEIGHT / 4 + 75, 20, RAYWHITE);
    DrawText(high_score_text, SCREEN_WIDTH / 2 - MeasureText(high_score_text, 20) / 2, SCREEN_HEIGHT / 4 + 95, 20, RAYWHITE);
    if (is_new_high_score_) {
        DrawText("New High Score!", SCREEN_WIDTH / 2 - MeasureText("New High Score!", 20) / 2, SCREEN_HEIGHT / 4 + 115, 20, RAYWHITE);
    }

    if (do_button(play_again, true) && !held_a_during_death_) {
        change_scene(GAME);
    }

    if (do_button(main_menu_from_game, true) && !held_a_during_death_) {
        change_scene(MAINMENU);
    }

    if (held_a_during_death_ && IsGamepadButtonReleased(0, BUTTON_A)) {
        held_a_during_death_ = false;
    }
}

static void draw_player_ui(void) {
    DrawRectangle(10, 10, 180, 80, get_background_color());

    // Current coint count
    const char *coins_text = TextFormat("Coins: %d", current_coins_);
    DrawText(coins_text, 100 - MeasureText(coins_text, 20) / 2, 20, 20, RAYWHITE);

    // Meter
    DrawRectangleV((Vector2){ .x = 25, .y = 50 }, (Vector2){ .x = 150, .y = 15 }, get_button_color());
    DrawRectangleV((Vector2){ .x = 25, .y = 50 }, (Vector2){ .x = Lerp(0, 150, get_player_meter() / get_player_max_meter()), .y = 15 }, BLUE);
    DrawRectangleLinesV((Vector2){ .x = 25, .y = 50 }, (Vector2){ .x = 150, .y = 15 }, BLACK);

    // Teleport cooldown
    const bbox_timer_t teleport_cooldown_timer = get_teleport_cooldown_timer();
    if (teleport_cooldown_timer.is_running) {
        DrawRectangleV((Vector2){ .x = 25, .y = 75 }, (Vector2){ .x = 150, .y = 8 }, RAYWHITE);
        DrawRectangleV((Vector2){ .x = 25, .y = 75 }, (Vector2){ .x = 150 * Lerp(0, 1, (teleport_cooldown_timer.progress / teleport_cooldown_timer.duration)), .y = 8 }, DARKGRAY);
        DrawRectangleLinesV((Vector2){ .x = 25, .y = 75 }, (Vector2){ .x = 150, .y = 8 }, BLACK);
    }
}

static void draw_pause_menu(void) {
    DrawRectangleV((Vector2){ SCREEN_WIDTH / 4, SCREEN_HEIGHT / 3 }, (Vector2){ SCREEN_WIDTH * 0.5, SCREEN_HEIGHT * 0.20 }, get_button_color());
    DrawText("Game Paused!", SCREEN_WIDTH / 2 - MeasureText("Game Paused!", 30) / 2, SCREEN_HEIGHT / 3 + 30, 30, RAYWHITE);
}

void draw_game_scene(void) {
    draw_background();
    if (is_game_over_) {
        draw_game_over();
        return;
    }

    // We change the draw order of the objects if the player is shifted / teleporting or not
    if (is_player_shifted() || is_player_teleporting()) {
        draw_objects();
        draw_player();
    } else {
        draw_player();
        draw_objects();
    }

    draw_player_ui();

    if (is_game_paused_) {
        draw_pause_menu();
    }
}

void end_game(void) {
#ifndef DEBUG_GODMODE
    if (IsGamepadButtonDown(0, BUTTON_A)) held_a_during_death_ = true;
    is_game_over_ = true;
    play_sfx_game_over();

    increment_total_runs();

    add_coins(current_coins_);
    is_new_high_score_ = get_high_score() < current_coins_;
#endif

    if (current_coins_ >= 100) {
        unlock_hat(HAT_CROWN);
    }
}

bool is_game_over(void) {
    return is_game_over_;
}

bool is_game_paused(void) {
    return is_game_paused_;
}

void toggle_pause(void) {
    is_game_paused_ = !is_game_paused_;
}

bool is_new_high_score(void) {
    return is_new_high_score_;
}

uint16_t get_current_coins(void) {
    return current_coins_;
}

void add_coin(void) {
    current_coins_++;
}