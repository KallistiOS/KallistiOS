#include <kos.h>

void mouse_test(void) {
    maple_device_t *cont, *mouse;
    cont_state_t *cstate;
    mouse_state_t *mstate;
    int c = 'M', x = 20, y = 20;

    printf("Now doing mouse test\n");

    while(1) {
        cont = maple_enum_type(0, MAPLE_FUNC_CONTROLLER);

        if(!cont) continue;

        /* Check for start on the controller */
        cstate = (cont_state_t *)maple_dev_status(cont);

        if(!cstate) {
            printf("Error getting controller status\n");
            return;
        }

        if(cstate->buttons & CONT_START) {
            printf("Pressed start\n");
            return;
        }

        thd_sleep(10);

        mouse = maple_enum_type(0, MAPLE_FUNC_MOUSE);

        if(!mouse) continue;

        /* Check for mouse input */
        mstate = (mouse_state_t *)maple_dev_status(mouse);

        if(!mstate) continue;

        /* Move the cursor if applicable */
        if(mstate->dx || mstate->dy || mstate->dz) {
            vid_clear(0, 0, 0);
            x += mstate->dx;
            y += mstate->dy;
            c += mstate->dz;
            bfont_draw(vram_s + (y * 640 + x), 640, 0, c);
        }

        thd_sleep(10);
    }
}

int main(int argc, char **argv) {
    mouse_test();

    return 0;
}
