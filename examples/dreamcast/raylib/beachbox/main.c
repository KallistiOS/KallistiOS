/* KallistiOS ##version##
   examples/dreamcast/raylib/beachbox/main.c
   Copyright (C) 2024 Agustín Bellagamba
   Copyright (C) 2024 Cypress
*/

#include <raylib.h>
#include <kos/thread.h>

#include "src/audio.h"
#include "src/config.h"
#include "src/scene.h"
#include "src/save.h"
#include "src/controller.h"
#include "src/player.h"
#include "src/hats.h"
#include "src/vmu.h"
#include "src/ui.h"

static void init_game(void) {
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "BeachBox");
    init_audio();
    load_hats();
    change_scene(RAYLOGO);
    thd_create(true, draw_vmu_animation, nullptr);
}

static void update_game(void) {
    update_controller();
    update_current_scene();
}

static void draw_game(void) {
    BeginDrawing();
    draw_current_scene();

#ifdef DEBUG_DRAW_FPS
    DrawRectangle(0, 440, 130, 50, (Color){ 22, 22, 22, 200 });
    DrawFPS(27, 450);
#endif

#ifdef DEBUG_DRAW_VOLUME
    DrawRectangle(320, 20, 300, 60, (Color){ 22, 22, 22, 200 });
    DrawText(TextFormat("music: %d, sfx: %d", get_music_volume(), get_sfx_volume()), 395, 40, 20, RED);
#endif

#ifdef DEBUG_DRAW_CURSOR_INFO
    DrawRectangle(320, 20, 300, 120, (Color){ 22, 22, 22, 200 });
    DrawText(TextFormat("col: %d, row: %d", get_selected_column(), get_selected_row()), 395, 40, 20, RED);
    DrawText(TextFormat("col_count: %d, row_count: %d", get_column_count(get_selected_row()), get_row_count(get_selected_column())), 345, 80, 20, RED);
#endif

    EndDrawing();
}

int main(int argc, char **argv) {
    init_game();

    while (true) {
        update_game();
        draw_game();
    }

    unload_hats();
    deinit_audio();

    return 0;
}
