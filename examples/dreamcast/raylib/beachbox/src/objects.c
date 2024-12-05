/* KallistiOS ##version##
   examples/dreamcast/raylib/beachbox/src/objects.c
   Copyright (C) 2024 Agustín Bellagamba
   Copyright (C) 2024 Cypress
*/

#include "timer.h"
#include "config.h"
#include "objects.h"
#include "audio.h"
#include "player.h"
#include "helper_functions.h"
#include "scenes/game.h"

static constexpr uint8_t max_objects_      = 16; // 16 is the limit since we are using a 16-bit bitfield
static constexpr float   max_object_speed_ = 13;
static constexpr float   coin_size_        = 10.0f;

struct Objects {
        Vector2 size[max_objects_ / 2]; // half because all coins are the same size
        Vector2 pos[max_objects_];
} static objects_ = { 0 };

// Bitfield that keeps track of which objects are active, 0 being inactive and 1 active
static uint16_t active_objects_bitfield_;

// We split the bitfield in half, the first 8 bits being the pillars and the last 8 bits being the coins
static constexpr uint8_t pillars_fist_bit_ = 0;
static constexpr uint8_t pillars_last_bit_ = 7;
static constexpr uint8_t coins_first_bit_  = 8;
static constexpr uint8_t coins_last_bit_   = 15;

// So for example, a value of 0b00010000'00000001 would mean there's a pillar occupying the first bit,
// and a coin on the 13th bit

// Bitfield that keeps track of which objects are shifted, 0 being not shifted and 1 shifted
static uint16_t shifted_objects_bitfield_;

static float base_object_speed;
static float current_object_speed_;

// Cooldown variables
static float        coin_spawn_cooldown_;
static float        pillar_spawn_cooldown_;
static float        giant_pillar_spawn_cooldown_;
static bbox_timer_t coin_spawn_timer_;
static bbox_timer_t pillar_spawn_timer_;
static bbox_timer_t giant_pillar_spawn_timer_;

static void calculate_object_cooldowns(void) {
    coin_spawn_cooldown_         = 8 / current_object_speed_;
    pillar_spawn_cooldown_       = 15 / current_object_speed_;
    giant_pillar_spawn_cooldown_ = 70 / current_object_speed_;
}

void init_objects(void) {
    active_objects_bitfield_ = 0;
    current_object_speed_ = base_object_speed = 5;
    start_timer(&coin_spawn_timer_, 1);
    start_timer(&pillar_spawn_timer_, 2);
    start_timer(&giant_pillar_spawn_timer_, 30); // The first giant pillar spawns later in the run
}

static void spawn_pillar(void) {
    if (pillar_spawn_timer_.is_running) return;

    uint16_t index;
    for (index = pillars_fist_bit_; index <= pillars_last_bit_; index++) {
        if (!(active_objects_bitfield_ & (1 << index))) {
            break;
        }
    }

    active_objects_bitfield_ |= (1 << index);

    // Spawn a giant pillar
    if (!giant_pillar_spawn_timer_.is_running) {
        objects_.size[index]       = (Vector2){ .x = 200, .y = FLOOR_HEIGHT };
        objects_.pos[index]        = (Vector2){ .x = SCREEN_WIDTH + 100, .y = FLOOR_HEIGHT - objects_.size[index].y };
        shifted_objects_bitfield_ &= ~(1 << index);

        // We start both timers to avoid spawning two pillars on top of each other
        start_timer(&giant_pillar_spawn_timer_, giant_pillar_spawn_cooldown_);
        start_timer(&pillar_spawn_timer_, pillar_spawn_cooldown_);
        return;
    }

    // If couldn't spawn a giant pillar, spawn a normal pillar
    objects_.size[index] = (Vector2){ .x = GetRandomValue(30, 50), .y = GetRandomValue(80, 120) };
    objects_.pos[index]  = (Vector2){ .x = SCREEN_WIDTH + 100, .y = FLOOR_HEIGHT - objects_.size[index].y };

    bool is_upside_down = GetRandomValue(0, 1);
    if (is_upside_down) {
        objects_.size[index].y = GetRandomValue(270, 300);
        objects_.pos[index].y  = -5;
    }

    shifted_objects_bitfield_ |= (1 << index) & GetRandomValue(0, 1);
    start_timer(&pillar_spawn_timer_, pillar_spawn_cooldown_);
    return;
}

