/* KallistiOS ##version##

   pvr_fog.c
   (C)2002 Paul Boese

 */

#include <assert.h>
#include <stdio.h>
#include <dc/pvr.h>
#include "pvr_internal.h"
#include "pvr_fog_tables.h"

/*

  This module handles things related to the hardware fog capabilities of the
  PVR chip in the DC. Thanks to Paul Boese for figuring this stuff out.

*/

/* note: this is my interpretation of stuffing the fog table
 *       in an attempt to satisfy the GL specs for GL_FOG_EXP
 *       GL_FOG_EXP2, and GL_LINEAR.
 *
 * With regard to GL_EXP...
 *
 * Two values specified by the GL spec: d=density, and z=(0,0,0,1)
 * use the formula f = exp(-(d*z)^2) where f is a value clamped
 * to the range [0..1]. f is used to blend a fog color value with post
 * textured pixels in the framebuffer.  In this code f is multiplied by
 * 255 so we can specify a range of values in each fog table entry from
 * 0..255. Where 0 = no fog,  and 255 = full opacity. There are 128
 * entries in the fog table each of which contains two 8 bit values. The
 * second or low byte is setup to be the same as the first or high byte
 * of the next entry (so that the fog density is continuous).
 *
 * sample fog table entries:
 *
 *   address    values
 *   0xa05f8200 0x0000fffd first entry - farthest from eye
 *   0xa05f8204 0x0000fdfa
 *   0xa05f8208 0x0000faf6
 *   0xa05f82c0 0x0000f6f0
 *   ...
 *
 * In Maiwe's docs there is register at 0xa05f80b8 called fog_density
 * that takes a weird floating point value.  From my experiments it seems
 * to work more like a far depth value for the table where index 1 resides
 * at the far depth and the last table index is at the frustum or eye. So it
 * would seem to specify how deep the fog is from the eye to a specified far
 * z distance.
 *
 * -----
 * According to Simon Fenney:
 *
 * There is a scale factor register which scales incoming per-pixel 1/w
 * values. This value is then clamped to the range 1.0 (maximum distance fog)
 * to 259.9999999 (minimum distance fg).
 *
 * (Remember that the with the original 1/w values, when 1/w == 1.0 USUALLY
 * implies the foreground clip plane distance while 1/w==0 is at infinity.
 * (Don't bother with far clipping, it's a waste of time and effort with a
 * floating point 1/w buffer))
 *
 * The fog table has 128 entries, each of which has two 8 bit fog values. Each
 * entry represents a range of (scaled) 1/w (i.e. perspective depth) values.
 * The second fog value of each pair is usually set up to be the same as the
 * first of the next (so that the fog density is continuous).
 *
 * The (first?) entries at index "i" in the table
 * represents the fog density at depth of:
 *
 * InverseW_Depth(i) = (pow(2.0, i>>4) * ((i&0xf) + 16)/16.0f)/FogScaleRegister
 *
 * The values you place in the table to assign fog densities is up to you
 * but the usual way is to use a power function like:
 *    (1.0 - pow(depth, SMALL_VALUE))
 * ------
 *
 * It appears that the FogScaleRegister is the same one referred to as
 * fog_density by the Maiwe docs. So what's being said here is that each
 * table value affects a range of 1/w depth. The FogScaleRegister controls
 * the maximum depth and the size of each 1/w depth segment.
 *
 * The density value d is easy to deal with, but the eye distance to the
 * center of a fragment z, where z=(0,0,0,1) in eye coords is a problem. The
 * z values are ones you get after applying the projection and modelview
 * matrix to the vertices, bot how do you relate those values to the 128 entries
 * in the fog table?
 *
 * After some experiments with the InverseW_Depth function above and several
 * days trying to load the fog table with values that correspond to the the
 * OpenGL fog types of GL_EXP, GL_EXP2 and GL_LINEAR a way to produce
 * perpectively correct fog table has been found.
 *
 * Two tables have been created that are used to scale fog z values so that
 * we get perspectively correct looking fog. One table can be used directly
 * with the linear fog table function.  The other table uses the same values
 * as the first only with each value multiplied by 259.999999.  That value
 * happens to correspond with the minimum fog distance. That table is used
 * by the EXP and EXP2 fog table functions.
 *
 * For the EXP and EXP2 fog functions the FogScaleRegister gets set with
 * the value of -259.999999.
 *
 * For LINEAR fog the FogScaleRegister is set the 'end' value. Where 'start'
 * and 'end' specify in world coordinates where the fog starts and ends.
 *
 * Please don't take any of this as the gospel truth, or final word on table
 * fog.  This module seems to work, but any corrections and insights that
 * would make it better would be appreciated.
 *
 * Special thanks goes to Simon Fenney for his explanation in the preceding
 * text and to Brian Paul (Mesa3D) for his fast negative exp function.
 *
 * Pb
 * ---
 *
 * Notes:
 *  A word about the fake alpha values fog_table_alpha.
 *  In my experiments with table fog I was could not seem to set an alpha
 *  value in the fog table color register. Therefore I created a fake one
 *  that effectively does the same thing as alpha.  We simple multiply
 *  the values to be loaded into the fog table registers by the alpha
 *  value to get the desired effect.
 *
 *      You should always specify the fog color before calling one of the
 *      three fog table functions.  This insures that the alpha value is set.
 *
 *      This module does NOT deal with VERTEX fog or TABLE COLOR (palette look
 *      up?) fog.
 *
 *      You should only call these functions outside of the pvr_scene_begin,
 *      pvr_scene_finish. If you call to change fog parameters while the pvr
 *      is rendering the scene you will get artifacts in the image.
 */

