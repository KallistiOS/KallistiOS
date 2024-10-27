#raylib [models] example - Cubicmap loading and drawing
#Copyright (c) 2015-2024 Ramon Santamaria (@raysan5)


TARGET = cubicmap.elf
OBJS = cubicmap.o romdisk.o
KOS_ROMDISK_DIR = romdisk

KOS_CFLAGS += -I${KOS_PORTS}/include/raylib

all: rm-elf $(TARGET)

include $(KOS_BASE)/Makefile.rules

clean: rm-elf
	-rm -f $(OBJS)

rm-elf:
	-rm -f $(TARGET) romdisk.*

$(TARGET): $(OBJS)
	kos-cc -o $(TARGET) $(OBJS) -lraylib -lGL -lm -lkosutils 

run: $(TARGET)
	$(KOS_LOADER) $(TARGET)

dist: $(TARGET)
	-rm -f $(OBJS) romdisk.img
	$(KOS_STRIP) $(TARGET)

