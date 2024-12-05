/* KallistiOS ##version##
   examples/dreamcast/raylib/beachbox/src/ui.c
   Copyright (C) 2024 Agustín Bellagamba
   Copyright (C) 2024 Cypress
*/

#include <assert.h>
#include <stdlib.h>

#include "audio.h"
#include "controller.h"
#include "config.h"
#include "helper_functions.h"
#include "ui.h"

static ui_state_t      ui_state_                 = { 0 };
static constexpr Color ui_background_color_      = (Color){ 1, 17, 35, 180 };
static constexpr Color ui_button_color_          = (Color){ 1, 35, 69, 200 };
static constexpr Color ui_button_selected_color_ = (Color){ 230, 230, 230, 222 };

void move_cursor(char direction) {
    uint8_t *selected_column = &ui_state_.selected.column;
    uint8_t *selected_row    = &ui_state_.selected.row;
    uint8_t *selected_layer  = &ui_state_.selected.layer;

    // All popups in this game only have two buttons on the same row, so we can assume this:
    if (*selected_layer == 1) {
        ui_state_.column_count[0] = 2;
        ui_state_.row_count[0]    = 1;
        ui_state_.row_count[1]    = 1;
    }

    const bool can_move_cursor_left  = *selected_column > 0;
    const bool can_move_cursor_right = *selected_column < ui_state_.column_count[*selected_row] - 1;
    const bool can_move_cursor_up    = *selected_row > 0;
    const bool can_move_cursor_down  = *selected_row < ui_state_.row_count[*selected_column] - 1;

    switch (direction) {
        case 'L':
            if (can_move_cursor_left) {
                --*selected_column;
            } else if (*selected_column != 0) {
                // Check if there's a button above or below selected_column - 1
                for (int offset = -ui_max_rows_; offset <= ui_state_.row_count[*selected_column]; offset++) {
                    const int new_row = *selected_row + offset;

                    const bool row_exists = new_row >= 0 && new_row < ui_state_.row_count[*selected_column];
                    if (!row_exists) continue;

                    const bool column_exists = ui_state_.column_count[new_row] > 0 && ui_state_.column_count[new_row] > *selected_column - 1;
                    if (!column_exists) continue;

                    --*selected_column;
                    *selected_row = new_row;
                    break;
                }
            }
            break;

        case 'R':
            if (can_move_cursor_right) {
                ++*selected_column;
            } else if (*selected_column != UINT8_MAX) {
                // Check if there's a button above or below selected_column + 1
                for (int offset = -ui_max_rows_; offset <= ui_state_.row_count[*selected_column]; offset++) {
                    const int new_row = *selected_row + offset;

                    const bool row_exists = new_row >= 0 && new_row < ui_state_.row_count[*selected_column];
                    if (!row_exists) continue;

                    const bool column_exists = ui_state_.column_count[new_row] > 0 && ui_state_.column_count[new_row] > *selected_column + 1;
                    if (!column_exists) continue;

                    ++*selected_column;
                    *selected_row = new_row;
                    break;
                }
            }
            break;

        case 'U':
            if (can_move_cursor_up) {
                --*selected_row;
            } else {
                *selected_row = ui_state_.row_count[*selected_column] - 1; // Loop around
            }
            break;

        case 'D':
            if (can_move_cursor_down) {
                ++*selected_row;
            } else {
                *selected_row = 0; // Loop around
            }
            break;

        default:
            break;
    }

    assert_msg(*selected_column < ui_max_columns_ && *selected_column >= 0, "Selected column is out of bounds");
    assert_msg(*selected_row < ui_max_rows_ && *selected_row >= 0, "Selected row is out of bounds");

    play_sfx_menu_move();
}

static bool is_button_selected(uibutton_t button) {
    return button.column == ui_state_.selected.column && button.row == ui_state_.selected.row && button.layer == ui_state_.selected.layer;
}

bool are_arrows_selected(uiarrows_t arrows) {
    return arrows.column == ui_state_.selected.column && arrows.row == ui_state_.selected.row && arrows.layer == ui_state_.selected.layer;
}

