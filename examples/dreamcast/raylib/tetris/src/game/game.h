/* KallistiOS ##version##

   examples/dreamcast/raylib/tetris/src/game/game.h
   Copyright (C) 2024 Cole Hall

*/

#pragma once
#include "../grid/grid.h"
#include "../blocks/blocks.cpp"
#include "../sound/soundManager.h"
#include "../vmu/vmuManager.h"
#include <kos.h>

class Game{
    public:
        Game();
        void Draw();
        void DrawBlockAtPosition(Block &block, int offsetX, int offsetY, int offsetXAdjustment, int offsetYAdjustment);
        void DrawHeld(int offsetX, int offsetY);
        void HandleInput();
        void MoveBlockDown();
        void DrawNext(int offsetX, int offsetY);
        bool Running();
        bool gameOver;
        int score;

    private:
        double lastHeldMoveTime;
        double floorContactTime = 0;
        double timeSinceLastRotation;
        bool IsBlockOutside();
        bool canHoldBlock = true;
        void RotateBlock(bool clockwise);
        void LockBlock();
        bool BlockFits();
        void Reset();
        void UpdateScore(int linesCleared, int moveDownPoints);
        void MoveBlockLeft();
        void MoveBlockRight();
        void HardDrop();
        void HoldBlock();
        std::vector<Block> GetAllBlocks();
        std::vector<Block> blocks;
        Block currentBlock;
        Block heldBlock = NullBlock();
        Block nextBlock;
        Block GetRandomBlock();
        Grid grid;
        uint16_t prev_buttons;
        uint16_t prev_triggers;
        bool running = true;
        const double moveThreshold = 0.075;
        const double timerGraceBig = 2;
        const double timerGraceSmall = 0.4;
        static const int moves[15][2];
        SoundManager soundManager = SoundManager();
        VmuManager vmuManager = VmuManager();
};