/* internal vars and stuff */
static float fog_alpha;

union ieee32_t {
    float flt32;
    uint32 uit32;
};

/* useful macros */
#define ABS(n) ( (n)<(0.0f) ? (-n):(n) )
#define NEG(n) ( (n)>(0.0f) ? (-n):(n) )
#define CLAMP01(x) \
    ( (x)<(0.0f) ? ((x) = (0.0f)) : ((x)>(1.0f) ? ((x)=(1.0f)) : (x)) )

/* helper functions */
#define FOG_EXP_TABLE_SIZE 256
#define FOG_MAX (10.0f)
#define EXP_FOG_MAX .0006595f
#define FOG_INCR (FOG_MAX/FOG_EXP_TABLE_SIZE)

/* A fast negative argument only exp function - That is it treats any
 * argument as a negative number
 */
static float neg_exp(float arg) {

    float result, f;
    int k;

    f = (float)(ABS(arg) * (1.0f / FOG_INCR));
    k = (int) f;

    if(k > FOG_EXP_TABLE_SIZE - 2)
        result = (float) EXP_FOG_MAX;
    else
        result = exp_table[k] + (f - k) * (exp_table[k + 1] - exp_table[k]);

    return result;
}

/* This function converts an IEEE 754 32-bit floating point number to
   16-bit normalized floating point number and returns it in the
   lower 16 bits of the 32 bit result.

                     1111111111222222222233
         0 12345678 90123456789012345678901
        |-|--------|-----------------------|
        |s|exponent|mantissa               | IEEE 32-bit float
   Bits |1|   8    |           23          |
        |-|--------|-----------------------|

        x = sign * (1).mantissa * (2 ** (exponent+127))

                     111111
         0 1234567 89012345
        |-|-------|--------|
        |1|mant.  |exponent| 16bit normalized float
   Bits |s| 7     |   8    |
        |-|-------|--------|

        x = sign * (1).mantissa * (2 ** exponent)

   Notes:
     The bias (exponent+127) used in the IEEE 32-bit float is subtracted
     before we repack the bits.

     The mantissa is truncated, but it might be better to round up the
     value instead.

     The special values for infinity and NaN are not dealt with at all.

 */
static uint32 float16(float f) {
    union ieee32_t float32;
    uint32 float16;
    uint32 sign, exponent, mantissa;

    float32.flt32 = f;
    sign = exponent = mantissa = float32.uit32;

    sign = (sign >> 16) & 0x8000;
    exponent = (((exponent >> 23) & 0xff) - 127) & 0xff;
    mantissa = (mantissa >> 8) & 0x7f00;
    float16 = sign | mantissa | exponent;
    return float16;
}

/* Set the fog table color */
void pvr_fog_table_color(float a, float r, float g, float b) {
    PVR_SET(PVR_FOG_TABLE_COLOR, PVR_PACK_COLOR(a, r, g, b));
    fog_alpha = a;
}

/* Set the fog vertex color */
void pvr_fog_vertex_color(float a, float r, float g, float b) {
    (void)a;
    (void)r;
    (void)g;
    (void)b;
    assert_msg(0, "not implemented");
    /*  PVR_SET(PVR_FOG_VERTEX_COLOR, PVR_PACK_COLOR(a, r, g, b)); */
}

