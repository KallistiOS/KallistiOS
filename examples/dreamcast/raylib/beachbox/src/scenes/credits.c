/* KallistiOS ##version##
   examples/dreamcast/raylib/beachbox/src/scenes/credits.c
   Copyright (C) 2024 Agustín Bellagamba
   Copyright (C) 2024 Cypress
*/

#include "credits.h"
#include "../background.h"
#include "../config.h"
#include "../ui.h"
#include "../scene.h"

static const uibutton_t exit_button_ = {
    .pos    = { .x = 43,  .y = 370 },
    .size   = { .x = 150, .y = 40  },
    .column = 0,
    .row    = 0,
    .layer  = 0,
    .text   = "Return",
};

static struct BouncingImage {
        Vector2   position;
        Vector2   speed;
        Texture2D texture;
} images[3];

static void initialize_images(void) {
    const char *paths[] = { "/rd/logos/koslogo.png", "/rd/logos/rayliblogo.png", "/rd/logos/psyoplogo.png" };

    constexpr Vector2 positions[] = {
        { 50,  50  },
        { 490, 250 },
        { 240, 350 }
    };

    for (int i = 0; i < 3; i++) {
        images[i].speed    = (Vector2){ 4, 3 };
        images[i].texture  = LoadTexture(paths[i]);
        images[i].position = positions[i];
    }
}

static void unload_credits_images(void) {
    for (int i = 0; i < 3; i++) {
        UnloadTexture(images[i].texture);
    }
}

void init_credits_scene(void) {
    set_row_count(0, 1);
    set_column_count(0, 1);
    initialize_images();
}

void update_credits_scene(void) {
    for (int i = 0; i < 3; i++) {
        images[i].position.x += images[i].speed.x;
        images[i].position.y += images[i].speed.y;

        if (images[i].position.x <= 0 || images[i].position.x + images[i].texture.width >= SCREEN_WIDTH) images[i].speed.x *= -1;
        if (images[i].position.y <= 0 || images[i].position.y + images[i].texture.height >= SCREEN_HEIGHT) images[i].speed.y *= -1;
    }
}

void draw_credits_scene(void) {
    draw_background();

    for (int i = 0; i < 3; i++) {
        DrawTexture(images[i].texture, (int)images[i].position.x, (int)images[i].position.y, WHITE);
    }

    DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, get_background_color());

    // TODO: add actual credits

    const char *credits_text  = "Made by Psyop Studios";
    const char *credits_text2 = "Music and SFX by Creepypastorius";
    const char *credits_text3 = "Credits song by mickschen";
    const char *credits_text4 = "Thanks to KallistiOS and raylib";
    DrawText(credits_text, (int)(SCREEN_WIDTH / 2) - MeasureText(credits_text, 24) / 2, (int)(SCREEN_HEIGHT / 4) + 20, 24, RAYWHITE);
    DrawText(credits_text2, (int)(SCREEN_WIDTH / 2) - MeasureText(credits_text2, 24) / 2, (int)(SCREEN_HEIGHT / 4) + 65, 24, RAYWHITE);
    DrawText(credits_text3, (int)(SCREEN_WIDTH / 2) - MeasureText(credits_text3, 24) / 2, (int)(SCREEN_HEIGHT / 4) + 95, 24, RAYWHITE);
    DrawText(credits_text4, (int)(SCREEN_WIDTH / 2) - MeasureText(credits_text4, 24) / 2, (int)(SCREEN_HEIGHT / 4) + 135, 24, RAYWHITE);

    if (do_button(exit_button_, true)) {
        unload_credits_images();
        change_scene(MAINMENU);
    }
}
