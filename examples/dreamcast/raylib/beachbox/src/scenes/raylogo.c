/* KallistiOS ##version##
   examples/dreamcast/raylib/beachbox/src/scenes/raylogo.c
   Copyright (C) 2024 Agustín Bellagamba
   Copyright (C) 2024 Cypress
*/

#include <raymath.h>

#include "raylogo.h"
#include "../timer.h"
#include "../scene.h"
#include "../config.h"
#include "../helper_functions.h"

static constexpr int raylogo_width_       = 290;
static constexpr int raylogo_height_      = 300;
static constexpr int raylogo_square_size_ = 15;
static constexpr int raylogo_columns_     = (raylogo_width_ / raylogo_square_size_);
static constexpr int raylogo_rows_        = (raylogo_height_ / raylogo_square_size_);

static constexpr float animation_duration_ = 1.0f;
static float           time_elapsed_       = 0.0f;

void init_raylogo_scene(void) {
    //
}

void update_raylogo_scene(void) {
    if (time_elapsed_ >= animation_duration_ + 1.0f) {
        change_scene(LOADING);
    }
    time_elapsed_ = GetTime() - animation_duration_;
}

void draw_raylogo_scene(void) {
    ClearBackground(RAYWHITE);

    // Text

    Vector2 text_pos = (Vector2){ SCREEN_WIDTH / 2 - MeasureText("raylib", 52) / 2 + 35, SCREEN_HEIGHT / 2 + 65 };

    Vector2 white_box_size = {
        .x = (MeasureText("raylib", 52) + 40) * (1 - time_elapsed_),
        .y = 80,
    };

    DrawText("raylib", (int)text_pos.x, (int)text_pos.y, 52, BLACK);
    DrawRectangleV(text_pos, white_box_size, RAYWHITE);

    // Borders

    for (int row = 0; row < raylogo_rows_; row++) {
        for (int column = 0; column < raylogo_columns_; column++) {
            const bool is_border_square = row == 0 || column == 0 || row == raylogo_rows_ - 1 || column == raylogo_columns_ - 1;
            if (!is_border_square) continue;

            const float delay        = 1 * (fabs(column - row) / (float)((raylogo_rows_ + raylogo_columns_) / 2));
            const float amount       = BBOX_MIN(1, BBOX_MAX(0, time_elapsed_ - delay) * 2);
            const float current_size = Lerp(0, raylogo_square_size_, amount);

            const Vector2 pos = { .x = SCREEN_WIDTH / 2 - raylogo_width_ / 2 + column * raylogo_square_size_, .y = SCREEN_HEIGHT / 2 - raylogo_height_ / 2 + row * raylogo_square_size_ };

            DrawRectangleV(pos, (Vector2){ current_size, current_size }, BLACK);
        }
    }
}