/* KallistiOS ##version##
   examples/dreamcast/raylib/beachbox/src/background.c
   Copyright (C) 2024 Agustín Bellagamba
   Copyright (C) 2024 Cypress
*/

#include <raylib.h>
#include <math.h>

#include "background.h"
#include "helper_functions.h"
#include "config.h"
#include "timer.h"
#include "scenes/game.h"
#include "player.h"
#include "objects.h"

static constexpr int   max_sand_particles_ = 20;
static constexpr float sand_spawn_rate_    = 0.15; // in seconds, lower is faster
static constexpr int   slow_wave_height_   = 12;
static constexpr int   slow_wave_width_    = 8;
static constexpr int   fast_wave_height_   = 20;
static constexpr int   fast_wave_width_    = 15;

struct SandParticle {
        Vector2 pos;
        bool    active;
} static sand_particles[max_sand_particles_] = { 0 };

void draw_ocean(void) {
    const float current_object_speed = get_current_object_speed();

    Color ocean_color = { 66, 147, 255, 255 };
    if (is_slowdown_active()) invert_color(&ocean_color);
    DrawRectangle(0, 250, SCREEN_WIDTH, FLOOR_HEIGHT - 250, ocean_color);

    Color colors[4] = {
        { 173, 216, 230, 255 },
        { 135, 206, 250, 255 },
        { 135, 206, 235, 255 },
        { 70,  130, 180, 255 },
    };

    if (is_slowdown_active()) {
        for (int i = 0; i < 4; i++) {
            invert_color(&colors[i]);
        }
    }

    static float slow_wave_step = 0;
    static float fast_wave_step = 0;

    if (!is_game_paused()) {
        slow_wave_step += current_object_speed != 0 ? 0.002f * current_object_speed : 0.01f;
        fast_wave_step += current_object_speed != 0 ? 0.004f * current_object_speed : 0.02f;
    }

    const int center_y = SCREEN_HEIGHT / 2;
    for (int row = 0; row < 2; row++) {
        int y = center_y + (row * slow_wave_height_);
        for (int x = 0; x < SCREEN_WIDTH; x += slow_wave_width_) {
            const float offsetY = sinf((x + slow_wave_step * 100) / 50.0f) * slow_wave_height_ / 2;
            DrawRectangle(x, y + (int)offsetY, slow_wave_width_, slow_wave_height_, colors[row]);
        }
    }

    for (int row = 0; row < 2; row++) {
        int y = center_y + (row * fast_wave_height_);
        for (int x = 0; x < SCREEN_WIDTH; x += fast_wave_width_) {
            const float offsetY = sinf((x + fast_wave_step * 100) / 50.0f) * fast_wave_height_ / 2 + 45;
            DrawRectangle(x, y + (int)offsetY, fast_wave_width_, fast_wave_height_, colors[row + 2]);
        }
    }
}

void draw_sand_particles(void) {
    const float current_object_speed = get_current_object_speed();

    static bbox_timer_t sand_particle_spawn_timer;
    update_timer(&sand_particle_spawn_timer);
    Color sand_particle_color = BROWN;
    if (is_slowdown_active()) invert_color(&sand_particle_color);

    for (int i = 0; i < max_sand_particles_; i++) {
        if (sand_particles[i].active) {
            DrawRectangleV(sand_particles[i].pos, (Vector2){ 7, 7 }, sand_particle_color);

            if (is_game_paused()) continue;

            sand_particles[i].pos.x -= current_object_speed != 0 ? current_object_speed : 5;

            if (sand_particles[i].pos.x < 0) {
                sand_particles[i].active = false;
            }

        } else {
            if (is_game_paused()) continue;
            if (!sand_particle_spawn_timer.is_running && GetRandomValue(0, 300) == 0) {
                start_timer(&sand_particle_spawn_timer, sand_spawn_rate_);
                sand_particles[i].pos    = (Vector2){ SCREEN_WIDTH, GetRandomValue(FLOOR_HEIGHT + 22, SCREEN_HEIGHT - 22) };
                sand_particles[i].active = true;
            }
        }
    }
}

void draw_background(void) {
    Color sky_color = { 135, 206, 250, 255 };
    if (is_slowdown_active()) invert_color(&sky_color);
    Color sand_color = { 242, 195, 68, 255 };
    if (is_slowdown_active()) invert_color(&sand_color);
    DrawRectangle(0, 0, SCREEN_WIDTH, FLOOR_HEIGHT, sky_color); // Draw sky
    draw_ocean();
    DrawRectangle(0, FLOOR_HEIGHT, SCREEN_WIDTH, SCREEN_HEIGHT - FLOOR_HEIGHT, sand_color); // Draw sand
    draw_sand_particles();
}