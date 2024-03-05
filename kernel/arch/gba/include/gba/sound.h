/* KallistiOS 2.0.0

   gba/sound.h
   (c)2002 Gil Megidish

*/

#ifndef __GBA_SOUND_H
#define __GBA_SOUND_H

#include <arch/types.h>

#define REG_SOUND1CNT_L (*(vu16*)0x04000060)
#define REG_SOUND1CNT_H (*(vu16*)0x04000062)
#define REG_SOUND1CNT_X (*(vu16*)0x04000064)
#define REG_SOUND2CNT_L (*(vu16*)0x04000068)
#define REG_SOUND2CNT_H (*(vu16*)0x0400006C)
#define REG_SOUND3CNT_L (*(vu16*)0x04000070)
#define REG_SOUND3CNT_H (*(vu16*)0x04000072)
#define REG_SOUND3CNT_X (*(vu16*)0x04000074)
#define REG_SOUND4CNT_L (*(vu16*)0x04000078)
#define REG_SOUND4CNT_H (*(vu16*)0x0400007C)

/* Master registers */
#define REG_SOUNDCNT_L  (*(vu16*)0x04000080)
#define REG_SOUNDCNT_H  (*(vu16*)0x04000082)
#define REG_SOUNDCNT_X  (*(vu16*)0x04000084)
#define REG_SOUNDBIAS   (*(vu16*)0x04000088)

#define PTR_WAVE_RAM    ((vu8*)0x0400008a)

#endif

