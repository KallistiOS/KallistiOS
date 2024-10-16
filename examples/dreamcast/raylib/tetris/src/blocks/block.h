/* KallistiOS ##version##

   examples/dreamcast/raylib/tetris/src/blocks/block.h
   Copyright (C) 2024 Cole Hall

*/

#pragma once

#include <vector>
#include <map>
#include "../position/position.h"
#include "../colors/colors.h"

class Block{

    public:
        Block();
        void Draw(int offsetX, int offsetY);
        void Move(int rows, int columns);
        void Reset();
        std::vector<Position> GetCellPositions();
        void Rotate();
        void UndoRotation();
        int id;
        std::map<int, std::vector<Position>> cells;
        const char* vmuIcon;

    private:
        int cellSize;
        int rotationState;
        std::vector<Color> colors;
        int rowOffset;
        int columnOffset;
};
