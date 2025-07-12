/* KallistiOS ##version##

   dc/pvr/pvr_regs.h
   Copyright (C) 2002 Megan Potter
   Copyright (C) 2014 Lawrence Sebald
   Copyright (C) 2023 Ruslan Rostovtsev
   Copyright (C) 2024 Falco Girgis
*/

/** \file       dc/pvr/pvr_regs.h
    \brief      PVR Driver Registers
    \ingroup    pvr_registers

    This file provides the low-level driver implementation details for the
    PowerVR API, including its memory map and register accessors.

    \author Megan Potter
    \author Roger Cattermole
    \author Paul Boese
    \author Brian Paul
    \author Lawrence Sebald
    \author Benoit Miller
    \author Ruslan Rostovtsev
    \author Falco Girgis
*/

#ifndef __DC_PVR_PVR_REGS_H
#define __DC_PVR_PVR_REGS_H

#include <sys/cdefs.h>
__BEGIN_DECLS

#include <kos/platform.h>

/**** Register macros ***************************************************/

/** \defgroup pvr_registers         Registers
    \brief                          Direct PVR register and memory access
    \ingroup                        pvr
    @{
*/

/* We use these macros to do all PVR register access, so that it's
   simple later on to hook them for debugging or whatnot. */

/** \brief   Retrieve a PVR register value

    \param   REG             The register to fetch. See \ref pvr_regs.
    
    \return                  The value of that register (32-bits)
*/
#define PVR_GET(REG) (* ( (vuint32*)( 0xa05f8000 + (REG) ) ) )

/** \brief   Set a PVR register value

    \param   REG             The register to set. See \ref pvr_regs.
    \param   VALUE           The value to set in the register (32-bits)
*/
#define PVR_SET(REG, VALUE) PVR_GET(REG) = (VALUE)

/** @} */

/** \defgroup pvr_regs   Offsets
    \brief               PowerVR register offsets
    \ingroup             pvr_registers

    The registers themselves; these are from Maiwe's powervr-reg.txt.

    \note
    2D specific registers have been excluded for now (like
    vsync, hsync, v/h size, etc) 

    @{
*/

#define PVR_ID                  0x0000  /**< \brief Chip ID */
#define PVR_REVISION            0x0004  /**< \brief Chip revision */
#define PVR_RESET               0x0008  /**< \brief Reset pins */

#define PVR_ISP_START           0x0014  /**< \brief Start the ISP/TSP */
#define PVR_UNK_0018            0x0018  /**< \brief ?? */

#define PVR_ISP_VERTBUF_ADDR    0x0020  /**< \brief Vertex buffer address for scene rendering */

#define PVR_ISP_TILEMAT_ADDR    0x002c  /**< \brief Tile matrix address for scene rendering */
#define PVR_SPANSORT_CFG        0x0030  /**< \brief ?? -- write 0x101 for now */

#define PVR_BORDER_COLOR        0x0040  /**< \brief Border Color in RGB888 */
#define PVR_FB_CFG_1            0x0044  /**< \brief Framebuffer config 1 */
#define PVR_FB_CFG_2            0x0048  /**< \brief Framebuffer config 2 */
#define PVR_RENDER_MODULO       0x004c  /**< \brief Render modulo */
#define PVR_FB_ADDR             0x0050  /**< \brief Framebuffer start address */
#define PVR_FB_IL_ADDR          0x0054  /**< \brief Framebuffer odd-field start address for interlace */

#define PVR_FB_SIZE             0x005c  /**< \brief Framebuffer display size */
#define PVR_RENDER_ADDR         0x0060  /**< \brief Render output address */
#define PVR_RENDER_ADDR_2       0x0064  /**< \brief Output for strip-buffering */
#define PVR_PCLIP_X             0x0068  /**< \brief Horizontal clipping area */
#define PVR_PCLIP_Y             0x006c  /**< \brief Vertical clipping area */

