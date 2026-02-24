/* KallistiOS ##version##
   examples/dreamcast/raylib/beachbox/src/scenes/shop.c
   Copyright (C) 2024 Agustín Bellagamba
   Copyright (C) 2024 Cypress
*/

#include <string.h>

#include "shop.h"
#include "../ui.h"
#include "../scene.h"
#include "../save.h"
#include "../background.h"
#include "../config.h"

static player_upgrade_t selected_shop_option_;

static const uibutton_t shop_buttons_[] = {
    { .pos = { 30, 50 },   .size = { 200, 40 }, .column = 0, .row = 0, .layer = 0, .text = "Movement Speed"    },
    { .pos = { 30, 110 },  .size = { 200, 40 }, .column = 0, .row = 1, .layer = 0, .text = "Max Meter"         },
    { .pos = { 30, 170 },  .size = { 200, 40 }, .column = 0, .row = 2, .layer = 0, .text = "Unlock Teleport"   },
    { .pos = { 30, 230 },  .size = { 200, 40 }, .column = 0, .row = 3, .layer = 0, .text = "Teleport Cooldown" },
    { .pos = { 30, 290 },  .size = { 200, 40 }, .column = 0, .row = 4, .layer = 0, .text = "Teleport Distance" },
    { .pos = { 30, 350 },  .size = { 200, 40 }, .column = 0, .row = 5, .layer = 0, .text = "Unlock Slowdown"   },
    { .pos = { 30, 410 },  .size = { 200, 40 }, .column = 0, .row = 6, .layer = 0, .text = "Slowdown Drain"    },
    { .pos = { 340, 400 }, .size = { 100, 40 }, .column = 1, .row = 0, .layer = 0, .text = "Buy"               },
    { .pos = { 460, 400 }, .size = { 100, 40 }, .column = 2, .row = 0, .layer = 0, .text = "Exit"              },
};

static constexpr uint8_t costs_[UPGRADE_COUNT] = {
    20, // SPEED
    20, // MAX_METER
    50, // TELEPORT_UNLOCK
    30, // TELEPORT_COOLDOWN
    30, // TELEPORT_DISTANCE
    50, // SLOWDOWN_UNLOCK
    30, // SLOWDOWN_DRAIN
};

static inline bool can_afford(player_upgrade_t option) {
    return costs_[option] <= get_total_coins();
}

static inline bool can_upgrade(player_upgrade_t option) {
    return get_upgrade_level(option) < get_max_upgrade_level(option);
}

static inline void purchase(void) {
    increment_upgrade_level(selected_shop_option_);
    add_coins(-costs_[selected_shop_option_]);
}

void draw_shop_description(void) {
    static const char *descriptions[] = {
        "Increases your movement speed\n\n\"For those who want to go fast\"",
        "Increases your max meter\n\nHelps you stay alive for longer\n\n\"I need more!\"",
        "Unlocks the teleport power\n\nTeleport forwards by pressing R\n\nHint:\nYou can teleport through\nthe big "
        "wall",
        "Dereases the teleport's cooldown\n\nAllows you to teleport more often",
        "Increases the teleport's distance\n\nAllows you to teleport further",
        "Unlocks the slowdown power\n\nSlow time down by pressing L",
        "Decreases the cost of slowdown\n\n\"Take it slow\"",
    };

    const char *description = descriptions[selected_shop_option_];
    const Color cost_color  = can_afford(selected_shop_option_) ? WHITE : RED;
    const Color level_color = get_upgrade_level(selected_shop_option_) == get_max_upgrade_level(selected_shop_option_) ? DARKGREEN : WHITE;

    DrawText(description, 270, 50, 20, WHITE);
    DrawText(TextFormat("Cost: %d coins", costs_[selected_shop_option_]), 270, 250, 20, cost_color);
    DrawText(TextFormat("Level: %d/%d", get_upgrade_level(selected_shop_option_), get_max_upgrade_level(selected_shop_option_)), 270, 300, 20, level_color);
}

void jump_to_buy_button(void) {
    set_selected_row(0);
    set_selected_column(1);
}

void init_shop_scene(void) {
    set_column_count(0, 3);
    set_column_count(1, 1);
    set_column_count(2, 1);
    set_column_count(3, 1);
    set_column_count(4, 1);
    set_column_count(5, 1);
    set_column_count(6, 1);

    set_row_count(0, 7);
    set_row_count(1, 1);
    set_row_count(2, 1);
}

void update_shop_scene(void) {
    if (get_selected_column() == 0 && get_selected_layer() == 0) {
        selected_shop_option_ = get_selected_row();
    }
}

static void exit_callback(const int option, void *user_data) {
    if (option == 1) { // yes pressed
        change_scene(MAINMENU);
        return;
    }
    if (option == 0) { // no pressed
        init_shop_scene();
        return;
    }
}

void draw_shop_scene(void) {
    draw_background();
    DrawRectangle(SCREEN_WIDTH * 0.4, 0, SCREEN_WIDTH * 0.6, SCREEN_HEIGHT, get_background_color());
    draw_shop_description();

    static void (*callback)(int option, void *user_data) = nullptr;

    // Draw total coins
    DrawRectangle(55, 0, 150, 40, get_button_color());
    DrawRectangleLines(55, 0, 150, 40, get_button_selected_color());
    const char *coins_text = TextFormat("Coins: %d", get_total_coins());
    DrawText(coins_text, 55 + (150 - MeasureText(coins_text, 20)) / 2, 10, 20, get_button_selected_color());

    DrawLine(SCREEN_WIDTH * 0.4, 0, SCREEN_WIDTH * 0.4, SCREEN_HEIGHT, RAYWHITE);

    bool can_buy = can_afford(selected_shop_option_) && can_upgrade(selected_shop_option_);

    // Buy
    if (do_button(shop_buttons_[7], can_buy) && can_buy) {
        purchase();
    }

    // Exit
    if (do_button(shop_buttons_[8], true)) {
        callback = exit_callback;
        set_selected_layer(1);
        set_selected_column(0);
        set_selected_row(0);
    }

    // Purchase options
    for (int i = 0; i < 7; i++) {
        if (do_button(shop_buttons_[i], can_afford(i) && can_upgrade(i))) {
            jump_to_buy_button();
        }
    }

    draw_confirmation_window(callback, nullptr, "Exit?");
}