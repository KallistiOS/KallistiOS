# KallistiOS ##version##
#
# examples/dreamcast/library/loadable-dependent/Makefile
# Copyright (C) 2024 Ruslan Rostovtsev
#

TARGET_NAME = library-dependent
TARGET = $(TARGET_NAME).klf
OBJS = $(TARGET_NAME).o

# Dependence library and host stubs for link test
DBG_LIBS = -llibrary-dependence -llibrary-test
KOS_LIB_PATHS += -L../loadable-dependence -L../

# Dependence include
KOS_CFLAGS += -I../loadable-dependence

include $(KOS_BASE)/loadable/Makefile.prefab
