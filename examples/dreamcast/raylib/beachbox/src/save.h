/* KallistiOS ##version##
   examples/dreamcast/raylib/beachbox/src/save.h
   Copyright (C) 2024 Agustín Bellagamba
   Copyright (C) 2024 Cypress
*/

#ifndef BBOX_SAVE_H
#define BBOX_SAVE_H

#include <stdint.h>
#include "hats.h"
#include "upgrades.h"

// Returns 1 on success, 0 on not enough space, -1 on no VMU found, -2 on unknown error
int save_game(void);

// Returns 1 on success, 0 on no savefile found, -1 on no VMU found
int load_game(void);

// Resets the save data, but doesn't save it
void new_game(void);

// Saves the game asynchronously
// NOTE: This function doesn't have a return code, because it sets off a timer
//       Which makes draw_save_popup display the save status
void save_game_async(void);

// Updates the save game timer used for saving asynchronously
void update_save_game_timer(void);

// Draws a save status window on the bottom right corner
// NOTE: This is only drawn if needed
void draw_save_popup(void);

// Returns true if a save is in progress
bool is_save_in_progress(void);

// Returns true if a load is in progress
bool is_load_in_progress(void);

// Returns the number of coins the player has
uint16_t get_total_coins(void);

// Add n coins to the player's total
void add_coins(uint16_t n);

// Returns the number of runs the player has played
uint16_t get_total_runs(void);

// Add a run to the player's total
void increment_total_runs(void);

// Returns the player's high score
uint16_t get_high_score(void);

// Sets the player's high score
void set_high_score(uint16_t new_high_score);

// Returns the level of the given upgrade
uint8_t get_upgrade_level(player_upgrade_t upgrade);

// Increments the level of the given upgrade
void increment_upgrade_level(player_upgrade_t upgrade);

// Returns the max level of the given upgrade
uint8_t get_max_upgrade_level(player_upgrade_t upgrade);

// Returns the type of hat the player is currently wearing
hat_t get_current_hat_type(void);

// Increments the player's chosen hat index
// NOTE: This function wraps around when the index has reached the end of the array
void increment_current_hat_index(void);

// Decrements the player's chosen hat index
// NOTE: This function wraps around when the index has reached the start of the array
void decrement_current_hat_index(void);

// Returns true if the given hat is unlocked
bool is_hat_unlocked(hat_t);

// Unlocks the given hat
void unlock_hat(hat_t hat);

// Returns the player's chosen color index
uint8_t get_player_current_color_index(void);

// Increments the player's chosen color index
// NOTE: This function wraps around when the index has reached the end of the array
void increment_player_color_index(void);

// Decrements the player's chosen color index
// NOTE: This function wraps around when the index has reached the start of the array
void decrement_player_color_index(void);

// Returns the saved music volume
uint8_t get_saved_music_volume(void);

// Returns the saved sfx volume
uint8_t get_saved_sfx_volume(void);

typedef struct __attribute__((__packed__)) Save {
        // HEADER //
        // Text fields are padded with space ($20). String fileds are padded with NUL ($00).
        // NOTE: These MUST be in this specific order
        struct {
                char    vms_menu_description[16]; // Description of file (shown in VMS file menu)
                char    dc_description[32];       // Description of file (shown in DC boot ROM file manager)
                char    app_identifier[16];       // Identifier of application that created the file (STRING)
                int16_t number_icons;             // Number of icons (>1 for animated icons)
                int16_t icon_animation_speed;     // Icon animation speed

                /*
                Graphic eyecatch type
                        0 = none

                        1 = 16-bit true colour | 8064 bytes total

                        2 = 256 colour palette | 4544 bytes total |
                        (same format as icon palette, but 256 entries. the bitmap is 1 byte per pixel instead)

                        3 = 16 colour palette  | 2048 bytes total | same as icon palette
                */
                int16_t graphic_eyecatch_type;

                int16_t crc;                   // CRC (for error checking)
                int32_t bytes_after_header;    // Number of bytes of actual file data following header, icon(s) and eyecatch.
                char    header_reserved[20];   // Reserved (fill with zeros)
                int16_t icon_palette[16];      // Icon palette (16 16-bit integers)
                uint8_t icon_bitmaps[512 * 3]; // Icon bitmaps (Nybbles) (512 bytes per frame)

                // We use the type 3 eyecatch
                int16_t eyecatch_palette[16];  // Graphic eyecatch palette (32 8-bit integers)
                uint8_t eyecatch_bitmap[2016]; // Graphic eyecatch bitmap (Nybbles) (2016 bytes, 72×56 image)
        } header;

        // GAME SAVE DATA //
        uint16_t                total_coins;
        uint16_t                total_runs;
        uint16_t                high_score;
        player_upgrade_levels_t upgrade_levels;
        uint8_t                 color_index; // Selected color from player_colors array
        uint8_t                 hat_index;   // Selected hat from hats array
        unlocked_hats_t         unlocked_hats;
        uint8_t                 music_volume;
        uint8_t                 sfx_volume;
        char                    reserved_data[369]; // Reserved for (potential) future use

} save_t;

#endif