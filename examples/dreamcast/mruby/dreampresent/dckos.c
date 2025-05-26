/* KallistiOS ##version##

   dckos.c - Exposed functions for mruby: using KOS features (implementation) 
   Copyright (C) 2019-2024 Yuji Yokoo
   Copyright (C) 2020-2024 Mickaël "SiZiOUS" Cardoso

   Dreampresent
   A simple presentation tool for Sega Dreamcast written in Ruby

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

#include <assert.h>
#include <kos.h>
#include <kos/img.h>
#include <mruby/mruby.h>
#include <mruby/mruby/internal.h>
#include <mruby/mruby/data.h>
#include <mruby/mruby/string.h>
#include <mruby/mruby/error.h>
#include <stdio.h>
#include <inttypes.h>
#include <png/png.h>

// Convert ARGB1555 to RGB565 - drops the A bit and G is exapanded to 6 bits
#define CONV1555TO565(colour) ( (((colour) & 0x7C00) << 1) | (((colour) & 0x03E0) << 1) | ((colour) & 0x001F) )

// CONVERT R, G, B to RGB565
#define PACK_PIXEL(r, g, b) ( (((r) & 0xF8) << 8) | (((g) & 0xFC) << 3) | ((b) >> 3)  )

int PX_PER_LINE = 640;

int _is_in_screen(int x, int y) {
  return x >= 0 && x < PX_PER_LINE && y >= 0 && y < 480;
}

// Be careful with this function. It'll attempt to read the entire file.
static mrb_value read_whole_txt_file(mrb_state *mrb, mrb_value self) {
  char buffer[2048];
  int length;
  file_t f;
  mrb_value m_path;
  char *path;

  char *result = NULL;
  result = mrb_malloc(mrb, sizeof(char));
  *result = '\0';

  mrb_get_args(mrb, "S", &m_path);
  path = mrb_str_to_cstr(mrb, m_path);
  f = fs_open(path, O_RDONLY);

  if(f < 0) {
    printf("Failed to open %s.\n", path);
    return mrb_nil_value();
  }

  while((length = fs_read(f, buffer, 2048))) {
    printf("read %i chars into buf.\n", length);
    result = mrb_realloc(mrb, result, strlen(result) + length + 1); // 1 for '\0'
    strncat(result, buffer, length);
  }

  fs_close(f);
  mrb_value whole_str_mrb = mrb_str_new_cstr(mrb, result);
  mrb_free(mrb, result);

  return whole_str_mrb;
}

mrb_value get_button_state(mrb_state *mrb, mrb_value self) {
  maple_device_t *cont1;
  cont_state_t *state;
  if((cont1 = maple_enum_type(0, MAPLE_FUNC_CONTROLLER))){
    state = (cont_state_t *)maple_dev_status(cont1);
    return mrb_fixnum_value(state->buttons);
  }
  return mrb_nil_value();
}

mrb_value check_btn(mrb_state *mrb, mrb_value self, uint16_t target) {
  struct mrb_value state;
  mrb_get_args(mrb, "i", &state);

  return mrb_bool_value(mrb_fixnum(state) & target);
}

mrb_value btn_start(mrb_state *mrb, mrb_value self) {
  return check_btn(mrb, self, CONT_START);
}

mrb_value btn_a(mrb_state *mrb, mrb_value self) {
  return check_btn(mrb, self, CONT_A);
}

mrb_value btn_b(mrb_state *mrb, mrb_value self) {
  return check_btn(mrb, self, CONT_B);
}

mrb_value dpad_down(mrb_state *mrb, mrb_value self) {
  return check_btn(mrb, self, CONT_DPAD_DOWN);
}

mrb_value dpad_right(mrb_state *mrb, mrb_value self) {
  return check_btn(mrb, self, CONT_DPAD_RIGHT);
}

mrb_value dpad_left(mrb_state *mrb, mrb_value self) {
  return check_btn(mrb, self, CONT_DPAD_LEFT);
}

mrb_value draw_str(mrb_state *mrb, mrb_value self) {
  char *unwrapped_content;
  mrb_value str_content;
  mrb_int x, y, r, g, b, bg_on;

  mrb_get_args(mrb, "Siiiiii", &str_content, &x, &y, &r, &g, &b, &bg_on);
  unwrapped_content = mrb_str_to_cstr(mrb, str_content);
  printf("KOS draw_str: %s\n", unwrapped_content);

  // assuming 16 bit colours
  bfont_draw_str_ex(vram_s + x + (y * PX_PER_LINE), PX_PER_LINE, PACK_PIXEL(r, g, b), 0x00000000, (sizeof (uint16_t)) << 3, bg_on, unwrapped_content);

  return mrb_nil_value();
}

mrb_value console_print(mrb_state *mrb, mrb_value self) {
  char *unwrapped_content;
  mrb_value str_content;

  mrb_get_args(mrb, "S", &str_content);
  unwrapped_content = mrb_str_to_cstr(mrb, str_content);
  printf("%s\n", unwrapped_content);

  return mrb_nil_value();
}

void _display_png_file(char *file_path, int x1, int y1, int x2, int y2) {
  pvr_ptr_t texture;
  pvr_poly_cxt_t cxt;
  pvr_poly_hdr_t hdr;
  pvr_vertex_t vert;

  pvr_wait_ready();
  pvr_scene_begin();

  pvr_list_begin(PVR_LIST_OP_POLY);

  texture = pvr_mem_malloc(512 * 512 * 2);
  png_to_texture(file_path, texture, PNG_NO_ALPHA);

  pvr_poly_cxt_txr(&cxt, PVR_LIST_OP_POLY, PVR_TXRFMT_RGB565, 512, 512, texture, PVR_FILTER_BILINEAR);
  pvr_poly_compile(&hdr, &cxt);
  pvr_prim(&hdr, sizeof(hdr));

  vert.argb = PVR_PACK_COLOR(1.0f, 1.0f, 1.0f, 1.0f);
  vert.oargb = 0;
  vert.flags = PVR_CMD_VERTEX;

  vert.x = x1;
  vert.y = y1;
  vert.z = 1;
  vert.u = 0.0;
  vert.v = 0.0;
  pvr_prim(&vert, sizeof(vert));

  vert.x = x2;
  vert.y = y1;
  vert.z = 1;
  vert.u = 1.0;
  vert.v = 0.0;
  pvr_prim(&vert, sizeof(vert));

  vert.x = x1;
  vert.y = y2;
  vert.z = 1;
  vert.u = 0.0;
  vert.v = 1.0;
  pvr_prim(&vert, sizeof(vert));

  vert.x = x2;
  vert.y = y2;
  vert.z = 1;
  vert.u = 1.0;
  vert.v = 1.0;
  vert.flags = PVR_CMD_VERTEX_EOL;
  pvr_prim(&vert, sizeof(vert));

  pvr_list_finish();
  pvr_scene_finish();

  pvr_mem_free(texture);

  return;
}

mrb_value draw_horizontal_line(mrb_state *mrb, mrb_value self) {
  mrb_int x, y, len, r, g, b;
  mrb_get_args(mrb, "iiiiii", &x, &y, &len, &r, &g, &b);

  int curr_x = 0;

  for(curr_x = x; curr_x < x + len; curr_x ++) {
    if(_is_in_screen(curr_x, y)) {
      vram_s[y * PX_PER_LINE + curr_x] = PACK_PIXEL(r, g, b);
    }
  }

  return mrb_nil_value();
}

mrb_value draw_vertical_line(mrb_state *mrb, mrb_value self) {
  mrb_int x, y, len, r, g, b;
  mrb_get_args(mrb, "iiiiii", &x, &y, &len, &r, &g, &b);

  int curr_y = 0;

  for(curr_y = y; curr_y < y + len; curr_y ++) {
    if(_is_in_screen(x, curr_y)) {
      vram_s[curr_y * PX_PER_LINE + x] = PACK_PIXEL(r, g, b);
    }
  }

  return mrb_nil_value();
}

// this uses pvr functions to show max 512x512 image.
mrb_value load_png(mrb_state *mrb, mrb_value self) {
  mrb_value png_path;
  mrb_int x1, y1, x2, y2;
  char *c_png_path;

  mrb_get_args(mrb, "Siiii", &png_path, &x1, &y1, &x2, &y2);
  c_png_path = mrb_str_to_cstr(mrb, png_path);

  _display_png_file(c_png_path, x1, y1, x2, y2);

  return mrb_nil_value();
}

// this renders to vram_s
mrb_value render_png(mrb_state *mrb, mrb_value self) {
  mrb_value png_path;
  mrb_int base_x, base_y;
  char *c_png_path;
  kos_img_t img;

  mrb_get_args(mrb, "Sii", &png_path, &base_x, &base_y);
  c_png_path = mrb_str_to_cstr(mrb, png_path);
  printf("---- got path: %s \n", c_png_path);

  png_to_img(c_png_path, 1, &img);

  printf("---- KOS_IMG_FMT_I %lu, KOS_IMG_FMT_D %lu\n", KOS_IMG_FMT_I(img.fmt), KOS_IMG_FMT_D(img.fmt));

  assert( KOS_IMG_FMT_I(img.fmt) == KOS_IMG_FMT_ARGB1555 );
  int x = 0;
  int y = 0;

  uint16_t *uint16_data = (uint16_t*)(img.data);

  for(y = 0 ; y < img.h ; y++) {
    uint16_t pixel = 0;
    int curr_x = 0;
    int curr_y = 0;
    for(x = 0 ; x < img.w ; x++) {
      pixel = uint16_data[(y) * img.w + (x)];
      curr_y = (base_y + y);
      curr_x = (base_x + x);
      if (pixel >> 15 && _is_in_screen(curr_x, curr_y)) {
        vram_s[curr_y * PX_PER_LINE + curr_x] = CONV1555TO565(pixel);
      }
    }
  }

  kos_img_free(&img, 0);

  return mrb_nil_value();
}

mrb_value pvr_initialise(mrb_state *mrb, mrb_value self) {
  pvr_init_defaults();

  return mrb_nil_value();
}

mrb_value next_video_mode(mrb_state *mrb, mrb_value self) {
  // TODO: this is dup setting from main.c
  static int vid_modes[] = {DM_640x480_VGA, DM_640x480_NTSC_IL, DM_640x480_PAL_IL};
  static int idx = 0;

  if(idx == (sizeof(vid_modes) / sizeof(int)) - 1 ) {
    idx = 0;
  } else {
    idx++;
  }

  printf("---- set video mode to %d\n", vid_modes[idx]);

  vid_set_mode(vid_modes[idx], PM_RGB565);

  return mrb_nil_value();
}

void print_exception(mrb_state *mrb) {
  if(mrb->exc) {
    mrb_value backtrace = mrb_get_backtrace(mrb);
    puts(mrb_str_to_cstr(mrb, mrb_inspect(mrb, backtrace)));

    mrb_value obj = mrb_funcall(mrb, mrb_obj_value(mrb->exc), "inspect", 0);
    fwrite(RSTRING_PTR(obj), RSTRING_LEN(obj), 1, stdout);
    putc('\n', stdout);
  }
}

void define_module_functions(mrb_state *mrb, struct RClass *module) {
  mrb_define_module_function(mrb, module, "read_whole_txt_file", read_whole_txt_file, MRB_ARGS_REQ(1));
  mrb_define_module_function(mrb, module, "draw_str", draw_str, MRB_ARGS_REQ(7));
  mrb_define_module_function(mrb, module, "load_png", load_png, MRB_ARGS_REQ(4));
  mrb_define_module_function(mrb, module, "render_png", render_png, MRB_ARGS_REQ(3));
  mrb_define_module_function(mrb, module, "pvr_initialise", pvr_initialise, MRB_ARGS_NONE());
  mrb_define_module_function(mrb, module, "get_button_state", get_button_state, MRB_ARGS_NONE());
  mrb_define_module_function(mrb, module, "btn_start?", btn_start, MRB_ARGS_REQ(1));
  mrb_define_module_function(mrb, module, "btn_a?", btn_a, MRB_ARGS_REQ(1));
  mrb_define_module_function(mrb, module, "btn_b?", btn_b, MRB_ARGS_REQ(1));
  mrb_define_module_function(mrb, module, "dpad_down?", dpad_down, MRB_ARGS_REQ(1));
  mrb_define_module_function(mrb, module, "dpad_right?", dpad_right, MRB_ARGS_REQ(1));
  mrb_define_module_function(mrb, module, "dpad_left?", dpad_left, MRB_ARGS_REQ(1));
  mrb_define_module_function(mrb, module, "console_print", console_print, MRB_ARGS_REQ(1));
  mrb_define_module_function(mrb, module, "draw_horizontal_line", draw_horizontal_line, MRB_ARGS_REQ(6));
  mrb_define_module_function(mrb, module, "draw_vertical_line", draw_vertical_line, MRB_ARGS_REQ(6));
  mrb_define_module_function(mrb, module, "next_video_mode", next_video_mode, MRB_ARGS_NONE());
}
