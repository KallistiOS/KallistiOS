/* KallistiOS 2.0.0

   gba/sprite.h
   (c)2001 Dan Potter

*/

/* Thanks to Dovoto's tutorial for these numbers */

#ifndef __GBA_SPRITE_H
#define __GBA_SPRITE_H

#include <arch/types.h>

/* Object Attribute Memory */
#define PTR_OAM     ((vu16*)0x07000000)

/* Object data memory */
#define PTR_OBJ_DATA    ((vu16*)0x06010000)

/* Object palette memory */
#define PTR_OBJ_PAL ((vu16*)0x05000200)

/* I chose to make two seperate structures here because using a union is
   probably going to result in awful looking code, and the common case
   is going to be #2 anyway... */

/* Object Attribute entry definition (bitfield version) */
typedef struct {
    /* Attribute 0 */
    u16 y: 9;       /* Y coordinate */
    u16 r: 1;       /* Rotation / Scaling flag */
    u16 sd: 1;      /* Size Double */
    u16 mode: 2;    /* Mode / Alpha */
    u16 m: 1;       /* Mosaic flag */
    u16 c: 1;       /* Color mode */
    u16 shape: 2;   /* Object shape */

    /* Attribute 1 */
    u16 x: 9;       /* X coordinate */
    u16 rdi: 5;     /* Rotation data index / flip flags*/
    u16 size: 2;    /* Size */

    /* Attribute 2 */
    u16 name: 10;   /* Character name / sprite index */
    u16 prio: 2;    /* Layer priority */
    u16 pal: 4;     /* Palette index */

    /* Attribute 3 */
    u16 attr3;      /* ? Set to zero */
} __attribute__((packed)) oam_entry_bf;

/* Object Attribute entry definition (raw version) */
typedef struct {
    u16 attr0, attr1, attr2, attr3;
} __attribute__((packed)) oam_entry_t;

/* Attribute 0 constants */
#define SPR_ROTATION    0x0100
#define SPR_SIZE_DOUBLE 0x0200
#define SPR_MODE_NORMAL 0x0000
#define SPR_MODE_TRANS  0x0400
#define SPR_MODE_WND    0x0800
#define SPR_MOSAIC  0x1000
#define SPR_COLOR_16    0x0000
#define SPR_COLOR_256   0x2000
#define SPR_SQUARE  0x0000
#define SPR_TALL    0x4000
#define SPR_WIDE    0x8000

/* Attribute 1 constants */
#define SPR_ROT_DATA(n) ((n) << 9)
#define SPR_H_FLIP  0x1000
#define SPR_V_FLIP  0x2000
#define SPR_SIZE_8  0x0000
#define SPR_SIZE_16 0x4000
#define SPR_SIZE_32 0x8000
#define SPR_SIZE_64 0xC000

/* Attribute 2 constants */
#define SPR_PRIO(n) ((n) << 10)
#define SPR_PAL(n)  ((n) << 12)

#endif  /* __GBA_SPRITE_H */

