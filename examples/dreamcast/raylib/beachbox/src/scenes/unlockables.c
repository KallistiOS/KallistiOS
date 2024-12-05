/* KallistiOS ##version##
   examples/dreamcast/raylib/beachbox/src/scenes/unlockables.c
   Copyright (C) 2024 Agustín Bellagamba
   Copyright (C) 2024 Cypress
*/

#include <raylib.h>
#include "../ui.h"
#include "../save.h"
#include "../hats.h"
#include "../background.h"
#include "../controller.h"
#include "../helper_functions.h"
#include "../config.h"
#include "../player.h"
#include "../scene.h"
#include "unlockables.h"

static constexpr Vector2 button_size_    = { .x = 200, .y = 50 };
static constexpr float   button_padding_ = 15;

const uibutton_t confirm_button_ = {
    .pos    = { .x = SCREEN_WIDTH / 2 - button_size_.x / 2, .y = SCREEN_HEIGHT - button_size_.y / 2 - button_padding_ * 2 },
    .size   = button_size_,
    .column = 0,
    .row    = 2,
    .layer  = 0,
    .text   = "Confirm",
};

static constexpr uint8_t hat_prices_[HAT_COUNT] = {
    0,   // No hat
    50,  // Red Slime
    50,  // Blue Slime
    80,  // Box
    100, // M hat
    100, // L hat
    100, // Z hat
    80,  // F hat
    150, // Murph hat
    0,   // Crown (not purchaseable)
};

void draw_hat_price(const uint8_t index) {
    Vector2 size = { 400, 130 };
    Vector2 pos  = { .x = SCREEN_WIDTH / 2 - size.x / 2, .y = FLOOR_HEIGHT - size.y * 2 };
    DrawRectangleV(pos, size, get_background_color());

    DrawText("Locked!", pos.x + size.x / 2 - MeasureText("Locked!", 20) / 2, pos.y + size.y / 2 - 50, 20, RED);

    if (index != 9) { // if not crown
        const char *price_text = TextFormat("Price: %d", hat_prices_[index]);
        DrawText(price_text, pos.x + size.x / 2 - MeasureText(price_text, 20) / 2, pos.y + size.y / 2 - 20, 20, WHITE);
        if (get_selected_row() == 2) return; // if on confirm button
        DrawText("Press A to buy", pos.x + size.x / 2 - MeasureText("Press A to buy", 20) / 2, pos.y + size.y / 2 + 30, 20, WHITE);
        return;
    }

    // Crown
    const char *crown_explanation_text = TextFormat("Reach a high score of 100 to unlock!");
    DrawText(crown_explanation_text, pos.x + size.x / 2 - MeasureText(crown_explanation_text, 20) / 2, pos.y + size.y / 2, 20, WHITE);
}

void init_unlockables_scene(void) {
    set_row_count(0, 3);
    set_column_count(0, 1);
}

void update_unlockables_scene(void) {
    //
}

static void purchase_hat(int option, void *user_data) {
    init_unlockables_scene();
    if (option == 1) {
        add_coins(-hat_prices_[get_current_hat_type()]);
        unlock_hat(get_current_hat_type());
    }
}

void draw_unlockables_scene(void) {
    draw_background();

    // Draw total coins
    DrawRectangle(245, 0, 150, 40, get_button_color());
    DrawRectangleLines(245, 0, 150, 40, get_button_selected_color());
    const char *coins_text = TextFormat("Coins: %d", get_total_coins());
    DrawText(coins_text, 245 + (150 - MeasureText(coins_text, 20)) / 2, 10, 20, get_button_selected_color());

    // Draw player
    constexpr Vector2 player_size  = (Vector2){ .x = 64, .y = 64 };
    constexpr Vector2 player_pos   = (Vector2){ .x = SCREEN_WIDTH / 2 - player_size.x / 2, .y = FLOOR_HEIGHT - player_size.y };
    const Color       player_color = get_player_color(get_player_current_color_index());

    DrawRectangleV(player_pos, player_size, player_color);
    DrawRectangleLinesExV(player_pos, player_size, 2, BLACK);

    const hat_t current_hat_type = get_current_hat_type();

    // Draw player hat
    if (current_hat_type != HAT_NIL) {
        float x_pos = player_pos.x + player_size.x / 4;

        // F and Murph need +4 X alignment
        if (current_hat_type == HAT_F || current_hat_type == HAT_MUPRH) {
            x_pos += 4;
        }

        float y_pos = player_pos.y - 32;

        // M and L need +8 Y alignment, Z needs +6, Crown needs +2
        if (current_hat_type == HAT_M || current_hat_type == HAT_L) {
            y_pos += 8;
        } else if (current_hat_type == HAT_Z) {
            y_pos += 4;
        } else if (current_hat_type == HAT_CROWN) {
            y_pos += 2;
        }

        const Texture2D *current_hat_texture = get_hat_texture(current_hat_type);
        DrawTextureEx(*current_hat_texture, (Vector2){ x_pos, y_pos }, 0, 2, WHITE);
    }

    // Hat selection arrows
    const Vector2 arrows_size = (Vector2){ .x = 24, .y = 24 };

    const uiarrows_t hat_arrows = {
        .column    = 0,
        .row       = 0,
        .layer     = 0,
        .pos_left  = { .x = player_pos.x - arrows_size.x * 2,             .y = player_pos.y - arrows_size.y * 0.8 },
        .pos_right = { .x = player_pos.x + player_size.x + arrows_size.x, .y = player_pos.y - arrows_size.y * 0.8 },
        .size      = arrows_size,
    };

    static void (*callback)(int option, void *user_data) = nullptr;

    if (!is_hat_unlocked(current_hat_type)) {
        draw_hat_price(current_hat_type);
        const bool can_buy_hat = get_total_coins() >= hat_prices_[current_hat_type] && get_selected_row() != 2 && current_hat_type != HAT_CROWN;
        if (IsGamepadButtonReleased(0, BUTTON_A) && can_buy_hat) {
            callback = purchase_hat;
            set_selected_layer(1);
        }
    }

    const int hat_arrows_return_code = do_arrows(hat_arrows);

    if (are_arrows_selected(hat_arrows)) {
        if (hat_arrows_return_code == 1) {
            increment_current_hat_index();
        } else if (hat_arrows_return_code == -1) {
            decrement_current_hat_index();
        }
    }

    // Color selection arrows
    const uiarrows_t color_arrows = {
        .column    = 0,
        .row       = 1,
        .layer     = 0,
        .pos_left  = { .x = player_pos.x - arrows_size.x * 2,             .y = player_pos.y + arrows_size.y * 0.8 },
        .pos_right = { .x = player_pos.x + player_size.x + arrows_size.x, .y = player_pos.y + arrows_size.y * 0.8 },
        .size      = arrows_size,
    };

    const int color_arrows_return_code = do_arrows(color_arrows);
    if (are_arrows_selected(color_arrows)) {
        if (color_arrows_return_code == 1) {
            increment_player_color_index();
        } else if (color_arrows_return_code == -1) {
            decrement_player_color_index();
        }
    }

    // Draw buttons
    if (do_button(confirm_button_, is_hat_unlocked(current_hat_type))) {
        change_scene(MAINMENU);
    }

    draw_confirmation_window(callback, nullptr, "Buy hat?");
}