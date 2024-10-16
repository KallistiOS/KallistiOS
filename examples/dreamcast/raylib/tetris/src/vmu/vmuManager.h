/* KallistiOS ##version##

   examples/dreamcast/raylib/tetris/src/vmu/vmuManager.h
   Copyright (C) 2024 Cole Hall

*/

#pragma once

#include <dc/maple/vmu.h>
#include <dc/maple.h>

class VmuManager {
public:
    VmuManager();
    void displayImage(const char *xmp);
    void resetImage();
};