void draw_rotating_sun(Vector2 anchor_pos) {
    static float angle = 0.0f;
    angle              = fmodf(angle + 0.75f, 360.0f);

    const Rectangle rect_1 = { anchor_pos.x, anchor_pos.y, 26.25, 26.25 };
    const Rectangle rect_2 = { anchor_pos.x, anchor_pos.y, 22.5, 22.5 };

    DrawRectanglePro(rect_1, (Vector2){ 13.125, 13.125 }, angle, BLACK);
    DrawRectanglePro(rect_1, (Vector2){ 13.125, 13.125 }, angle - 45, BLACK);
    DrawRectanglePro(rect_2, (Vector2){ 11.25, 11.25 }, angle, YELLOW);
    DrawRectanglePro(rect_2, (Vector2){ 11.25, 11.25 }, angle - 45, YELLOW);
}

bool do_button(uibutton_t button, bool is_active) {
    const bool  is_selected  = is_button_selected(button);
    const bool  is_pressed   = is_selected && IsGamepadButtonReleased(0, BUTTON_A);
    const Color button_color = is_selected ? ui_button_selected_color_ : ui_button_color_;
    Color       text_color   = is_selected ? BLACK : RAYWHITE;

    if (!is_active) text_color = (Color){ 30, 30, 30, 150 };

    DrawRectangleV(button.pos, button.size, button_color);
    DrawRectangleLinesV(button.pos, button.size, text_color);

    if (is_selected) draw_rotating_sun(button.pos);

    const int text_x = (int)(button.pos.x + button.size.x / 2 - MeasureText(button.text, 20) / 2);
    const int text_y = (int)(button.pos.y + button.size.y / 2 - 10);
    DrawText(button.text, text_x, text_y, 20, text_color);

    if (!is_active) {
        if (is_pressed) play_sfx_menu_error();
        return false;
    }

    if (is_pressed) play_sfx_menu_select();

    return is_pressed;
}

int do_arrows(uiarrows_t arrows) {
    const bool  is_selected      = are_arrows_selected(arrows);
    const bool  is_right_pressed = IsGamepadButtonReleased(0, DPAD_RIGHT);
    const bool  is_left_pressed  = IsGamepadButtonReleased(0, DPAD_LEFT);
    const Color arrow_color      = is_selected ? ui_button_selected_color_ : ui_button_color_;
    const Color border_color     = BLACK;

    // Left arrow
    const Vector2 left_v1 = { .x = arrows.pos_left.x + arrows.size.x / 2, .y = arrows.pos_left.y };
    const Vector2 left_v2 = { .x = arrows.pos_left.x, .y = arrows.pos_left.y + arrows.size.y / 2 };
    const Vector2 left_v3 = { .x = arrows.pos_left.x + arrows.size.x / 2, .y = arrows.pos_left.y + arrows.size.y };

    DrawTriangle(left_v1, left_v2, left_v3, arrow_color);
    DrawTriangleLines(left_v1, left_v2, left_v3, border_color);

    // Right arrow
    const Vector2 right_v1 = { .x = arrows.pos_right.x + arrows.size.x, .y = arrows.pos_right.y + arrows.size.y / 2 };
    const Vector2 right_v2 = { .x = arrows.pos_right.x + arrows.size.x / 2, .y = arrows.pos_right.y };
    const Vector2 right_v3 = { .x = arrows.pos_right.x + arrows.size.x / 2, .y = arrows.pos_right.y + arrows.size.y };

    DrawTriangle(right_v1, right_v2, right_v3, arrow_color);
    DrawTriangleLines(right_v1, right_v2, right_v3, border_color);

    if (is_right_pressed) return 1;

    if (is_left_pressed) return -1;

    return 0;
}

