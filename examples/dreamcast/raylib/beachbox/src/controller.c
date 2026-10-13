/* KallistiOS ##version##
   examples/dreamcast/raylib/beachbox/src/controller.c
   Copyright (C) 2024 Agustín Bellagamba
   Copyright (C) 2024 Cypress
*/

#include <raylib.h>

#include "controller.h"
#include "scene.h"
#include "ui.h"
#include "player.h"
#include "scenes/game.h"

// NOTE: This is in PascalCase to follow raylib naming conventions
static bool IsTriggerPressed(int gamepad, trigger_t trigger) {
    static bool     is_left_trigger_down;
    static bool     is_right_trigger_down;
    constexpr float trigger_threshold = 0.5f;

    bool *is_trigger_down  = (trigger == LEFT_TRIGGER) ? &is_left_trigger_down : &is_right_trigger_down;
    float trigger_movement = GetGamepadAxisMovement(0, trigger);

    if (trigger_movement > trigger_threshold && !*is_trigger_down) {
        *is_trigger_down = true;
        return true;
    }

    if (trigger_movement <= trigger_threshold && *is_trigger_down) {
        *is_trigger_down = false;
    }

    return false;
}

void update_controller(void) {
    if (!IsGamepadAvailable(0)) return;

    // pressed
    const bool is_left_pressed      = IsGamepadButtonPressed(0, DPAD_LEFT);
    const bool is_right_pressed     = IsGamepadButtonPressed(0, DPAD_RIGHT);
    const bool is_up_pressed        = IsGamepadButtonPressed(0, DPAD_UP);
    const bool is_down_pressed      = IsGamepadButtonPressed(0, DPAD_DOWN);
    const bool is_l_trigger_pressed = IsTriggerPressed(0, LEFT_TRIGGER);
    const bool is_r_trigger_pressed = IsTriggerPressed(0, RIGHT_TRIGGER);
    const bool is_start_pressed     = IsGamepadButtonPressed(0, BUTTON_START);

    // released
    const bool is_a_released = IsGamepadButtonReleased(0, BUTTON_A);

    // down
    const bool is_left_down  = IsGamepadButtonDown(0, DPAD_LEFT);
    const bool is_right_down = IsGamepadButtonDown(0, DPAD_RIGHT);
    const bool is_a_down     = IsGamepadButtonDown(0, BUTTON_A);
    const bool is_x_down     = IsGamepadButtonDown(0, BUTTON_X);

    switch (get_current_scene()) {
        case GAME:
            if (is_game_over()) {
                if (is_left_pressed) {
                    move_cursor('L');
                } else if (is_right_pressed) {
                    move_cursor('R');
                }
                break;
            }

            if (is_start_pressed) {
                toggle_pause();
            }

            if (is_game_paused()) break;

            const float axis_movement_left_x = GetGamepadAxisMovement(0, GAMEPAD_AXIS_LEFT_X);
            if (axis_movement_left_x != 0) {
                move_player((Vector2){ .x = axis_movement_left_x, .y = 0 });
            } else {
                if (is_left_down) {
                    move_player((Vector2){ .x = -1, .y = 0 });
                }
                if (is_right_down) {
                    move_player((Vector2){ .x = 1, .y = 0 });
                }
            }

            if (is_a_down) {
                jump();
            } else if (is_a_released) {
                cut_jump();
            }

            if (is_l_trigger_pressed) {
                toggle_slowdown();
            }
            if (is_r_trigger_pressed) {
                teleport();
            }

            shift_player(is_x_down);
            break;

        case RAYLOGO:
            if (is_start_pressed) {
                change_scene(LOADING);
            }

        case LOADING:
        case MAINMENU:
        case SHOP:
        case UNLOCKABLES:
        case OPTIONS:
        case CREDITS:
            if (is_left_pressed) {
                move_cursor('L');
            } else if (is_right_pressed) {
                move_cursor('R');
            } else if (is_up_pressed) {
                move_cursor('U');
            } else if (is_down_pressed) {
                move_cursor('D');
            }
            break;

        default:
            break;
    }
}