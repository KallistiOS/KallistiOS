/* KallistiOS ##version##

   examples/dreamcast/raylib/tetris/src/sound/soundManager.cpp
   Copyright (C) 2024 Cole Hall

*/

#include "soundManager.h"

SoundManager::SoundManager(){
    sndRotate = snd_sfx_load("/rd/assets/sound/rotate.wav");
    sndClear = snd_sfx_load("/rd/assets/sound/clear.wav");
}

SoundManager::~SoundManager(){
    snd_sfx_unload(sndRotate);
    snd_sfx_unload(sndClear);
}

void SoundManager::PlayRotateSound(){
    if (sndRotate != SFXHND_INVALID) snd_sfx_play(sndRotate, 255, 128);
}

void SoundManager::PlayClearSound(){
    if (sndClear != SFXHND_INVALID) snd_sfx_play(sndClear, 255, 128);
}
