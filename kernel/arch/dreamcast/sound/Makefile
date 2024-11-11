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

snd_stream_drv.o: arm/stream.drv
	$(KOS_BASE)/utils/bin2o/bin2o arm/stream.drv snd_stream_drv snd_stream_drv.o
