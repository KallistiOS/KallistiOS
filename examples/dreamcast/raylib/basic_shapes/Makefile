#raylib [shapes] example - Draw basic shapes 2d (rectangle, circle, line...)
#Copyright (c) 2014-2024 Ramon Santamaria (@raysan5)

TARGET = shapes.elf
OBJS = shapes.o

KOS_CFLAGS += -I${KOS_PORTS}/include/raylib

all: rm-elf $(TARGET)

include $(KOS_BASE)/Makefile.rules

clean: rm-elf
	-rm -f $(OBJS)

rm-elf:
	-rm -f $(TARGET)

$(TARGET): $(OBJS)
	kos-cc -o $(TARGET) $(OBJS) -lraylib -lGL -lm 

run: $(TARGET)
	$(KOS_LOADER) $(TARGET)

dist: $(TARGET)
	-rm -f $(OBJS)
	$(KOS_STRIP) $(TARGET)
