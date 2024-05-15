/* KallistiOS 2.0.0

   gba/video.h
   (c)2001 Dan Potter

*/

/* Thanks to Dovoto's tutorial for these numbers */

#ifndef __GBA_VIDEO_H
#define __GBA_VIDEO_H

#include <arch/types.h>

/* Display mode register */
#define REG_DISPCNT (*(vu16*)0x04000000)

/* Display status register (vsync) */
#define REG_DISPSTAT    (*(vu16*)0x04000004)

/* Scanline counter */
#define REG_VCOUNT  (*(vu16*)0x04000006)

/* Background controls */
#define REG_BG0CNT  (*(vu16*)0x04000008)
#define REG_BG1CNT  (*(vu16*)0x0400000a)
#define REG_BG2CNT  (*(vu16*)0x0400000c)
#define REG_BG3CNT  (*(vu16*)0x0400000e)

/* Background offsets */
#define REG_BG0HOFS (*(vu16*)0x04000010)
#define REG_BG0VOFS (*(vu16*)0x04000012)
#define REG_BG1HOFS (*(vu16*)0x04000014)
#define REG_BG1VOFS (*(vu16*)0x04000016)
#define REG_BG2HOFS (*(vu16*)0x04000018)
#define REG_BG2VOFS (*(vu16*)0x0400001a)
#define REG_BG3HOFS (*(vu16*)0x0400001c)
#define REG_BG3VOFS (*(vu16*)0x0400001e)

/* The two display buffers */
#define PTR_VID_0   ((vu16*)0x06000000)
#define PTR_VID_1   ((vu16*)0x0600A000) /* Valid only in Mode 4 */

/* Background palete memory (in indexed mode) */
#define PTR_BG_PAL  ((vu16*)0x05000000)

/* Basic display mode ("background type") */
#define DISP_MODE_0     0x0000
#define DISP_MODE_1     0x0001
#define DISP_MODE_2     0x0002
#define DISP_MODE_3     0x0003
#define DISP_MODE_4     0x0004
#define DISP_MODE_5     0x0005

/* Misc options */
#define DISP_BUFFER_0       0x0000
#define DISP_BUFFER_1       0x0010
#define DISP_HBLANK_OAM     0x0020
#define DISP_OBJ_2D     0x0000
#define DISP_OBJ_1D     0x0040
#define DISP_FORCE_BLANK    0x0080

/* Tile layer enable/disable */
#define DISP_BG0_DISABLE    0x0000
#define DISP_BG0_ENABLE     0x0100
#define DISP_BG1_DISABLE    0x0000
#define DISP_BG1_ENABLE     0x0200
#define DISP_BG2_DISABLE    0x0000
#define DISP_BG2_ENABLE     0x0400
#define DISP_BG3_DISABLE    0x0000
#define DISP_BG3_ENABLE     0x0800

/* Sprite enable/disable */
#define DISP_OBJ_ENABLE     0x1000
#define DISP_OBJ_DISABLE    0x0000
#define DISP_WIN1_ENABLE    0x2000
#define DISP_WIN1_DISABLE   0x0000
#define DISP_WIN2_ENABLE    0x4000
#define DISP_WIN2_DISABLE   0x0000
#define DISP_WINOBJ_ENABLE  0x8000
#define DISP_WINOBJ_DISABLE 0x0000

/* Display status flags (DISPSTAT) */
#define DISP_VBLANK     0x0001
#define DISP_HBLANK     0x0002
#define DISP_VCOUNTER       0x0004
#define DISP_VBLANK_IRQ     0x0008
#define DISP_HBLANK_IRQ     0x0010
#define DISP_VCOUNTER_IRQ   0x0020

/* Shifts required before OR'ing value with DISPSTAT */
#define DISP_VCOUNTER_SHIFT 0x0008

#endif  /* __GBA_VIDEO_H */
