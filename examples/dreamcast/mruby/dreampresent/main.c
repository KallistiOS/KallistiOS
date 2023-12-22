#include <kos.h>
#include <mruby/mruby.h>
#include <mruby/mruby/irep.h>
#include "dckos.h"

KOS_INIT_FLAGS(INIT_DEFAULT | INIT_MALLOCSTATS);

/* Compiled ruby code */
extern const uint8_t dreampresent_bytecode[];

int main(int argc, char **argv) {
    vid_set_mode(DM_640x480_VGA, PM_RGB565);    

    mrb_state *mrb = mrb_open();
    if (!mrb) { return 1; }

    struct RClass *dc_kos = mrb_define_module(mrb, "DcKos");

    define_module_functions(mrb, dc_kos);

    mrb_load_irep(mrb, dreampresent_bytecode);

    print_exception(mrb);

    mrb_close(mrb);

    return 0;
}