void draw_confirmation_window(void (*callback)(int option, void *user_data), void *user_data, const char *message) {
    if (ui_state_.selected.layer == 0) return;

    const Vector2 conf_window_size = { SCREEN_WIDTH * 0.6f, SCREEN_HEIGHT * 0.5f };
    const Vector2 conf_window_pos  = { (SCREEN_WIDTH - conf_window_size.x) / 2.0f, (SCREEN_HEIGHT - conf_window_size.y) / 2.0f };

    DrawRectangleV(conf_window_pos, conf_window_size, ui_background_color_);

    const Vector2 button_size = { conf_window_size.x * 0.4f, conf_window_size.y * 0.25f };

    const uibutton_t yes_button = {
        .pos = { conf_window_pos.x + 30, conf_window_pos.y + conf_window_size.y * 0.5f },
          .size = button_size, .column = 0, .row = 0, .layer = 1, .text = "Yes"
    };

    const uibutton_t no_button = {
        .pos = { conf_window_pos.x + conf_window_size.x - button_size.x - 30, conf_window_pos.y + conf_window_size.y * 0.5f },
          .size = button_size, .column = 1, .row = 0, .layer = 1, .text = "No"
    };

    const int message_width = MeasureText(message, 20);
    const int message_x     = (int)(conf_window_pos.x + conf_window_size.x / 2 - message_width / 2);
    const int message_y     = (int)(conf_window_pos.y + conf_window_size.y * 0.25f - 10);
    DrawText(message, message_x, message_y, 20, RAYWHITE);

    static bool first_a_release = true; // We ignore the first release of A as it is released on the first frame
                                        // (since you need to release A to open this menu)

    if (first_a_release && IsGamepadButtonReleased(0, BUTTON_A)) {
        first_a_release = false;
        return;
    }

    if (do_button(yes_button, true)) {
        if (callback) callback(true, user_data);
        reset_cursor();
        first_a_release = true;
        return;
    }

    if (do_button(no_button, true)) {
        if (callback) callback(false, user_data);
        reset_cursor();
        first_a_release = true;
        return;
    }
}

uint8_t get_selected_column(void) {
    return ui_state_.selected.column;
}

void set_selected_column(uint8_t column) {
    assert_msg(column < ui_max_columns_ && column >= 0, "Column passed to set_selected_column is out of bounds");
    ui_state_.selected.column = column;
}

uint8_t get_selected_row(void) {
    return ui_state_.selected.row;
}

void set_selected_row(uint8_t row) {
    assert_msg(row < ui_max_rows_ && row >= 0, "Row passed to set_selected_row is out of bounds");
    ui_state_.selected.row = row;
}

uint8_t get_selected_layer(void) {
    return ui_state_.selected.layer;
}

void set_selected_layer(uint8_t layer) {
    assert_msg(layer == 0 || layer == 1, "Layer passed to set_selected_layer is out of bounds");
    ui_state_.selected.layer = layer;
}

void reset_cursor(void) {
    ui_state_.selected.layer  = 0;
    ui_state_.selected.column = 0;
    ui_state_.selected.row    = 0;
}

void reset_column_count(void) {
    for (int i = 0; i < ui_max_rows_; i++) {
        ui_state_.column_count[i] = 0;
    }
}

void reset_row_count(void) {
    for (int i = 0; i < ui_max_columns_; i++) {
        ui_state_.row_count[i] = 0;
    }
}

uint8_t get_column_count(uint8_t row) {
    assert_msg(row < ui_max_rows_ && row >= 0, "Row passed to get_column_count is out of bounds");
    return ui_state_.column_count[row];
}

void set_column_count(uint8_t row, uint8_t count) {
    assert_msg(row < ui_max_rows_ && row >= 0, "Row passed to set_column_count is out of bounds");
    assert_msg(count >= 0, "Count passed to set_column_count is negative");
    assert_msg(count <= ui_max_columns_, "Count passed to set_column_count is too large");
    ui_state_.column_count[row] = count;
}

uint8_t get_row_count(uint8_t column) {
    assert_msg(column < ui_max_columns_ && column >= 0, "Column passed to get_row_count is out of bounds");
    return ui_state_.row_count[column];
}

void set_row_count(uint8_t column, uint8_t count) {
    assert_msg(column < ui_max_columns_ && column >= 0, "Column passed to set_row_count is out of bounds");
    assert_msg(count >= 0, "Count passed to set_row_count is negative");
    assert_msg(count <= ui_max_rows_, "Count passed to set_row_count is too large");
    ui_state_.row_count[column] = count;
}

Color get_background_color(void) {
    return ui_background_color_;
}

Color get_button_color(void) {
    return ui_button_color_;
}

Color get_button_selected_color(void) {
    return ui_button_selected_color_;
}