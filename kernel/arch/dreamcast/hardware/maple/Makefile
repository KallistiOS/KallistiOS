# KallistiOS ##version##
#
# arch/dreamcast/hardware/maple/Makefile
# Copyright (C) 2000-2001 Megan Potter
#

# Core maple handling stuff
OBJS := maple_driver.o maple_enum.o maple_globals.o
OBJS := $(OBJS) maple_init_shutdown.o maple_irq.o
OBJS := $(OBJS) maple_queue.o maple_utils.o

# Various input devices
OBJS := $(OBJS) controller.o keyboard.o mouse.o
OBJS := $(OBJS) purupuru.o sip.o dreameye.o lightgun.o

# Output devices
OBJS := $(OBJS) vmu.o

SUBDIRS =

include $(KOS_BASE)/Makefile.prefab
