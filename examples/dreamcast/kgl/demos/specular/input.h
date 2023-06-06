/* 
   KallistiOS 2.0.0

   input.h
   (C) 2013 Josh Pearson
*/

#ifndef INPUT_H
#define INPUT_H

#define STATE_IDLE   0x00000000
#define STATE_JOG    0x0000000F
#define STATE_RUN    0x000000F0
#define STATE_RTURN  0x00000F00
#define STATE_LTURN  0x0000F000
#define STATE_JUMP   0x000F0000
#define STATE_FALL   0x00F00000
#define STATE_ATTACK 0x0F000000
#define STATE_HURT   0xF0000000

#define RANGLE 15.0f          // Rotation Angle
#define RSD 360.0f/RANGLE     // Rotated Side
static const float ANGLE = 10.0f;
#define ROTATION RADIAN*ANGLE
static const float RSHIFT = RADIAN * 90.0f;
static const float FRSHIFT = RADIAN * 270.0f;

void InputCallback(vector3f campos, vector3f camdst);

#endif
