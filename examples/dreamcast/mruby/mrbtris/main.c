/* KallistiOS ##version##

   main.c - Main entry point
   Copyright (C) 2019-2024 Yuji Yokoo
   Copyright (C) 2020-2024 Mickaël "SiZiOUS" Cardoso

   Mrbtris
   A sample Tetris clone for Sega Dreamcast written in Ruby

   Permission is hereby granted, free of charge, to any person obtaining a copy
   of this software and associated documentation files (the "Software"), to deal
   in the Software without restriction, including without limitation the rights
   to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
   copies of the Software, and to permit persons to whom the Software is
   furnished to do so, subject to the following conditions:

   The above copyright notice and this permission notice shall be included in
   all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
   SOFTWARE.
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
extern const uint8_t mrbtris_bytecode[];

int main(int argc, char **argv) {
    vid_set_mode(DM_640x480_VGA, PM_RGB565);

    mrb_state *mrb = mrb_open();
    if (!mrb) { return 1; }

    struct RClass *dc2d_module = mrb_define_module(mrb, "Dc2d");

    define_module_functions(mrb, dc2d_module);

    mrb_load_irep(mrb, mrbtris_bytecode);

    print_exception(mrb);

    mrb_close(mrb);

    return 0;
}
