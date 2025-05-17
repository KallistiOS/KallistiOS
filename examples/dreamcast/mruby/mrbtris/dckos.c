/* KallistiOS ##version##

   dckos.c - Exposed functions for mruby: using KOS features (implementation) 
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
#include <mruby/mruby/internal.h>
#include <mruby/mruby/data.h>
#include <mruby/mruby/string.h>
#include <mruby/mruby/error.h>
#include <mruby/mruby/array.h>
#include <stdio.h>
#include <inttypes.h>

#define PACK_PIXEL(r, g, b) ( (((r) & 0xF8) << 8) | (((g) & 0xFC) << 3) | ((b) >> 3)  )

#define BUFSIZE 100

struct InputBuf {
  uint16_t buffer[BUFSIZE];
  uint32_t index;
} input_buf;

// Globals for cntroller input
//static uint32 *btn_buffer;
//static uint32 *buf_index;

static mrb_value btn_mrb_buffer;

// buf has to be BUFSIZE elements at least
void *read_buttons(void*) {
  while(1) {
    input_buf.index = (input_buf.index + 1) % BUFSIZE;
    //printf("index: %" PRIu32 "\n", buf_index);
    maple_device_t *cont1;
    cont_state_t *state;
    if((cont1 = maple_enum_type(0, MAPLE_FUNC_CONTROLLER))){
      state = (cont_state_t *)maple_dev_status(cont1);
      //printf("buttons: %" PRIu32 "\n", state->buttons);
      input_buf.buffer[input_buf.index] = state->buttons;
    }

    thd_pass();
  }
}

mrb_value init_controller_buffer(mrb_state *mrb, mrb_value self) {
  btn_mrb_buffer = mrb_ary_new(mrb);;
  input_buf.index = 0;

  int i = 0;
  while(i < BUFSIZE) {
    mrb_ary_set(mrb, btn_mrb_buffer, i, mrb_nil_value());
    input_buf.buffer[i] = 0; i ++ ;
  }

  return mrb_nil_value();
}

mrb_value start_controller_reader(mrb_state *mrb, mrb_value self) {
  thd_create(1, read_buttons, NULL);

  // start thread
  return mrb_fixnum_value(0);
}

mrb_value get_current_ms(mrb_state *mrb, mrb_value self) {
    return mrb_fixnum_value(timer_ms_gettime64());
}

mrb_value put_pixel640(mrb_state *mrb, mrb_value self) {
  mrb_int x, y, r, g, b;
  mrb_get_args(mrb, "iiiii", &x, &y, &r, &g, &b);

  vram_s[x + y * 640] = PACK_PIXEL(r, g, b);

  return mrb_nil_value();
}

static mrb_value draw20x20_640(mrb_state *mrb, mrb_value self) {
  mrb_int x, y, r, g, b;
  mrb_get_args(mrb, "iiiii", &x, &y, &r, &g, &b);

  int i = 0, j = 0;

  if(r == 0 && g == 0 && b == 0) {
    for(i = 0; i < 20; i++) {
      for(j = 0; j < 20; j++) {
        vram_s[x+j + (y+i) * 640] = PACK_PIXEL(r, g, b);
      }
    }
  } else {
    int r_light = (r+128 <= 255) ? r+128 : 255;
    int g_light = (g+128 <= 255) ? g+128 : 255;
    int b_light = (b+128 <= 255) ? b+128 : 255;

    int r_dark = (r-64 >= 0) ? r-64 : 0;
    int g_dark = (g-64 >= 0) ? g-64 : 0;
    int b_dark = (b-64 >= 0) ? b-64 : 0;

    // TODO: implement lines and use them.
    for(j = 0; j < 20; j++) {
      vram_s[x+j + (y) * 640] = PACK_PIXEL(30, 30, 30);
      vram_s[x+j + (y+19) * 640] = PACK_PIXEL(30, 30, 30);
    }
    for(j = 1; j < 19; j++) {
      vram_s[x+j + (y+1) * 640] = PACK_PIXEL(r_light, g_light, b_light);
    }
    for(j = 2; j < 20; j++) {
      vram_s[x+j + (y+19) * 640] = PACK_PIXEL(r_dark, g_dark, b_dark);
    }
    for(i = 2; i < 19; i++) {
      vram_s[x + (y+i) * 640] = PACK_PIXEL(30, 30, 30);
      vram_s[x+1 + (y+i) * 640] = PACK_PIXEL(r_light, g_light, b_light);
      for(j = 2; j < 19; j++) {
        vram_s[x+j + (y+i) * 640] = PACK_PIXEL(r, g, b);
      }
      vram_s[x+19 + (y+i) * 640] = PACK_PIXEL(r_dark, g_dark, b_dark);
      //vram_s[x+19 + (y+i) * 640] = PACK_PIXEL(30, 30, 30);
    }
  }

  return mrb_nil_value();
}

static mrb_value waitvbl(mrb_state *mrb, mrb_value self) {
  vid_waitvbl();

  return mrb_nil_value();
}

static mrb_value get_button_state(mrb_state *mrb, mrb_value self) {
  //printf("returning buttons at: %" PRIu32 ", value: " PRIu32 "\n", *buf_index, btn_buffer[*buf_index]);
  if(maple_enum_type(0, MAPLE_FUNC_CONTROLLER)){
    return mrb_fixnum_value(input_buf.buffer[input_buf.index]);
  } else {
    return mrb_nil_value();
  }
}

mrb_value get_next_button_state(mrb_state *mrb, mrb_value self) {
  mrb_int wanted_index;
  mrb_get_args(mrb, "i", &wanted_index);
  int curr_index = input_buf.index;

    //int j;
    //for(j=0 ; j < BUFSIZE ; j++) { printf("%d,", (int)input_buf.buffer[j]); }
    //printf("\n\n");

  if(wanted_index >= BUFSIZE || wanted_index < 0) { wanted_index = wanted_index % BUFSIZE; }

  // return null if wanted index is current + 1
  // TODO: this will break if processing is slow and current catches up with wanted - 1
  // So this should overwrite just-read values with null.
  if(wanted_index == (curr_index + 1) % BUFSIZE) {
    return mrb_nil_value();
  } else {
    return mrb_fixnum_value(input_buf.buffer[wanted_index]);
  }
}

mrb_value get_current_button_index(mrb_state *mrb, mrb_value self) {
  return mrb_fixnum_value(input_buf.index);
}

// synchronises input buf with btn_mrb_buffer
static mrb_value get_button_states(mrb_state *mrb, mrb_value self) {
  static mrb_int btn_mrb_index;
  mrb_int idx;
  mrb_get_args(mrb, "i", &idx);

  btn_mrb_index = input_buf.index; // Background thread may be updating buf index

  //printf("idx: %d\n", (int)idx);
  //printf("btn_mrb_index: %d\n", (int)btn_mrb_index);

    int j;
    for(j=0 ; j < BUFSIZE ; j++) { printf("%d,", (int)input_buf.buffer[j]); }
    printf("\n\n");

  // see if arena thing helps...
  int arena_idx = mrb_gc_arena_save(mrb);
  //printf("starting at %d\n", (int)idx);
  //printf("-- btn index: %d, " PRIu32, input_buf.buffer[btn_mrb_index]);
  while(idx != btn_mrb_index) {
    //printf("idx: %d\n", (int)idx);
    //printf("-- setting %" PRIu32, input_buf.buffer[idx]);
    mrb_ary_set(mrb, btn_mrb_buffer, idx, mrb_fixnum_value(input_buf.buffer[idx]));
    //printf("...set\n");

    idx = (idx + 1) % BUFSIZE; // last position has been read, so skip
  }
  // one more to copy at btn_mrb_index
  mrb_ary_set(mrb, btn_mrb_buffer, idx, mrb_fixnum_value(input_buf.buffer[idx]));
  // printf("ending at %d with value %" PRIu32 "\n\n", (int)idx, input_buf.buffer[idx]);

  //printf("-- the idx whent up to %d\n", (int)idx);
  mrb_gc_arena_restore(mrb, arena_idx);

  static mrb_value result_array;
  result_array = mrb_ary_new(mrb);

  mrb_ary_push(mrb, result_array, mrb_fixnum_value(btn_mrb_index));
  mrb_ary_push(mrb, result_array, btn_mrb_buffer);

//  printf("-- returning: %d and %d \n\n",
//    (int)mrb_fixnum(mrb_ary_entry(result_array, 0)),
//    (int)mrb_fixnum(mrb_ary_entry(mrb_ary_entry(result_array, 1), input_buf.index))
//  );
  return result_array;
}

static mrb_value start_btn(mrb_state *mrb, mrb_value self) {
  struct mrb_value state;
  mrb_get_args(mrb, "i", &state);

  return mrb_bool_value(mrb_fixnum(state) & CONT_START);
}

static mrb_value dpad_left(mrb_state *mrb, mrb_value self) {
  struct mrb_value state;
  mrb_get_args(mrb, "i", &state);

  return mrb_bool_value(mrb_fixnum(state) & CONT_DPAD_LEFT);
}

static mrb_value dpad_right(mrb_state *mrb, mrb_value self) {
  struct mrb_value state;
  mrb_get_args(mrb, "i", &state);

  return mrb_bool_value(mrb_fixnum(state) & CONT_DPAD_RIGHT);
}

static mrb_value dpad_up(mrb_state *mrb, mrb_value self) {
  struct mrb_value state;
  mrb_get_args(mrb, "i", &state);

  return mrb_bool_value(mrb_fixnum(state) & CONT_DPAD_UP);
}

static mrb_value dpad_down(mrb_state *mrb, mrb_value self) {
  struct mrb_value state;
  mrb_get_args(mrb, "i", &state);

  return mrb_bool_value(mrb_fixnum(state) & CONT_DPAD_DOWN);
}

static mrb_value btn_b(mrb_state *mrb, mrb_value self) {
  struct mrb_value state;
  mrb_get_args(mrb, "i", &state);

  return mrb_bool_value(mrb_fixnum(state) & CONT_B);
}

static mrb_value btn_a(mrb_state *mrb, mrb_value self) {
  struct mrb_value state;
  mrb_get_args(mrb, "i", &state);

  return mrb_bool_value(mrb_fixnum(state) & CONT_A);
}

mrb_value get_button_masks(mrb_state *mrb, mrb_value self) {
  mrb_value mask_array;
  mask_array = mrb_ary_new(mrb);

  mrb_ary_push(mrb, mask_array, mrb_fixnum_value(CONT_START));
  mrb_ary_push(mrb, mask_array, mrb_fixnum_value(CONT_DPAD_LEFT));
  mrb_ary_push(mrb, mask_array, mrb_fixnum_value(CONT_DPAD_RIGHT));
  mrb_ary_push(mrb, mask_array, mrb_fixnum_value(CONT_DPAD_UP));
  mrb_ary_push(mrb, mask_array, mrb_fixnum_value(CONT_DPAD_DOWN));
  mrb_ary_push(mrb, mask_array, mrb_fixnum_value(CONT_A));
  mrb_ary_push(mrb, mask_array, mrb_fixnum_value(CONT_B));

  return mask_array;
}

static mrb_value clear_score(mrb_state *mrb, mrb_value self) {
  char* clear_str = "Press START";
  bfont_draw_str(vram_s + 640 * 100 + 16, 640, 1, clear_str);

  return mrb_nil_value();
}

static mrb_value render_score(mrb_state *mrb, mrb_value self) {
  struct mrb_value score;
  mrb_get_args(mrb, "i", &score);
  bfont_draw_str_fmt(vram_s + 640 * 100 + 16, 640, 1, "Score: %8" PRId32, mrb_fixnum(score));

  return mrb_nil_value();
}

void print_exception(mrb_state* mrb) {
    if(mrb->exc) {
      mrb_value backtrace = mrb_get_backtrace(mrb);
      puts(mrb_str_to_cstr(mrb, mrb_inspect(mrb, backtrace)));

      mrb_value obj = mrb_funcall(mrb, mrb_obj_value(mrb->exc), "inspect", 0);
      fwrite(RSTRING_PTR(obj), RSTRING_LEN(obj), 1, stdout);
      putc('\n', stdout);
    }
}

void define_module_functions(mrb_state* mrb, struct RClass* module) {
    mrb_define_module_function(mrb, module, "put_pixel640", put_pixel640, MRB_ARGS_REQ(5));
    mrb_define_module_function(mrb, module, "draw20x20_640", draw20x20_640, MRB_ARGS_REQ(5));
    mrb_define_module_function(mrb, module, "waitvbl", waitvbl, MRB_ARGS_NONE());

    mrb_define_module_function(mrb, module, "clear_score", clear_score, MRB_ARGS_NONE());

    mrb_define_module_function(mrb, module, "render_score", render_score, MRB_ARGS_REQ(1));

    // TODO: get rid of single
    mrb_define_module_function(mrb, module, "get_button_state", get_button_state, MRB_ARGS_NONE());
    mrb_define_module_function(mrb, module, "get_button_states", get_button_states, MRB_ARGS_REQ(1));
    mrb_define_module_function(mrb, module, "get_next_button_state", get_next_button_state, MRB_ARGS_REQ(1));
    mrb_define_module_function(mrb, module, "get_current_button_index", get_current_button_index, MRB_ARGS_NONE());

    // TODO: maybe switch to doing these in mruby?
    mrb_define_module_function(mrb, module, "start_btn?", start_btn, MRB_ARGS_REQ(1));
    mrb_define_module_function(mrb, module, "dpad_left?", dpad_left, MRB_ARGS_REQ(1));
    mrb_define_module_function(mrb, module, "dpad_right?", dpad_right, MRB_ARGS_REQ(1));
    mrb_define_module_function(mrb, module, "dpad_up?", dpad_up, MRB_ARGS_REQ(1));
    mrb_define_module_function(mrb, module, "dpad_down?", dpad_down, MRB_ARGS_REQ(1));
    mrb_define_module_function(mrb, module, "btn_a?", btn_a, MRB_ARGS_REQ(1));
    mrb_define_module_function(mrb, module, "btn_b?", btn_b, MRB_ARGS_REQ(1));

    mrb_define_module_function(mrb, module, "get_button_masks", get_button_masks, MRB_ARGS_NONE());

    mrb_define_module_function(mrb, module, "get_current_ms", get_current_ms, MRB_ARGS_NONE());
    mrb_define_module_function(mrb, module, "start_controller_reader", start_controller_reader, MRB_ARGS_NONE());
    mrb_define_module_function(mrb, module, "init_controller_buffer", init_controller_buffer, MRB_ARGS_NONE());

    // TODO: stop_controller_reader?
    // get_input_buffer
}
