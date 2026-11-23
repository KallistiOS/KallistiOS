/* KallistiOS ##version##
   examples/dreamcast/raylib/beachbox/src/scene.h
   Copyright (C) 2024 Agustín Bellagamba
   Copyright (C) 2024 Cypress
*/

#ifndef BBOX_SCENE_H
#define BBOX_SCENE_H

typedef enum Scene { RAYLOGO, LOADING, MAINMENU, GAME, SHOP, UNLOCKABLES, OPTIONS, CREDITS } scene_t;

// Changes the current scene to the given scene, initializing it and switching the song if needed
void change_scene(scene_t scene);

// Updates the current scene
void update_current_scene(void);

// Draws the current scene
void draw_current_scene(void);

// Returns the current scene
scene_t get_current_scene(void);

#endif