/* KallistiOS ##version##

   mie_calib.c
   Copyright (C) 2026 Ruslan Rostovtsev

   Wheel/pedal calibration: track raw min/max during a wizard, then map
   JVS 16-bit values to cont_state ranges (-128..127 wheel, 0..255 pedals).
   Without valid calib data, legacy fixed-scale conversion is used.
*/

#include <string.h>

#include <dc/maple/mie.h>

#include "mie_internal.h"

static mie_analog_calib_t active_calib;
static mie_analog_calib_t work_calib;
static mie_analog_calib_step_t step_calib;
static bool active_calib_valid;
static uint16_t last_wheel;
static uint16_t last_accel;
static uint16_t last_brake;

static uint16_t mie_analog_raw_clean(uint16_t v) {
    return v & MIE_JVS_ANALOG_RAW_MASK;
}

/* Fixed center at 0x8000, scale to -128..127 (used when no calib stored). */
static int mie_analog_legacy_wheel(uint16_t v) {
    int32_t centered;

    v = mie_analog_raw_clean(v);
    centered = (int32_t)v - 0x8000;
    centered = centered * 127 / 0x8000;
    if(centered < -128) {
        return -128;
    }
    if(centered > 127) {
        return 127;
    }
    return (int)centered;
}

static int mie_analog_legacy_pedal(uint16_t v) {
    v = mie_analog_raw_clean(v);
    return (int)(v >> 8);
}

static int mie_analog_apply_wheel_calib(uint16_t raw) {
    const mie_analog_axis_calib_t *a = &active_calib.wheel;
    int32_t v;

    raw = mie_analog_raw_clean(raw);

    if(raw <= a->center) {
        if(a->center == a->min) {
            return -128;
        }
        v = (int32_t)(raw - a->center) * 128 / (int32_t)(a->center - a->min);
        if(v < -128) {
            return -128;
        }
        return (int)v;
    }

    if(a->max == a->center) {
        return 127;
    }
    v = (int32_t)(raw - a->center) * 127 / (int32_t)(a->max - a->center);
    if(v > 127) {
        return 127;
    }
    return (int)v;
}

static int mie_analog_apply_pedal_calib(uint16_t raw,
                                        const mie_analog_axis_calib_t *a) {
    int32_t v;

    raw = mie_analog_raw_clean(raw);

    if(a->max <= a->min) {
        return mie_analog_legacy_pedal(raw);
    }
    if(raw <= a->min) {
        return 0;
    }
    if(raw >= a->max) {
        return 255;
    }

    v = (int32_t)(raw - a->min) * 255 / (int32_t)(a->max - a->min);
    return (int)v;
}

static bool mie_analog_calib_ranges_valid(const mie_analog_calib_t *c) {
    return c->wheel.min < c->wheel.center
        && c->wheel.center < c->wheel.max
        && c->accel.min < c->accel.max
        && c->brake.min < c->brake.max;
}

static void mie_analog_set_active(const mie_analog_calib_t *calib) {
    active_calib = *calib;
    active_calib_valid = mie_analog_calib_ranges_valid(&active_calib);
}

bool mie_analog_calib_valid(void) {
    return active_calib_valid;
}

int mie_analog_norm_wheel(uint16_t raw) {
    if(active_calib_valid) {
        return mie_analog_apply_wheel_calib(raw);
    }
    return mie_analog_legacy_wheel(raw);
}

int mie_analog_norm_accel(uint16_t raw) {
    if(active_calib_valid) {
        return mie_analog_apply_pedal_calib(raw, &active_calib.accel);
    }
    return mie_analog_legacy_pedal(raw);
}

int mie_analog_norm_brake(uint16_t raw) {
    if(active_calib_valid) {
        return mie_analog_apply_pedal_calib(raw, &active_calib.brake);
    }
    return mie_analog_legacy_pedal(raw);
}

int mie_analog_map_wheel(uint16_t raw) {
    /* During calib wizard return raw values so the UI can display them. */
    if(step_calib != MIE_ANALOG_CALIB_IDLE) {
        return (int)mie_analog_raw_clean(raw);
    }
    return mie_analog_norm_wheel(raw);
}

