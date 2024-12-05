/* KallistiOS ##version##
   examples/dreamcast/raylib/beachbox/src/player.h
   Copyright (C) 2024 Agustín Bellagamba
   Copyright (C) 2024 Cypress
*/

#ifndef BBOX_PLAYER_H
#define BBOX_PLAYER_

#include <raylib.h>
#include "timer.h"

typedef struct Character {
        Vector2 size;
        Vector2 pos;
        Vector2 velocity;
        float   speed;
        float   max_meter;
        float   meter;
        bool    is_shifted;
        Color   color;
} character_t;

// Initializes the player
void init_player(void);

// Updates the player
void update_player(void);

// Draws the player
void draw_player(void);

// Moves the player towards the given direction
void move_player(Vector2 direction);

// Sets the player's 'dimension' to the given value
// False being normal, true being shifted
void shift_player(bool should_shift);

// Makes the player jump
void jump(void);

// This function gets called when the player lets go of the jump button, cutting the jump short
void cut_jump(void);

// Toggles the slowdown power on / off (if it's available)
void toggle_slowdown(void);

// Turns the slowdown power off
void turn_slowdown_off(void);

// Teleports the player forward (if it's available)
void teleport(void);

// Returns true if the slowdown power is active
bool is_slowdown_active(void);

// Returns true if the teleport power is active
bool is_player_teleporting(void);

// Returns true if the player is shifted
bool is_player_shifted(void);

// Returns a Rectangle representing the player's position and size
Rectangle get_player_rect(void);

// Returns the player's current meter
float get_player_meter(void);

// Returns the player's max meter
float get_player_max_meter(void);

// Adds the given amount to the player's meter
void refill_player_meter(float amount);

// Returns the teleport cooldown timer
bbox_timer_t get_teleport_cooldown_timer(void);

// Returns the color at the given index on the player colors array
Color get_player_color(int index);

// Returns the size of the player colors array
int get_player_color_count(void);

#endif