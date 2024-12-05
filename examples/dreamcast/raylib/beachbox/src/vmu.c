/* KallistiOS ##version##
   examples/dreamcast/raylib/beachbox/src/vmu.c
   Copyright (C) 2024 Agustín Bellagamba
   Copyright (C) 2024 Cypress
*/

#include <stdio.h>
#include <dc/maple.h>
#include <dc/vmu_fb.h>

#include "save.h"
#include "vmu.h"
#include "vmu_animations.h"
#include "timer.h"
#include "scene.h"
#include "scenes/game.h"

// Vmu animations are drawn to the vmu frame buffer, and then presented to the screen
// Each frame is just an array of raw bits, 1bpp (see vmu_animations.h)

static vmufb_t      fb_;
static uint8_t      current_frame_       = 0;
static uint8_t      mainmenu_text_frame_ = 0;
static bbox_timer_t mainmenu_text_update_cooldown_;
static int          credits_bar_width_       = 1;
static bool         credits_bar_should_grow_ = true;

static void update_vmu_menu_text(void) {
    update_timer(&mainmenu_text_update_cooldown_);

    char buffer[16];
    switch (mainmenu_text_frame_) {
        case 0:
            vmufb_print_string_into(&fb_, nullptr, 4, 1, 48, 6, 2, "Hello, and");
            vmufb_print_string_into(&fb_, nullptr, 8, 7, 48, 6, 2, "welcome!");
            break;
        case 1:
            vmufb_print_string_into(&fb_, nullptr, 2, 1, 48, 6, 2, "I hope that");
            vmufb_print_string_into(&fb_, nullptr, 5, 7, 48, 6, 2, "you enjoy ");
            break;
        case 2:
            vmufb_print_string_into(&fb_, nullptr, 5, 5, 48, 6, 2, "BeachBox!!");
            break;
        case 3:
            vmufb_print_string_into(&fb_, nullptr, 4, 1, 48, 6, 2, "Check out");
            vmufb_print_string_into(&fb_, nullptr, 8, 7, 48, 6, 2, "the shop-");
            break;
        case 4:
            vmufb_print_string_into(&fb_, nullptr, 4, 1, 48, 6, 2, "-and spend");
            vmufb_print_string_into(&fb_, nullptr, 3, 7, 48, 6, 2, "your money!");
            break;
        case 5:
            snprintf(buffer, sizeof(buffer), "You have %d", get_total_coins());
            vmufb_print_string_into(&fb_, nullptr, 1, 1, 48, 6, 2, buffer);
            vmufb_print_string_into(&fb_, nullptr, 3, 7, 48, 6, 2, "coins left");
            break;
        case 6:
            snprintf(buffer, sizeof(buffer), "%d times!!", get_total_runs());
            vmufb_print_string_into(&fb_, nullptr, 4, 1, 48, 6, 2, "You played");
            vmufb_print_string_into(&fb_, nullptr, 8, 7, 48, 6, 2, buffer);
            break;
        case 7:
            vmufb_print_string_into(&fb_, nullptr, 8, 5, 48, 6, 2, "Congrats!");
            break;
    }

    if (mainmenu_text_update_cooldown_.is_running) {
        return;
    }

    start_timer(&mainmenu_text_update_cooldown_, 0.15f);

    mainmenu_text_frame_ = (mainmenu_text_frame_ + 1) % 8;
}

static void update_vmu_credits_animation(void) {
    static const char data[192] = { 0 };

    credits_bar_width_ = (credits_bar_width_ + (credits_bar_should_grow_ ? 1 : -1)) % 49;
    if (credits_bar_width_ == 0 || credits_bar_width_ == 48) credits_bar_should_grow_ = !credits_bar_should_grow_;

    vmufb_paint_area(&fb_, 0, 0, credits_bar_width_, 32, data);
}

void reset_vmu_animation(void) {
    current_frame_       = 0;
    mainmenu_text_frame_ = 0;
    start_timer(&mainmenu_text_update_cooldown_, 0.3f);
    credits_bar_width_       = 1;
    credits_bar_should_grow_ = true;
}

#define SET_VMU_ANIMATION(animation)                                            \
    do {                                                                        \
        vmu_current_animation = (animation);                                    \
        vmu_current_num_frames = sizeof((animation)) / sizeof((animation)[0]);  \
    } while(false)

void *draw_vmu_animation(void *param) {
    while (true) {
        thd_sleep(250);
        if (is_save_in_progress() || is_load_in_progress()) continue;
        maple_device_t *vmu = maple_enum_type(0, MAPLE_FUNC_MEMCARD);
        if (!vmu) continue;

        const char  **vmu_current_animation  = nullptr;
        int           vmu_current_num_frames = 0;
        const scene_t current_scene          = get_current_scene();

        switch (current_scene) {
            case RAYLOGO:
                SET_VMU_ANIMATION(vmu_raylib_animation);
                break;
            case LOADING:
                SET_VMU_ANIMATION(vmu_loading_animation);
                break;
            case MAINMENU:
                SET_VMU_ANIMATION(vmu_face_animation);
                break;
            case GAME:
                if (is_game_over()) {
                    SET_VMU_ANIMATION(vmu_game_over_animation);
                } else {
                    SET_VMU_ANIMATION(vmu_game_scene_animation);
                }
                break;
            case SHOP:
                SET_VMU_ANIMATION(vmu_shop_animation);
                break;
            case UNLOCKABLES:
                SET_VMU_ANIMATION(vmu_unlockables_animation);
                break;
            case OPTIONS:
                SET_VMU_ANIMATION(vmu_options_animation);
                break;
            case CREDITS:
                SET_VMU_ANIMATION(vmu_credits_animation);
                break;
        }

        if (!vmu_current_animation) continue;

        current_frame_ = (current_frame_ + 1) % vmu_current_num_frames;
        vmufb_paint_area(&fb_, 0, 0, 48, 32, vmu_current_animation[current_frame_]);

        if (current_scene == MAINMENU) update_vmu_menu_text();
        if (current_scene == CREDITS) update_vmu_credits_animation();

        vmufb_present(&fb_, vmu);
    }

    return nullptr;
};

#undef SET_VMU_ANIMATION
