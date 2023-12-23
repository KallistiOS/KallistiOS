/* KallistiOS ##version##

   main.c
   Copyright (C) 2019-2024 Yuji Yokoo

   A simple presentation tool for Sega Dreamcast written in Ruby
*/

#include <kos.h>
#include <mruby/mruby.h>
#include <mruby/mruby/irep.h>
#include "dckos.h"

/* These macros tell KOS how to initialize itself. All of this initialization
   happens before main() gets called, and the shutdown happens afterwards. So
   you need to set any flags you want here. Here are some possibilities:

   INIT_NONE         -- don't do any auto init
   INIT_IRQ          -- knable IRQs
   INIT_THD_PREEMPT  -- Enable pre-emptive threading
   INIT_NET          -- Enable networking (doesn't imply lwIP!)
   INIT_MALLOCSTATS  -- Enable a call to malloc_stats() right before shutdown

   You can OR any or all of those together. If you want to start out with
   the current KOS defaults, use INIT_DEFAULT (or leave it out entirely). */
KOS_INIT_FLAGS(INIT_DEFAULT | INIT_MALLOCSTATS);

/* Compiled Ruby code, declared in the RB file */
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
