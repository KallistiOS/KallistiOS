/* KallistiOS ##version##

   examples/dreamcast/raylib/tetris/src/constants/constants.h
   Copyright (C) 2024 Cole Hall

*/

#pragma once

namespace Constants {
    constexpr int gridOffset = 200;
    constexpr int cellSize = 23;
    constexpr int numRows = 20;
    constexpr int numCols = 10;

    constexpr int gridWidth = cellSize * numCols;
    constexpr int gridHeight = cellSize * numRows; 

    constexpr int gridWidthWithOffset = gridOffset + gridWidth;
}

namespace UIPadding{
    constexpr int tiny = 6;
    constexpr int small = 12;
    constexpr int medium = 24;
    constexpr int large = 36;
}

namespace UIFont{
    constexpr int tiny = 8;
    constexpr int small = 16;
    constexpr int medium = 24;
    constexpr int large = 32;
    constexpr int xlarge = 40;
}
