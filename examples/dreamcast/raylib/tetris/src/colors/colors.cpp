/* KallistiOS ##version##

   examples/dreamcast/raylib/tetris/src/colors/colors.cpp
   Copyright (C) 2024 Cole Hall

*/

#include "colors.h"

const Color darkGrey = {26, 31, 40, 255};
const Color green = {47, 230, 23, 255};
const Color red = {232, 18, 18, 255};
const Color orange = {226, 116, 17, 255};
const Color yellow = {237, 234, 4, 255};
const Color purple = {166, 0, 247, 255};
const Color cyan = {21, 204, 209, 255};
const Color blue = {0, 64, 216, 255};
const Color lightBlue = {47, 109, 122, 255};
const Color darkBlue = {44, 44, 127, 255};

std::vector<Color> GetCellColors(){
    return {darkGrey, green, red, orange, yellow, purple, cyan, blue};
}
