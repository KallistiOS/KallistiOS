#include <kos/thread.h>
#include <dc/video.h>
#include <dc/biosfont.h>
#include <dc/maple.h>
#include <dc/maple/controller.h>
#include <kos/init.h>

KOS_INIT_FLAGS(INIT_DEFAULT | INIT_NET);

void httpd();
void *do_httpd(void * foo) {
    httpd();
    return NULL;
}

int main(int argc, char **argv) {
    thd_create(1, do_httpd, NULL);

    vid_clear(50, 0, 70);
    bfont_draw_str(vram_s + 20 * 640 + 20, 640, 0, "KOSHttpd active");
    bfont_draw_str(vram_s + 44 * 640 + 20, 640, 0, "Press START to quit.");

    thd_sleep(1000 * 5);

    for(; ;) {
        MAPLE_FOREACH_BEGIN(MAPLE_FUNC_CONTROLLER, cont_state_t, st)

        if(st->buttons & CONT_START)
            return 0;

        MAPLE_FOREACH_END()
    }

    return 0;
}
