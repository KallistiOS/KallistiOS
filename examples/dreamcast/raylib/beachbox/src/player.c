/* KallistiOS ##version##
   examples/dreamcast/raylib/beachbox/src/player.c
   Copyright (C) 2024 Agustín Bellagamba
   Copyright (C) 2024 Cypress
*/

#include "player.h"
#include "timer.h"
#include "controller.h"
#include "config.h"
#include "hats.h"
#include "helper_functions.h"
#include "save.h"
#include "audio.h"
#include "scenes/game.h"

static constexpr int player_colors_count_ = 17;

static const Color player_colors[player_colors_count_] = {
    RED,
    ORANGE,
    GOLD,
    YELLOW,
    DARKGREEN,
    LIME,
    GREEN,
    BLUE,
    (Color){ 137, 207, 240, 255 }, // Light blue
    (Color){ 75,  54,  157, 255 }, // Indigo
    (Color){ 112, 54,  157, 255 }, // Violet
    MAGENTA,
    PINK,
    WHITE,
    GRAY,
    DARKBROWN,
    DARKGRAY,
};

Color get_player_color(int index) {
    return player_colors[index];
}

int get_player_color_count(void) {
    return player_colors_count_;
}

static character_t  player                  = { 0 };
static bbox_timer_t teleport_duration_timer = { 0 };
static bbox_timer_t teleport_cooldown_timer = { 0 };
static bool         is_slowdown_active_     = false;
static bool         is_teleporting_         = false;

bool is_slowdown_active(void) {
    return is_slowdown_active_;
}

bool is_player_teleporting(void) {
    return is_teleporting_;
}

bool is_player_shifted(void) {
    return player.is_shifted;
}

Rectangle get_player_rect(void) {
    return (Rectangle){ player.pos.x, player.pos.y, player.size.x, player.size.y };
}

float get_player_meter(void) {
    return player.meter;
}

float get_player_max_meter(void) {
    return player.max_meter;
}

void refill_player_meter(float amount) {
    player.meter = BBOX_MIN(get_player_meter() + amount, get_player_max_meter());
}

bbox_timer_t get_teleport_cooldown_timer(void) {
    return teleport_cooldown_timer;
}

void init_player(void) {
    player.size       = (Vector2){ 32.0f, 32.0f };
    player.pos        = (Vector2){ .x = 100, .y = FLOOR_HEIGHT - player.size.y };
    player.velocity   = (Vector2){ 0.0f, 0.0f };
    player.speed      = 6.5 + 0.5 * get_upgrade_level(UPGRADE_SPEED);
    player.max_meter  = 100 + 10 * get_upgrade_level(UPGRADE_METER);
    player.meter      = player.max_meter;
    player.is_shifted = false;
    player.color      = player_colors[get_player_current_color_index()];

    is_slowdown_active_ = false;
    is_teleporting_     = false;
}

void move_player(Vector2 direction) {
    if (is_teleporting_) return; // If the player is teleporting, we don't let the player move
    player.velocity.x += direction.x * player.speed;
}

void shift_player(bool should_shift) {
    player.is_shifted = should_shift;
}

void update_player_pos(void) {
    player.pos.x += is_teleporting_ ? 8.5 + 0.5 * get_upgrade_level(UPGRADE_TELEPORT_DISTANCE) : player.velocity.x;
    if (player.pos.x < 0) player.pos.x = 0;
    if (player.pos.x > SCREEN_WIDTH - player.size.x) player.pos.x = SCREEN_WIDTH - player.size.x;
    player.velocity.x = 0;

    if (is_teleporting_) return; // If the player is teleporting, we skip falling down
    player.velocity.y += player.velocity.y < 0 ? GRAVITY : GRAVITY * 1.5;
    player.pos.y      += player.velocity.y;

    player.pos.y = BBOX_MIN(player.pos.y, FLOOR_HEIGHT - player.size.y);

    if (player.pos.y > FLOOR_HEIGHT - player.size.y) {
        player.pos.y      = FLOOR_HEIGHT - player.size.y;
        player.velocity.y = 0;
    }
}

void jump(void) {
    const bool    is_player_on_ground = player.pos.y == FLOOR_HEIGHT - player.size.y;
    constexpr int jump_force          = 15;

    if (is_player_on_ground) {
        player.velocity.y = -jump_force;
    }
}

void cut_jump(void) {
    if (player.velocity.y < 0) {
        player.velocity.y *= 0.7;
    }
}

void toggle_slowdown(void) {
    if (!get_upgrade_level(UPGRADE_SLOWDOWN_UNLOCK)) return;

    if (!is_slowdown_active_) play_sfx_slowdown_activate();
    else play_sfx_slowdown_deactivate();

    is_slowdown_active_ = !is_slowdown_active_;
}

void turn_slowdown_off(void) {
    is_slowdown_active_ = false;
}

void teleport(void) {
    if (!get_upgrade_level(UPGRADE_TELEPORT_UNLOCK)) return;

    if (teleport_cooldown_timer.is_running) return;
    if (!teleport_duration_timer.is_running) { // If the player can teleport
        play_sfx_teleport();
        is_teleporting_ = true;
        start_timer(&teleport_duration_timer, 0.4);
        start_timer(&teleport_cooldown_timer, 5 - 0.5 * get_upgrade_level(UPGRADE_TELEPORT_COOLDOWN));
    }
}

void update_player(void) {
    if (is_teleporting_) {
        player.velocity.y = BBOX_MIN(player.velocity.y, 0); // If the player was falling down before teleporting, we set it back to 0
        update_timer(&teleport_duration_timer);
        if (!teleport_duration_timer.is_running) is_teleporting_ = false;
    }
    update_timer(&teleport_cooldown_timer);

    update_player_pos();
    player.meter -= 0.16;
    if (is_slowdown_active_) player.meter -= 0.22 / (1 + get_upgrade_level(UPGRADE_SLOWDOWN_DRAIN) / 4);

    if (player.meter <= 0) {
        end_game();
    }
}

void draw_player(void) {
    constexpr Color transparent = { 0, 0, 0, 0 };
    constexpr Color translucid  = { 0, 0, 0, 120 };

    Color current_player_color = is_teleporting_ ? transparent : player.color;
    current_player_color.a     = player.is_shifted ? 140 : current_player_color.a;

    Color hat_color = is_teleporting_ ? translucid : WHITE;
    hat_color.a     = player.is_shifted ? 140 : hat_color.a;

    DrawRectangleV(player.pos, player.size, current_player_color);
    DrawRectangleLinesExV(player.pos, player.size, 2, BLACK);

    const hat_t current_hat_index = get_current_hat_type();

    if (current_hat_index != HAT_NIL) {
        float x_pos = player.pos.x + player.size.x / 4;

        // F and Murph need +2 X alignment
        if (current_hat_index == HAT_F || current_hat_index == HAT_MUPRH) {
            x_pos += 2;
        }

        float y_pos = player.pos.y - 16;

        // M and L need +4 Y alignment, Z needs +3, Crown needs +1
        if (current_hat_index == HAT_M || current_hat_index == HAT_L) {
            y_pos += 4;
        } else if (current_hat_index == HAT_Z) {
            y_pos += 3;
        } else if (current_hat_index == HAT_CROWN) {
            y_pos += 1;
        }

        const Texture2D *current_hat_texture = get_hat_texture(current_hat_index);
        DrawTextureV(*current_hat_texture, (Vector2){ x_pos, y_pos }, hat_color);
    }
}