#define PVR_CHEAP_SHADOW        0x0074  /**< \brief Cheap shadow control */
#define PVR_OBJECT_CLIP         0x0078  /**< \brief Distance for polygon culling */
#define PVR_UNK_007C            0x007c  /**< \brief ?? -- write 0x0027df77 for now */
#define PVR_UNK_0080            0x0080  /**< \brief ?? -- write 7 for now */
#define PVR_TEXTURE_CLIP        0x0084  /**< \brief Distance for texture clipping */
#define PVR_BGPLANE_Z           0x0088  /**< \brief Distance for background plane */
#define PVR_BGPLANE_CFG         0x008c  /**< \brief Background plane config */

#define PVR_UNK_0098            0x0098  /**< \brief ?? -- write 0x00800408 for now */

#define PVR_UNK_00A0            0x00a0  /**< \brief ?? -- write 0x20 for now */

#define PVR_UNK_00A8            0x00a8  /**< \brief ?? -- write 0x15d1c951 for now */

#define PVR_FOG_TABLE_COLOR     0x00b0  /**< \brief Table fog color */
#define PVR_FOG_VERTEX_COLOR    0x00b4  /**< \brief Vertex fog color */
#define PVR_FOG_DENSITY         0x00b8  /**< \brief Fog density coefficient */
#define PVR_COLOR_CLAMP_MAX     0x00bc  /**< \brief RGB Color clamp max */
#define PVR_COLOR_CLAMP_MIN     0x00c0  /**< \brief RGB Color clamp min */
#define PVR_GUN_POS             0x00c4  /**< \brief Light gun position */
#define PVR_HPOS_IRQ            0x00c8  /**< \brief Horizontal position IRQ */
#define PVR_VPOS_IRQ            0x00cc  /**< \brief Vertical position IRQ */
#define PVR_IL_CFG              0x00d0  /**< \brief Interlacing config */
#define PVR_BORDER_X            0x00d4  /**< \brief Window border X position */
#define PVR_SCAN_CLK            0x00d8  /**< \brief Clock and scanline values */
#define PVR_BORDER_Y            0x00dc  /**< \brief Window border Y position */

#define PVR_TEXTURE_MODULO      0x00e4  /**< \brief Output texture width modulo */
#define PVR_VIDEO_CFG           0x00e8  /**< \brief Misc video config */
#define PVR_BITMAP_X            0x00ec  /**< \brief Bitmap window X position */
#define PVR_BITMAP_Y            0x00f0  /**< \brief Bitmap window Y position */
#define PVR_SCALER_CFG          0x00f4  /**< \brief Smoothing scaler */

#define PVR_PALETTE_CFG         0x0108  /**< \brief Palette format */
#define PVR_SYNC_STATUS         0x010c  /**< \brief V/H blank status */
#define PVR_UNK_0110            0x0110  /**< \brief ?? -- write 0x93f39 for now */
#define PVR_UNK_0114            0x0114  /**< \brief ?? -- write 0x200000 for now */
#define PVR_UNK_0118            0x0118  /**< \brief ?? -- write 0x8040 for now */

#define PVR_TA_OPB_START        0x0124  /**< \brief Object Pointer Buffer start for TA usage */
#define PVR_TA_VERTBUF_START    0x0128  /**< \brief Vertex buffer start for TA usage */
#define PVR_TA_OPB_END          0x012c  /**< \brief OPB end for TA usage */
#define PVR_TA_VERTBUF_END      0x0130  /**< \brief Vertex buffer end for TA usage */
#define PVR_TA_OPB_POS          0x0134  /**< \brief Top used memory location in OPB for TA usage */
#define PVR_TA_VERTBUF_POS      0x0138  /**< \brief Top used memory location in vertbuf for TA usage */
#define PVR_TILEMAT_CFG         0x013c  /**< \brief Tile matrix size config */
#define PVR_OPB_CFG             0x0140  /**< \brief Active lists / list size */
#define PVR_TA_INIT             0x0144  /**< \brief Initialize vertex reg. params */
#define PVR_YUV_ADDR            0x0148  /**< \brief YUV conversion destination */
#define PVR_YUV_CFG             0x014c  /**< \brief YUV configuration */
#define PVR_YUV_STAT            0x0150  /**< \brief The number of YUV macroblocks converted */

