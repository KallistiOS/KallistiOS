TARGET = beachbox.elf
SRCS = main.c \
       src/scenes/credits.c \
       src/scenes/game.c \
       src/scenes/loading.c \
       src/scenes/mainmenu.c \
       src/scenes/options.c \
       src/scenes/raylogo.c \
       src/scenes/shop.c \
       src/scenes/unlockables.c \
       src/audio.c \
       src/background.c \
       src/controller.c \
       src/hats.c \
       src/objects.c \
       src/player.c \
       src/save.c \
       src/ui.c \
       src/scene.c \
       src/vmu.c
OBJS = $(SRCS:.c=.o) romdisk.o
KOS_ROMDISK_DIR = romdisk
KOS_CFLAGS += -std=c23

all: $(TARGET)

include $(KOS_BASE)/Makefile.rules

clean: rm-elf
	-rm -f $(OBJS)

rm-elf:
	-rm -f $(TARGET) romdisk.*

$(TARGET): $(OBJS)
	kos-cc -o $(TARGET) $(OBJS) -lraylib -lGL -lm -lkosutils -lADX

run: $(TARGET)
	$(KOS_LOADER) $(TARGET)

dist: $(TARGET)
	-rm -f $(OBJS) romdisk.img
	$(KOS_STRIP) $(TARGET)

.PHONY: all clean rm-elf run dist
