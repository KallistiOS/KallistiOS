/* KallistiOS ##version##

   mie_basic.c
   Copyright (C) 2026 Ruslan Rostovtsev

   Poll MIE/JVS inputs, run wheel/pedal calibration, blink JVS outputs,
   and print mapped controller state plus native JVS fields.
*/

#include <stdio.h>
#include <string.h>

#include <kos.h>
#include <kos/init.h>

#if defined(_arch_sub_naomi)
#include <dc/minifont.h>
#define bfont_draw(a,b,c,d) minifont_draw((a),(b),(d))
#else
#include <dc/biosfont.h>
#endif
#include <dc/maple.h>
#include <dc/maple/controller.h>
#include <dc/maple/keyboard.h>
#include <dc/maple/mie.h>
#include <dc/maple/mouse.h>
#include <dc/pvr.h>

KOS_INIT_FLAGS(INIT_DEFAULT | INIT_MIE);

#define UI_FONT_W   12
#define UI_FONT_H   24
#define UI_LINE_MAX 80
#define UI_ROW_MAX  19

static pvr_ptr_t ui_font_tex;

static const char *port_mode_name(mie_port0_mode_t mode) {
    switch(mode) {
    case MIE_PORT0_JVS:
        return "JVS";
    case MIE_PORT0_MAPLE:
        return "Maple";
    default:
        return "unknown";
    }
}

static void ui_font_init(void) {
    uint16_t *vram;
    int x, y;

    ui_font_tex = pvr_mem_malloc(256 * 256 * 2);
    vram = (uint16_t *)ui_font_tex;

    for(y = 0; y < 8; y++) {
        for(x = 0; x < 16; x++) {
            bfont_draw(vram, 256, 0, y * 16 + x);
            vram += 16;
        }
        vram += 23 * 256;
    }
}

static void ui_draw_char(float x1, float y1, int c) {
    pvr_vertex_t vert;
    int ix, iy;
    float u1, v1, u2, v2;

    if(c == ' ') {
        return;
    }

    if(!(c > ' ' && c < 127)) {
        return;
    }

    ix = (c % 16) * 16;
    iy = (c / 16) * 24;
    u1 = ix * 1.0f / 256.0f;
    v1 = iy * 1.0f / 256.0f;
    u2 = (ix + 12) * 1.0f / 256.0f;
    v2 = (iy + 24) * 1.0f / 256.0f;

    vert.flags = PVR_CMD_VERTEX;
    vert.x = x1;
    vert.y = y1 + UI_FONT_H;
    vert.z = 1.0f;
    vert.u = u1;
    vert.v = v2;
    vert.argb = PVR_PACK_COLOR(1.0f, 1.0f, 1.0f, 1.0f);
    vert.oargb = 0;
    pvr_prim(&vert, sizeof(vert));

    vert.x = x1;
    vert.y = y1;
    vert.u = u1;
    vert.v = v1;
    pvr_prim(&vert, sizeof(vert));

    vert.x = x1 + UI_FONT_W;
    vert.y = y1 + UI_FONT_H;
    vert.u = u2;
    vert.v = v2;
    pvr_prim(&vert, sizeof(vert));

    vert.flags = PVR_CMD_VERTEX_EOL;
    vert.x = x1 + UI_FONT_W;
    vert.y = y1;
    vert.u = u2;
    vert.v = v1;
    pvr_prim(&vert, sizeof(vert));
}

static void ui_draw_str(float x, float y, const char *str) {
    pvr_poly_cxt_t cxt;
    pvr_poly_hdr_t poly;
    int i, len;

    if(!str || !str[0]) {
        return;
    }

    len = (int)strlen(str);

    pvr_poly_cxt_txr(&cxt, PVR_LIST_TR_POLY,
                     PVR_TXRFMT_ARGB1555 | PVR_TXRFMT_NONTWIDDLED,
                     256, 256, ui_font_tex, PVR_FILTER_NONE);
    pvr_poly_compile(&poly, &cxt);
    pvr_prim(&poly, sizeof(poly));

    for(i = 0; i < len; i++) {
        ui_draw_char(x, y, str[i]);
        x += UI_FONT_W;
    }
}

static void ui_draw(const char lines[][UI_LINE_MAX], int count) {
    float x = 8.0f;
    float y = 8.0f;
    int i;

    pvr_wait_ready();
    pvr_scene_begin();
    pvr_list_begin(PVR_LIST_TR_POLY);

    for(i = 0; i < count; i++) {
        ui_draw_str(x, y, lines[i]);
        y += UI_FONT_H;
    }

    pvr_list_finish();
    pvr_scene_finish();
}

static volatile uint32_t coin_cb_count = 0;

static void mie_coin_cb(mie_jvs_input_t input, uint32_t mask) {
    (void)input;
    (void)mask;
    coin_cb_count++;
}

static void mie_service_cb(mie_jvs_input_t input, uint32_t mask) {
    (void)input;
    (void)mask;
    mie_coin_add(0, 1, true);
}