// This function is also responsible of increasing the object speed each time a coin is spawned
static void spawn_coin(void) {
    if (coin_spawn_timer_.is_running) return;
    uint16_t index;
    for (index = coins_first_bit_; index <= coins_last_bit_; index++) {
        if (!(active_objects_bitfield_ & (1 << index))) {
            break;
        }
    }

    active_objects_bitfield_ |= (1 << index);

    objects_.pos[index]        = (Vector2){ .x = SCREEN_WIDTH + 100, .y = GetRandomValue(190, FLOOR_HEIGHT - 100) };
    shifted_objects_bitfield_ |= (1 << index) & GetRandomValue(0, 1);

    base_object_speed = BBOX_MIN(base_object_speed + 0.08, max_object_speed_);
    calculate_object_cooldowns();
    start_timer(&coin_spawn_timer_, coin_spawn_cooldown_);
}

static void add_objects(void) {
    if (active_objects_bitfield_ == max_objects_) return;
    spawn_pillar();
    spawn_coin();
}

static inline bool is_giant_pillar(Vector2 size) {
    return size.y >= FLOOR_HEIGHT;
}

void update_objects(void) {
    update_timer(&coin_spawn_timer_);
    update_timer(&pillar_spawn_timer_);
    update_timer(&giant_pillar_spawn_timer_);
    current_object_speed_ = is_slowdown_active() ? base_object_speed / 1.5 : base_object_speed;
    add_objects();
    const Rectangle player_rect = get_player_rect();

    for (int index = 0; index < max_objects_; index++) {
        if (!(active_objects_bitfield_ & (1 << index))) continue; // Skip if the object is not active
        objects_.pos[index].x -= current_object_speed_;           // Move objects

        // Remove objects that are off the screen
        if (index >= coins_first_bit_ && index <= coins_last_bit_) {
            if (objects_.pos[index].x < -coin_size_) {
                active_objects_bitfield_ &= ~(1 << index);
                continue;
            }
        } else {
            if (objects_.pos[index].x < -objects_.size[index].x) {
                active_objects_bitfield_ &= ~(1 << index);
                continue;
            }
        }

        // Check for collisions with the player

        // Coins
        if (index >= coins_first_bit_ && index <= coins_last_bit_) {
            if ((shifted_objects_bitfield_ & (1 << index)) != is_player_shifted() && !is_player_teleporting())
                continue; // If the player and the coin's "dimension" do not match, skip
                          // If the player is teleporting, we grab the coin no matter what

            // Checking against a bigger hitbox when grabbing coins makes the game feel more fair
            const Rectangle player_rect_coins = (Rectangle){ player_rect.x, player_rect.y, player_rect.width * 1.2, player_rect.height * 1.2 };
            if (CheckCollisionCircleRec(objects_.pos[index], coin_size_, player_rect_coins)) {
                active_objects_bitfield_ &= ~(1 << index);
                play_sfx_coin();
                add_coin();
                refill_player_meter(20);
            }
        }

        // Pillars
        if (index >= pillars_fist_bit_ && index <= pillars_last_bit_) {
            if (is_player_teleporting() || (!is_giant_pillar(objects_.size[index]) && (shifted_objects_bitfield_ & (1 << index)) != is_player_shifted()))
                continue; // If the player and the pillar's "dimension" do not match, skip
                          // If the player is teleporting, we do not check for collisions

            const Vector2 player_pos  = (Vector2){ player_rect.x, player_rect.y };
            // Checking against a smaller hitbox when colliding with pillars makes the game feel more fair
            const Vector2 player_size = (Vector2){ player_rect.width * 0.8, player_rect.height * 0.8 };

            if ((CheckCollisionRectangleV(player_pos, player_size, objects_.pos[index], objects_.size[index]))) {
                end_game();
                break;
            }
        }
    }
}

void draw_objects(void) {
    // Pillars
    for (int index = pillars_fist_bit_; index <= pillars_last_bit_; index++) {
        if (!(active_objects_bitfield_ & (1 << index))) continue;

        Color color = is_giant_pillar(objects_.size[index]) ? (Color){ 136, 216, 176, 255 } : (Color){ 255, 154, 49, 255 };

        if (is_slowdown_active()) invert_color(&color);
        if (shifted_objects_bitfield_ & (1 << index)) {
            color.a = 50;
        }

        DrawRectangleV(objects_.pos[index], objects_.size[index], color);
        DrawRectangleLinesExV(objects_.pos[index], objects_.size[index], 2, BLACK);
    }

    // Coins
    for (int index = coins_first_bit_; index <= coins_last_bit_; index++) {
        Color color = { 224, 212, 0, 255 };

        if (is_slowdown_active()) invert_color(&color);
        if (shifted_objects_bitfield_ & (1 << index)) {
            color.a = 100;
        }

        if (active_objects_bitfield_ & (1 << index)) {
            DrawCircleV(objects_.pos[index], coin_size_, color);
            DrawCircleLinesExV(objects_.pos[index], coin_size_, 1.25, BLACK);
        }
    }
}

float get_current_object_speed(void) {
    return current_object_speed_;
}

void reset_object_speed(void) {
    current_object_speed_ = 5.0f;
}