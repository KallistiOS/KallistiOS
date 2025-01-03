/* KallistiOS ##version##
   examples/dreamcast/raylib/beachbox/src/hats.h
   Copyright (C) 2024 Agustín Bellagamba
   Copyright (C) 2024 Cypress
*/

#ifndef BBOX_HATS_H
#define BBOX_HATS_H

#include <stdint.h>
#include <raylib.h>

typedef enum Hats : uint8_t {
    HAT_NIL,
    HAT_SLIME_RED,
    HAT_SLIME_BLUE,
    HAT_BOX,
    HAT_M,
    HAT_L,
    HAT_Z,
    HAT_F,
    HAT_MUPRH,
    HAT_CROWN,
    HAT_COUNT,
} hat_t;

typedef struct UnlockedHats {
        bool nil : 1;
        bool slime_red : 1;
        bool slime_blue : 1;
        bool box : 1;
        bool m : 1;
        bool l : 1;
        bool z : 1;
        bool f : 1;
        bool muprh : 1;
        bool crown : 1;
} unlocked_hats_t;

// Loads the hat textures into memory
void load_hats(void);

// Unloads the hat textures from memory
void unload_hats(void);

// Returns the texture for the given hat
const Texture2D *get_hat_texture(hat_t hat);

#endif // BBOX_HATS_H