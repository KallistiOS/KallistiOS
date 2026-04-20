#
# std::filesystem test program
#   

TARGET = filesystem.elf
OBJS = filesystem.o romdisk.o
KOS_CPPFLAGS += -std=c++23
KOS_ROMDISK_DIR = romdisk
KOS_GCCVER_MIN = 14.0.0

all: rm-elf $(TARGET)

include $(KOS_BASE)/Makefile.rules

ifeq ($(call KOS_GCCVER_MIN_CHECK,$(KOS_GCCVER_MIN)),1)

clean: rm-elf
	-rm -f $(OBJS) 

rm-elf:
	-rm -f $(TARGET) romdisk.*

$(TARGET): $(OBJS)
	kos-c++ -o $(TARGET) $(OBJS)

run: $(TARGET)
	$(KOS_LOADER) $(TARGET)

dist: $(TARGET)
	-rm -f $(OBJS) romdisk.img
	$(KOS_STRIP) $(TARGET)

else
  all $(TARGET) clean rm-elf run dist:
	$(KOS_GCCVER_MIN_WARNING)
endif
