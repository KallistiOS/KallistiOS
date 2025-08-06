/* KallistiOS ##version##

   arch/dreamcast/include/arch/kos.h
   Copyright (C) 2025 Eric Fradella

*/

/** \file   arch/kos.h
    \brief  Include everything this arch implementation has to offer!

    This is the arch-specific implementation of kos.h, the universal header
    file that includes all of KallistiOS's functionality.

    This file is already included via the main kos.h, so there's no need
    to include it yourself.

    \author Eric Fradella
*/

#ifndef __ARCH_KOS_H
#define __ARCH_KOS_H

__BEGIN_DECLS

#include <arch/gdb.h>
#include <arch/mmu.h>
#include <arch/memory.h>
#include <arch/wdt.h>

#include <dc/asic.h>
#include <dc/biosfont.h>
#include <dc/cdrom.h>
#include <dc/fb_console.h>
#include <dc/flashrom.h>
#include <dc/fmath.h>
#include <dc/fs_dcload.h>
#include <dc/fs_dclsocket.h>
#include <dc/fs_iso9660.h>
#include <dc/fs_vmu.h>
#include <dc/g1ata.h>
#include <dc/g2bus.h>
#include <dc/maple.h>
#include <dc/maple/controller.h>
#include <dc/maple/dreameye.h>
#include <dc/maple/keyboard.h>
#include <dc/maple/mouse.h>
#include <dc/maple/purupuru.h>
#include <dc/maple/sip.h>
#include <dc/maple/vmu.h>
#include <dc/matrix3d.h>
#include <dc/matrix.h>
#include <dc/modem/modem.h>
#include <dc/net/broadband_adapter.h>
#include <dc/net/lan_adapter.h>
#include <dc/perfctr.h>
#include <dc/pvr.h>
#include <dc/scif.h>
#include <dc/sci.h>
#include <dc/sd.h>
#include <dc/sound/stream.h>
#include <dc/sound/sfxmgr.h>
#include <dc/spu.h>
#include <dc/sq.h>
#include <dc/ubc.h>
#include <dc/vblank.h>
#include <dc/vec3f.h>
#include <dc/video.h>
#include <dc/vmu_fb.h>
#include <dc/vmu_pkg.h>
#include <dc/vmufs.h>

__END_DECLS

#endif
