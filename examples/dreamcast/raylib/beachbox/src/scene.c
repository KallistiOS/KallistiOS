/* KallistiOS ##version##
   examples/dreamcast/raylib/beachbox/src/scene.c
   Copyright (C) 2024 Agustín Bellagamba
   Copyright (C) 2024 Cypress
*/

#include <kos/thread.h>
#include <stdatomic.h>

#include "audio.h"
#include "scene.h"
#include "ui.h"
#include "save.h"
#include "helper_functions.h"
#include "timer.h"
#include "vmu.h"
#include "scenes/raylogo.h"
#include "scenes/loading.h"
#include "scenes/options.h"
#include "scenes/mainmenu.h"
#include "scenes/game.h"
#include "scenes/credits.h"
#include "scenes/shop.h"
#include "scenes/unlockables.h"

static scene_t current_scene_ = RAYLOGO;

void change_scene(scene_t scene) {
    static const void (*init_scene_functions[])(void) = { [RAYLOGO] = init_raylogo_scene, [LOADING] = init_loading_scene,         [MAINMENU] = init_mainmenu_scene, [GAME] = init_game_scene,
                                                          [SHOP] = init_shop_scene,       [UNLOCKABLES] = init_unlockables_scene, [OPTIONS] = init_options_scene,   [CREDITS] = init_credits_scene };

    reset_column_count();
    reset_row_count();
    reset_cursor();
    reset_vmu_animation();

    const scene_t previous_scene = current_scene_;

    switch (scene) {
        case GAME:
            if (previous_scene == GAME && !is_save_in_progress()) {
                save_game_async();
            }
            break;
        case MAINMENU:
            if (previous_scene != LOADING && previous_scene != CREDITS && !is_save_in_progress()) {
                save_game_async();
            }
            break;
        default:
            break;
    }
    init_scene_functions[scene]();
    current_scene_ = scene;
    update_song();
}

void update_current_scene(void) {
    static const void (*update_scene_functions[])(void)
        = { [RAYLOGO] = update_raylogo_scene, [LOADING] = update_loading_scene,         [MAINMENU] = update_mainmenu_scene, [GAME] = update_game_scene,
            [SHOP] = update_shop_scene,       [UNLOCKABLES] = update_unlockables_scene, [OPTIONS] = update_options_scene,   [CREDITS] = update_credits_scene };
    update_scene_functions[current_scene_]();
    update_save_game_timer();
}

void draw_current_scene(void) {
    static const void (*draw_scene_functions[])(void) = { [RAYLOGO] = draw_raylogo_scene, [LOADING] = draw_loading_scene,         [MAINMENU] = draw_mainmenu_scene, [GAME] = draw_game_scene,
                                                          [SHOP] = draw_shop_scene,       [UNLOCKABLES] = draw_unlockables_scene, [OPTIONS] = draw_options_scene,   [CREDITS] = draw_credits_scene };
    draw_scene_functions[current_scene_]();
    draw_save_popup();
}

scene_t get_current_scene(void) {
    return current_scene_;
}