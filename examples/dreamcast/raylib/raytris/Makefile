#
# Raytris
# Copyright (C) 2024 Cole Hall
#   

TARGET = raytris.elf
OBJS = src/raytris.o romdisk.o src/grid/grid.o src/colors/colors.o src/position/position.o src/blocks/block.o src/constants/vmuIcons.o src/game/game.o src/sound/soundManager.o src/vmu/vmuManager.o
KOS_ROMDISK_DIR = romdisk

CPPFLAGS = -I${KOS_PORTS}/include/raylib
LDLIBS = -lraylib -lGL -lwav

include $(KOS_BASE)/Makefile.rules
