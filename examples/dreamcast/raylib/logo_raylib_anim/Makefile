#raylib [shapes] example - raylib logo animation
#Copyright (c) 2014-2023 Ramon Santamaria (@raysan5)

TARGET = logo.elf
OBJS = logo.o

all: rm-elf $(TARGET)

KOS_CFLAGS += -I${KOS_PORTS}/include/raylib

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