static void outputs_blink(void) {
    static uint64_t last_ms;
    static uint8_t step;
    uint64_t now;

    if(mie_port0_mode() != MIE_PORT0_JVS) {
        return;
    }

    now = timer_ms_gettime64();
    if(now - last_ms < 500) {
        return;
    }

    last_ms = now;
    mie_jvs_set_outputs(MIE_JVS_OUTPUT_MASK(step), false);
    step = (uint8_t)((step + 1) % MIE_JVS_OUTPUT_COUNT);
}

static const char *calib_step_name(mie_analog_calib_step_t step) {
    switch(step) {
    case MIE_ANALOG_CALIB_WHEEL:
        return "turn wheel fully both ways, press A";
    case MIE_ANALOG_CALIB_WHEEL_CENTER:
        return "hold wheel centered, press A";
    case MIE_ANALOG_CALIB_ACCEL:
        return "press accel fully, press A";
    case MIE_ANALOG_CALIB_BRAKE:
        return "press brake fully, press A";
    default:
        return "idle";
    }
}

static uint32_t ui_poll_aux_buttons(void) {
    uint32_t btns = 0;

    MAPLE_FOREACH_BEGIN(MAPLE_FUNC_CONTROLLER, cont_state_t, c)
        if(c) {
            btns |= c->buttons;
        }
    MAPLE_FOREACH_END()

    MAPLE_FOREACH_BEGIN(MAPLE_FUNC_KEYBOARD, kbd_state_t, kbd)
        if(kbd) {
            if(kbd->key_states[KBD_KEY_A].is_down) {
                btns |= CONT_A;
            }
            if(kbd->key_states[KBD_KEY_B].is_down) {
                btns |= CONT_B;
            }
            if(kbd->key_states[KBD_KEY_ESCAPE].is_down) {
                btns |= CONT_START;
            }
        }
    MAPLE_FOREACH_END()

    MAPLE_FOREACH_BEGIN(MAPLE_FUNC_MOUSE, mouse_state_t, m)
        if(m) {
            if(m->buttons & MOUSE_LEFTBUTTON) {
                btns |= CONT_A;
            }
            if(m->buttons & MOUSE_RIGHTBUTTON) {
                btns |= CONT_B;
            }
            if(m->buttons & MOUSE_SIDEBUTTON) {
                btns |= CONT_START;
            }
        }
    MAPLE_FOREACH_END()

    return btns;
}

static void ui_buttons_text(char *linebuf, size_t len, uint32_t btns) {
    size_t n = 0;

    linebuf[0] = '\0';

    if(btns & CONT_START)
        n += snprintf(linebuf + n, len - n, "START ");
    if(btns & CONT_A)
        n += snprintf(linebuf + n, len - n, "A ");
    if(btns & CONT_B)
        n += snprintf(linebuf + n, len - n, "B ");
    if(btns & CONT_X)
        n += snprintf(linebuf + n, len - n, "X ");
    if(btns & CONT_Y)
        n += snprintf(linebuf + n, len - n, "Y ");
    if(btns & CONT_Z)
        n += snprintf(linebuf + n, len - n, "Z ");
    if(btns & CONT_C)
        n += snprintf(linebuf + n, len - n, "C ");
    if(btns & CONT_D)
        n += snprintf(linebuf + n, len - n, "D ");
    if(btns & CONT_DPAD_UP)
        n += snprintf(linebuf + n, len - n, "UP ");
    if(btns & CONT_DPAD_DOWN)
        n += snprintf(linebuf + n, len - n, "DOWN ");
    if(btns & CONT_DPAD_LEFT)
        n += snprintf(linebuf + n, len - n, "LEFT ");
    if(btns & CONT_DPAD_RIGHT)
        n += snprintf(linebuf + n, len - n, "RIGHT ");

    if(n == 0) {
        snprintf(linebuf, len, "(none)");
    }
}

static void ui_sys_line(char *linebuf, size_t len,
                        const mie_jvs_system_t *sys) {
    size_t n;

    n = snprintf(linebuf, len, "sys=0x%02x", sys->raw);
    if(sys->test) {
        n += snprintf(linebuf + n, len - n, " test");
    }
    if(sys->raw == 0) {
        snprintf(linebuf + n, len - n, " (idle)");
    }
}

static void ui_panel_line(char *linebuf, size_t len,
                            const mie_jvs_panel_t *panel) {
    snprintf(linebuf, len,
             "panel dip=0x%02x psw=0x%02x test=%d service=%d",
             panel->dip.raw, panel->psw.raw,
             panel->psw.test, panel->psw.service);
}