int mie_analog_map_accel(uint16_t raw) {
    if(step_calib != MIE_ANALOG_CALIB_IDLE) {
        return (int)mie_analog_raw_clean(raw);
    }
    return mie_analog_norm_accel(raw);
}

int mie_analog_map_brake(uint16_t raw) {
    if(step_calib != MIE_ANALOG_CALIB_IDLE) {
        return (int)mie_analog_raw_clean(raw);
    }
    return mie_analog_norm_brake(raw);
}

static void mie_analog_track_axis(mie_analog_axis_calib_t *axis, uint16_t raw) {
    raw = mie_analog_raw_clean(raw);

    if(axis->min == 0xFFFF && axis->max == 0) {
        axis->min = raw;
        axis->max = raw;
        return;
    }
    if(raw < axis->min) {
        axis->min = raw;
    }
    if(raw > axis->max) {
        axis->max = raw;
    }
}

/* Record min/max (or center) while the user moves one axis per calib step. */
void mie_analog_track(const mie_jvs_inputs_t *jvs) {
    if(step_calib == MIE_ANALOG_CALIB_IDLE) {
        return;
    }

    last_wheel = mie_analog_raw_clean(jvs->wheel);
    last_accel = mie_analog_raw_clean(jvs->accel);
    last_brake = mie_analog_raw_clean(jvs->brake);

    switch(step_calib) {
        case MIE_ANALOG_CALIB_WHEEL:
            mie_analog_track_axis(&work_calib.wheel, jvs->wheel);
            break;
        case MIE_ANALOG_CALIB_ACCEL:
            mie_analog_track_axis(&work_calib.accel, jvs->accel);
            break;
        case MIE_ANALOG_CALIB_BRAKE:
            mie_analog_track_axis(&work_calib.brake, jvs->brake);
            break;
        default:
            break;
    }
}

const mie_analog_calib_t *mie_analog_calib_get(void) {
    return &active_calib;
}

void mie_analog_calib_set(const mie_analog_calib_t *calib) {
    if(!calib) {
        return;
    }
    mie_analog_set_active(calib);
}

void mie_analog_calib_reset(void) {
    memset(&active_calib, 0, sizeof(active_calib));
    active_calib_valid = false;
    step_calib = MIE_ANALOG_CALIB_IDLE;
}

void mie_analog_calib_start(void) {
    memset(&work_calib, 0, sizeof(work_calib));
    work_calib.wheel.min = 0xFFFF;
    work_calib.accel.min = 0xFFFF;
    work_calib.brake.min = 0xFFFF;
    last_wheel = 0;
    last_accel = 0;
    last_brake = 0;
    /* User moves wheel full range first, then captures center on next step. */
    step_calib = MIE_ANALOG_CALIB_WHEEL;
}

void mie_analog_calib_cancel(void) {
    step_calib = MIE_ANALOG_CALIB_IDLE;
}

bool mie_analog_calib_active(void) {
    return step_calib != MIE_ANALOG_CALIB_IDLE;
}

mie_analog_calib_step_t mie_analog_calib_current(void) {
    return step_calib;
}

/* Wizard: wheel extremes -> wheel center -> accel -> brake -> commit. */
bool mie_analog_calib_capture(void) {
    switch(step_calib) {
        case MIE_ANALOG_CALIB_WHEEL:
            step_calib = MIE_ANALOG_CALIB_WHEEL_CENTER;
            return false;
        case MIE_ANALOG_CALIB_WHEEL_CENTER:
            work_calib.wheel.center = last_wheel;
            step_calib = MIE_ANALOG_CALIB_ACCEL;
            return false;
        case MIE_ANALOG_CALIB_ACCEL:
            step_calib = MIE_ANALOG_CALIB_BRAKE;
            return false;
        case MIE_ANALOG_CALIB_BRAKE:
            if(mie_analog_calib_ranges_valid(&work_calib)) {
                mie_analog_set_active(&work_calib);
            }
            step_calib = MIE_ANALOG_CALIB_IDLE;
            return true;
        default:
            return false;
    }
}
