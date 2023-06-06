/* 
   KallistiOS 2.0.0

   input.c
   (C) 2013 Josh Pearson

   An input callback to update the camera position and destination
   based on user input in a 3d environment. Only meant for testing.
*/

#include <kos.h>

#include "vector.h"
#include "input.h"

#define SHIFT_X  0.01f
#define SHIFT_Z  0.01f
#define SHIFT_Y  0.2f
#define ROT_XZ  ROTATION/5.0


void InputCallback(vector3f campos, vector3f camdst) {
    maple_device_t *cont = maple_enum_type(0, MAPLE_FUNC_CONTROLLER);

    if(cont) {
        cont_state_t *state = (cont_state_t *)maple_dev_status(cont);

        if(!state)
            return;

        if(state->buttons & CONT_DPAD_UP || state->buttons & CONT_DPAD2_UP)
            if(camdst[1] <= campos[1] + 90.0f)
                camdst[1] += SHIFT_Y;

        if(state->buttons & CONT_DPAD_DOWN || state->buttons & CONT_DPAD2_DOWN)
            if(camdst[1] >= campos[1] - 90.0f)
                camdst[1] -= SHIFT_Y;

        if(state->buttons & CONT_DPAD_LEFT || state->buttons & CONT_DPAD2_LEFT)
            vec3f_rotr_xz(camdst[0], camdst[1], camdst[2], campos[0], campos[1], campos[2], -ROT_XZ);

        if(state->buttons & CONT_DPAD_RIGHT || state->buttons & CONT_DPAD2_RIGHT)
            vec3f_rotr_xz(camdst[0], camdst[1], camdst[2], campos[0], campos[1], campos[2], ROT_XZ);

        if(state->buttons & CONT_Y)
            VectorShift(camdst, campos, SHIFT_Z);

        if(state->buttons & CONT_A)
            VectorShift(camdst, campos, -SHIFT_X);

        if(state->buttons & CONT_X) {
            vec3f_rotr_xz(camdst[0], camdst[1], camdst[2], campos[0], campos[1], campos[2], RSHIFT);
            VectorShift(camdst, campos, -SHIFT_X);
            vec3f_rotr_xz(camdst[0], camdst[1], camdst[2], campos[0], campos[1], campos[2], FRSHIFT);
        }

        if(state->buttons & CONT_B) {
            vec3f_rotr_xz(camdst[0], camdst[1], camdst[2], campos[0], campos[1], campos[2], RSHIFT);
            VectorShift(camdst, campos, SHIFT_X);
            vec3f_rotr_xz(camdst[0], camdst[1], camdst[2], campos[0], campos[1], campos[2], FRSHIFT);
        }
    }
}

