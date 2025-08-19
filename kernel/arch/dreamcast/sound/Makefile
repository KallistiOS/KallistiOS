# KallistiOS ##version##
#
# libmp3/Makefile
# (c)2001 Megan Potter
# (c)2023 Ruslan Rostovtsev
#

OBJS = snd_iface.o \
	snd_sfxmgr.o \
	snd_stream.o \
	snd_stream_drv.o \
	snd_mem.o \
	snd_pcm_split.o

KOS_CFLAGS += -I $(KOS_BASE)/kernel/arch/dreamcast/include/dc/sound

SUBDIRS = arm

include $(KOS_BASE)/Makefile.prefab

arm/stream.drv: subdirs

snd_iface.c: snd_stream_drv.c

snd_stream_drv.c: arm/stream.drv $(KOS_BASE)/utils/bin2c/bin2c
	$(KOS_BASE)/utils/bin2c/bin2c $< $@ snd_stream_drv

clean:
	-rm -f snd_stream_drv.c