#define PVR_UNK_0160            0x0160  /**< \brief ?? */
#define PVR_TA_OPB_INIT         0x0164  /**< \brief Object pointer buffer position init */

#define PVR_FOG_TABLE_BASE      0x0200  /**< \brief Base of the fog table */

#define PVR_PALETTE_TABLE_BASE  0x1000  /**< \brief Base of the palette table */
/** @} */

/** \defgroup pvr_addresses     Addresses and Constants
    \brief                      Miscellaneous Addresses and Constants
    \ingroup                    pvr_registers

    Useful PVR memory locations and values.

    @{
*/
#define PVR_TA_INPUT        0x10000000  /**< \brief TA command input (64-bit, TA) */
#define PVR_TA_YUV_CONV     0x10800000  /**< \brief YUV converter (64-bit, TA) */
#define PVR_TA_TEX_MEM      0x11000000  /**< \brief VRAM 64-bit, TA=>VRAM */
#define PVR_TA_TEX_MEM_32   0x13000000  /**< \brief VRAM 32-bit, TA->VRAM */
#define PVR_RAM_BASE_32_P0  0x05000000  /**< \brief VRAM 32-bit, P0 area, PVR->VRAM */
#define PVR_RAM_BASE_64_P0  0x04000000  /**< \brief VRAM 64-bit, P0 area, PVR->VRAM */
#define PVR_RAM_BASE        0xa5000000  /**< \brief VRAM 32-bit, P2 area, PVR->VRAM */
#define PVR_RAM_INT_BASE    0xa4000000  /**< \brief VRAM 64-bit, P2 area, PVR->VRAM */

#define PVR_RAM_SIZE_MB     (KOS_PLATFORM_IS_NAOMI ? 16 : 8)    /**< \brief RAM size in MiB */
#define PVR_RAM_SIZE        (PVR_RAM_SIZE_MB*1024*1024)         /**< \brief RAM size in bytes */

#define PVR_RAM_TOP         (PVR_RAM_BASE + PVR_RAM_SIZE)       /**< \brief Top of raw PVR RAM */
#define PVR_RAM_INT_TOP     (PVR_RAM_INT_BASE + PVR_RAM_SIZE)   /**< \brief Top of int PVR RAM */
/** @} */

/* Register content defines, as needed; these will be filled in over time
   as the implementation requires them. There's too many to do otherwise. */

/** \defgroup pvr_reset_vals        Reset Values
    \brief                          Values used to reset parts of the PVR
    \ingroup                        pvr_registers

    These values are written to the PVR_RESET register in order to reset the
    system or to take it out of reset.

    @{
*/
#define PVR_RESET_ALL       0xffffffff  /**< \brief Reset the whole PVR */
#define PVR_RESET_NONE      0x00000000  /**< \brief Cancel reset state */
#define PVR_RESET_TA        0x00000001  /**< \brief Reset only the TA */
#define PVR_RESET_ISPTSP    0x00000002  /**< \brief Reset only the ISP/TSP */
/** @} */

/** \defgroup pvr_go        Init/Start Values
    \brief                  Values to be written to registers to conform or start operations.
    \ingroup                pvr_registers
    @{
*/
#define PVR_ISP_START_GO    0xffffffff  /**< \brief Write to the PVR_ISP_START register to start rendering */

#define PVR_TA_INIT_GO      0x80000000  /**< \brief Write to the PVR_TA_INIT register to confirm settings */
/** @} */


__END_DECLS

#endif  /* __DC_PVR_PVR_REGS_H */
