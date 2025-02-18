/* KallistiOS ##version##
   examples/dreamcast/raylib/raytris/src/sound/soundManager.h
   Copyright (C) 2024 Cole Hall
*/

#pragma once

#include <dc/sound/sound.h>
#include <dc/sound/sfxmgr.h>

class SoundManager {
public:
    SoundManager();
    ~SoundManager();

    void PlayRotateSound();
    void PlayClearSound();

private:
    sfxhnd_t sndRotate;
    sfxhnd_t sndClear;
};