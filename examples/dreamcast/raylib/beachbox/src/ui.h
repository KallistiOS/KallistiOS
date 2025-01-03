/* KallistiOS ##version##
   examples/dreamcast/raylib/beachbox/src/ui.h
   Copyright (C) 2024 Agustín Bellagamba
   Copyright (C) 2024 Cypress
*/

#ifndef BBOX_UI_H
#define BBOX_UI_H

#include <raylib.h>
#include <stdint.h>

typedef struct UIButton {
        const Vector2 pos;
        const Vector2 size;
        const uint8_t column;
        const uint8_t row;
        const uint8_t layer;
        const char   *text;
} uibutton_t;

typedef struct UIArrows {
        const Vector2 size;
        const Vector2 pos_left;
        const Vector2 pos_right;
        const uint8_t column;
        const uint8_t row;
        const uint8_t layer;
} uiarrows_t;

struct UiSelected {
        uint8_t column;
        uint8_t row;
        uint8_t layer;
};

static constexpr uint8_t ui_max_columns_ = 10;
static constexpr uint8_t ui_max_rows_    = 10;

typedef struct UiState {
        struct UiSelected selected;
        uint8_t           row_count[ui_max_columns_];
        uint8_t           column_count[ui_max_rows_];
} ui_state_t;

// Moves the "cursor" when the player presses the D-Pad, changing the selected UI element
// Direction is one of 'L', 'R', 'U', 'D' for left, right, up, down
void move_cursor(char direction);

// Draws a rotating sun around the selected UIButton
void draw_rotating_sun(Vector2 anchor_pos);

// Draws an UIButton. Returns true if the button is pressed
bool do_button(uibutton_t button, bool is_active);

// Draws UIArrows. Returns 1 if right is pressed, -1 if left is pressed, 0 otherwise
int do_arrows(uiarrows_t arrows);

// Takes a callback function, parameters to pass to the function and a message to display
void draw_confirmation_window(void (*callback)(int option, void *user_data), void *user_data, const char *message);

bool are_arrows_selected(uiarrows_t arrows);

// Returns the selected column
uint8_t get_selected_column(void);

// Sets the selected column to the given value
void set_selected_column(uint8_t column);

// Returns the selected row
uint8_t get_selected_row(void);

// Sets the selected row to the given value
void set_selected_row(uint8_t row);

// Returns the selected layer
uint8_t get_selected_layer(void);

// Sets the selected layer to the given value
void set_selected_layer(uint8_t layer);

// Returns the column count for the given row
uint8_t get_column_count(const uint8_t row);

// Sets the given row's column count to the given value
void set_column_count(uint8_t row, uint8_t count);

// Returns the row count for the given column
uint8_t get_row_count(uint8_t column);

// Sets the given column's row count to the given value
void set_row_count(uint8_t column, uint8_t count);

// Resets the cursor position to layer 0, column 0, row 0
void reset_cursor(void);

// Resets the column count to 0
void reset_column_count(void);

// Resets the row count to 0
void reset_row_count(void);

// Returns the background color
Color get_background_color(void);

// Returns the button color
Color get_button_color(void);

// Returns the selected button color
Color get_button_selected_color(void);

#endif