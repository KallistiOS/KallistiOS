/* KallistiOS 2.0.0

   gba/dma.h
   (c)2002 Gil Megidish

*/

#ifndef __GBA_DMA_H
#define __GBA_DMA_H

#include <arch/types.h>

/* There are four dma channels; All source addresses and destination
 * addresses are 24bit only. Consult proper documentation about
 * the properties of each channel.
 */

#define REG_DMA0SAD (*(vu32*)0x040000b0)
#define REG_DMA0DAD (*(vu32*)0x040000b4)
#define REG_DMA0CNT_L   (*(vu16*)0x040000b8)
#define REG_DMA0CNT_H   (*(vu16*)0x040000ba)
#define REG_DMA1SAD (*(vu32*)0x040000bc)
#define REG_DMA1DAD (*(vu32*)0x040000c0)
#define REG_DMA1CNT_L   (*(vu16*)0x040000c4)
#define REG_DMA1CNT_H   (*(vu16*)0x040000c6)
#define REG_DMA2SAD (*(vu32*)0x040000c8)
#define REG_DMA2DAD (*(vu32*)0x040000cc)
#define REG_DMA2CNT_L   (*(vu16*)0x040000d0)
#define REG_DMA2CNT_H   (*(vu16*)0x040000d2)
#define REG_DMA3SAD (*(vu32*)0x040000d4)
#define REG_DMA3DAD (*(vu32*)0x040000d8)
#define REG_DMA3CNT_L   (*(vu16*)0x040000dc)
#define REG_DMA3CNT_H   (*(vu16*)0x040000de)

#endif