/* Set the fog far depth */
void pvr_fog_far_depth(float d) {
    PVR_SET(PVR_FOG_DENSITY, float16(NEG(d)));
}

#define TABLE_LEN (PVR_FOG_TABLE_BASE + 0x200)

/* Initialize the fog table using an exp2 algorithm (like GL_EXP2) */
void pvr_fog_table_exp2(float density) {
    uint32 idx, i;
    uint32 value, valh, vall;
    float z =  259.999999f;
    float d2 = density * density;

    pvr_fog_far_depth(259.999999f);

    valh = (uint32)(fog_alpha * 255.0f * (1.0f - neg_exp(-(z * z * d2)))) & 0xff;

    for(idx = PVR_FOG_TABLE_BASE, i = 0; idx < TABLE_LEN; idx += 4, i++) {
        z = inverse_w_depth260[i];
        vall = (uint32)(fog_alpha * 255.0f * (1.0f - neg_exp(-(z * z * d2)))) & 0xff;
        value = (((valh) << 8 & 0xff00) + vall);
        PVR_SET(idx, value);
        valh = vall;
    }
}

/* Initialize the fog table using an exp algorithm (like GL_EXP) */
void pvr_fog_table_exp(float density) {
    uint32 idx, i;
    uint32 value, valh, vall;
    float z =  259.999999f;

    pvr_fog_far_depth(259.999999f);

    valh = (uint32)(fog_alpha * 255.0f * (1.0f - neg_exp(-(z * density)))) & 0xff;

    for(idx = PVR_FOG_TABLE_BASE, i = 0; idx < TABLE_LEN; idx += 4, i++) {
        z = inverse_w_depth260[i];
        vall = (uint32)(fog_alpha * 255.0f * (1.0f - neg_exp(-(z * density)))) & 0xff;
        value = (((valh) << 8 & 0xff00) + vall);
        PVR_SET(idx, value);
        valh = vall;
    }
}

/* Initialize the fog table using a linear algorithm (like GL_LINEAR)

         Formula for GL_FOG_LINEAR

             end - z           f is clamped [0,1]
         f = -----------       z eye distance to center of fragment
             end - start

   Truly linear entries in the fog table looks too heavy so we use
   a pre-calculated power table to get the z values and make a nice
   perspectively correct linear fog.
*/

void pvr_fog_table_linear(float start, float end) {
    uint32 idx, i, tdx;
    uint32 value, valh, vall;
    uint32 table_start, non_zero_entries, step_size;

    start = ABS(start);
    end = ABS(end);

    if(start >= end) {
        dbglog(DBG_DEBUG, "pvr_fog_table_linear: start >= end\n");
        return;
    }

    /* ratio of start/end times number of fog table entries */
    table_start = (uint32)((start / end) * 128.0f);
    /* number of entries to load with linear fog values */
    non_zero_entries = 128 - table_start;
    /* size of steps to use as we walk the inverse_w_depth table */
    step_size = 128 / non_zero_entries;

    pvr_fog_far_depth(end);

    valh = (uint32)(255.0f * fog_alpha); /* f=1/1  full occlusion */

    for(idx = PVR_FOG_TABLE_BASE, i = 0, tdx = 127; idx < TABLE_LEN; idx += 4, i += step_size, tdx--) {
        if(tdx >= table_start) {
            vall = (uint32)(inverse_w_depth[i] * 255.0f * fog_alpha);
        }
        else {
            vall = 0x0;
        }

        value = (((valh) << 8 & 0xff00) + vall);
        PVR_SET(idx, value);
        valh = vall;
    }
}

/* Set a custom fog table from float values
 *
 * Expects a table of floats with >>>129<<< entries with values
 * clamped 0..1. It is the responsibility of the programmer to call
 * pvr_fog_far_depth as part of proper fog table initialization.
 *
 * 0th entry is farthest from eye. Last entry is nearest to eye.
 * The larger the value the heavier the fog.
 */
void pvr_fog_table_custom(float *table) {
    uint32 idx, i;
    uint32 value, valh, vall;

    valh = (uint32)(fog_alpha * 255.0f * CLAMP01(table[0])) & 0xff;

    for(idx = PVR_FOG_TABLE_BASE, i = 1; idx < TABLE_LEN; idx += 4, i++) {
        vall = (uint32)(fog_alpha * 255.0f * CLAMP01(table[i])) & 0xff;
        value = (((valh) << 8 & 0xff00) + vall);
        PVR_SET(idx, value);
        valh = vall;
    }
}


