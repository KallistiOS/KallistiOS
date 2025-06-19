/* KallistiOS ##version##
   examples/dreamcast/raylib/raytris/src/raytris.cpp
   Copyright (C) 2024 Cole Hall
*/

#include <raylib.h>
#include "game/game.h"
#include "constants/constants.h"
#include "colors/colors.h"
#include <iostream>
#include <string>
#include <iostream> 

#include <kos/init.h>
#include <kos/dbgio.h>
#include <dc/sound/stream.h>
#include <dc/maple.h>
#include <dc/maple/controller.h>
#include <wav/sndwav.h>

static double lastUpdateTime = 0;

static bool EventTriggered(double interval){
    double currentTime = GetTime();
    if(currentTime - lastUpdateTime >= interval){
        lastUpdateTime = currentTime;
        return true;
    }
    return false;
}

int main(int argc, char* argv[]){
    const int screenWidth = 640;
    const int screenHeight = 480;
    wav_stream_hnd_t bgm;

    InitWindow(screenWidth, screenHeight, "Block stacking puzzle game in KOS!");
    SetTargetFPS(60);

    snd_stream_init();
    wav_init();

    bgm = wav_create("/rd/assets/sound/bgm.adpcm", 1);
    wav_volume(bgm, 255);
    wav_play(bgm);

    Game game = Game();

    int TextUIDistance = Constants::gridWidthWithOffset + UIPadding::large;
    int scorePaddingHeight = UIPadding::medium;
    int scoreBoxPaddingHeight = scorePaddingHeight + 15 + UIPadding::small;
    int nextPaddingHeight = scoreBoxPaddingHeight + 60 + UIPadding::large;
    int nextBoxPaddingHeight = nextPaddingHeight + UIFont::medium + UIPadding::small;
    int gameOverPaddingHeight = nextBoxPaddingHeight + 180 + UIPadding::large;

    while(game.Running()){
        game.HandleInput();

        if(EventTriggered(0.2)){
            game.MoveBlockDown();
        }
        BeginDrawing();
        ClearBackground(darkBlue);
        game.Draw();
        DrawText("Hold", Constants::gridOffset - UIPadding::medium * 4, nextPaddingHeight, UIFont::medium, WHITE);
        DrawRectangleRounded({UIPadding::medium, (float)nextBoxPaddingHeight, Constants::gridOffset - UIPadding::large, 170}, 0.3, 6, lightBlue);

        DrawText("Score", TextUIDistance, scorePaddingHeight, UIFont::medium, WHITE);
        DrawRectangleRounded({Constants::gridWidthWithOffset + UIPadding::medium, (float)scoreBoxPaddingHeight, 170, 60}, 0.3, 6, lightBlue);

        std::string scoreText = std::to_string(game.score);
        Vector2 textSize = MeasureTextEx(GetFontDefault(), scoreText.c_str(), UIFont::medium, 0);

        DrawText(scoreText.c_str(), TextUIDistance + (170 - textSize.x)/2, scoreBoxPaddingHeight + UIPadding::medium, UIFont::medium, WHITE);

        DrawText("Next", TextUIDistance,  nextPaddingHeight, UIFont::medium, WHITE);
        DrawRectangleRounded({Constants::gridWidthWithOffset + UIPadding::medium, (float)nextBoxPaddingHeight, 170, 180}, 0.3, 6, lightBlue);
        if(game.gameOver){
            DrawText("GAME OVER\nPress start!", TextUIDistance, gameOverPaddingHeight, UIFont::medium, WHITE);
        }
        game.DrawNext(TextUIDistance - 20, nextBoxPaddingHeight + UIPadding::large * 1.5);

        game.DrawHeld(-20, nextBoxPaddingHeight + UIPadding::large * 1.5);
        EndDrawing();
    }

    std::cout << "Finishing - Cleaning up\n";  
    wav_stop(bgm);
    wav_shutdown();
    snd_stream_shutdown();
    std::cout << "Finished - Cleaning up\n";  

    return 0;
}