static int ui_build(mie_state_t *st, char lines[][UI_LINE_MAX]) {
    int n = 0;

    snprintf(lines[n++], UI_LINE_MAX, "MIE basic example");
    snprintf(lines[n++], UI_LINE_MAX, "port A mode: %s",
             port_mode_name(mie_port0_mode()));

    if(!st) {
        snprintf(lines[n++], UI_LINE_MAX, "No MIE device (waiting...)");
        snprintf(lines[n++], UI_LINE_MAX,
                 "outputs=0x%08lx (running light 0..%d)",
                 (unsigned long)mie_jvs_get_outputs(),
                 MIE_JVS_OUTPUT_COUNT - 1);
        snprintf(lines[n++], UI_LINE_MAX,
                 "Press START on pad or Esc to exit");
        return n;
    }

    snprintf(lines[n++], UI_LINE_MAX, "Mapped controller (st->cont):");
    ui_buttons_text(lines[n++], UI_LINE_MAX, st->cont.buttons);
    snprintf(lines[n++], UI_LINE_MAX,
             "joyx=%d ltrig=%d rtrig=%d",
             st->cont.joyx, st->cont.ltrig, st->cont.rtrig);
    snprintf(lines[n++], UI_LINE_MAX, "Native JVS (st->jvs):");
    ui_sys_line(lines[n++], UI_LINE_MAX, &st->jvs.system);
    snprintf(lines[n++], UI_LINE_MAX, "p1=0x%04x p2=0x%04x",
             st->jvs.p1.raw, st->jvs.p2.raw);
    ui_panel_line(lines[n++], UI_LINE_MAX, &st->jvs.panel);
    snprintf(lines[n++], UI_LINE_MAX,
             "meter0=%u meter1=%u coin callbacks: %lu",
             mie_get_coin_meter(0), mie_get_coin_meter(1),
             (unsigned long)coin_cb_count);
    snprintf(lines[n++], UI_LINE_MAX,
             "coin0 raw=0x%04x count=%u busy=0x%04x",
             st->jvs.coin[0].raw, (unsigned)st->jvs.coin[0].count,
             (unsigned)(st->jvs.coin[0].raw & MIE_JVS_COIN_BUSY));
    snprintf(lines[n++], UI_LINE_MAX,
             "coin1 raw=0x%04x count=%u busy=0x%04x",
             st->jvs.coin[1].raw, (unsigned)st->jvs.coin[1].count,
             (unsigned)(st->jvs.coin[1].raw & MIE_JVS_COIN_BUSY));
    snprintf(lines[n++], UI_LINE_MAX,
             "wheel=%u accel=%u brake=%u",
             st->jvs.wheel, st->jvs.accel, st->jvs.brake);
    snprintf(lines[n++], UI_LINE_MAX,
             "outputs=0x%08lx (running light 0..%d)",
             (unsigned long)mie_jvs_get_outputs(),
             MIE_JVS_OUTPUT_COUNT - 1);

    if(mie_analog_calib_active()) {
        snprintf(lines[n++], UI_LINE_MAX, "CALIB: %s",
                 calib_step_name(mie_analog_calib_current()));
    }
    else if(mie_analog_calib_valid()) {
        snprintf(lines[n++], UI_LINE_MAX,
                 "calibrated (B/pad/kbd/mouse: recalibrate)");
    }
    else {
        snprintf(lines[n++], UI_LINE_MAX,
                 "not calibrated (B/pad/kbd/mouse: calibrate)");
    }

    snprintf(lines[n++], UI_LINE_MAX,
             "B=calib A=capture (MIE/pad/key/mouse)");
    snprintf(lines[n++], UI_LINE_MAX,
             "START/Esc/mouse side=exit");
    return n;
}

int main(int argc, char *argv[]) {
    mie_state_t *st;
    char lines[UI_ROW_MAX][UI_LINE_MAX];
    int count;
    uint32_t old_btns = 0;

    (void)argc;
    (void)argv;

    pvr_init_defaults();
    pvr_set_bg_color(0.0f, 0.0f, 0.0f);
    ui_font_init();

    mie_jvs_callback(MIE_JVS_IN_COIN1, MIE_JVS_COIN_INSERT_BIT, mie_coin_cb);
    mie_jvs_callback(MIE_JVS_IN_COIN2, MIE_JVS_COIN_INSERT_BIT, mie_coin_cb);
    mie_jvs_callback(MIE_JVS_IN_PANEL_PSW, MIE_JVS_PANEL_SERVICE_BIT, mie_service_cb);
    mie_jvs_callback(MIE_JVS_IN_P1, MIE_JVS_SERVICE_BIT, mie_service_cb);

    for(;;) {
        int quit = 0;
        uint32_t btns = ui_poll_aux_buttons();

        outputs_blink();

        st = NULL;

        MAPLE_FOREACH_BEGIN(MAPLE_FUNC_MIE, mie_state_t, ms)
            if(ms) {
                st = ms;
                btns |= ms->cont.buttons;

                if((btns & CONT_B) && !(old_btns & CONT_B) &&
                        !mie_analog_calib_active()) {
                    mie_analog_calib_start();
                }

                if((btns & CONT_A) && !(old_btns & CONT_A) &&
                        mie_analog_calib_active()) {
                    mie_analog_calib_capture();
                }

                if((btns & CONT_START) && !(old_btns & CONT_START)) {
                    quit = 1;
                }
            }
        MAPLE_FOREACH_END()

        if(!st && (btns & CONT_START) && !(old_btns & CONT_START)) {
            quit = 1;
        }

        old_btns = btns;

        if(quit) {
            break;
        }

        count = ui_build(st, lines);
        ui_draw(lines, count);
    }

    pvr_mem_free(ui_font_tex);
    return 0;
}
