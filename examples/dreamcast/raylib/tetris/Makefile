#
# Tetris
# Copyright (C) 2024 Cole Hall
#   

TARGET = tetris.elf
OBJS = src/tetris.o romdisk.o src/grid/grid.o src/colors/colors.o src/position/position.o src/blocks/block.o src/constants/vmuIcons.o src/game/game.o src/sound/soundManager.o src/vmu/vmuManager.o
KOS_ROMDISK_DIR = romdisk

KOS_CFLAGS += -I${KOS_PORTS}/include/raylib

all: rm-elf $(TARGET)

include $(KOS_BASE)/Makefile.rules

clean: rm-elf
	-rm -f $(OBJS)

rm-elf:
	-rm -f $(TARGET) romdisk.*

$(TARGET): $(OBJS)
	kos-c++ -o $(TARGET) $(OBJS) -lraylib -lGL -lwav

run: $(TARGET)
	$(KOS_LOADER) $(TARGET)

dist: $(TARGET)
	-rm -f $(OBJS) romdisk.img
	$(KOS_STRIP) $(TARGET)
