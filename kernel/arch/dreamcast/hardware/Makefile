# KallistiOS ##version##
#
# arch/dreamcast/hardware/Makefile
# (c)2000-2001 Megan Potter
#

# Init wrapper
OBJS = hardware.o

# BIOS services
ifneq ($(KOS_SUBARCH), naomi)
	OBJS += biosfont.o cdrom.o flashrom.o
endif

# System Calls
OBJS += syscalls.o syscall_font.o

# G2
OBJS += g2dma.o

# Sound
OBJS += spu.o

# Bus support
OBJS += asic.o g2bus.o

# Video-related
OBJS += video.o vblank.o

# CPU-related
OBJS += sq.o sq_fast_cpy.o scif.o sci.o ubc.o dmac.o reverse_bits.o

# SPI device support
OBJS += scif-spi.o sd.o

# G1 Bus ATA support
ifneq ($(KOS_SUBARCH), naomi)
	OBJS += g1ata.o
endif

SUBDIRS = pvr maple

ifneq ($(KOS_SUBARCH), naomi)
	SUBDIRS += network modem
endif

include $(KOS_BASE)/Makefile.prefab
