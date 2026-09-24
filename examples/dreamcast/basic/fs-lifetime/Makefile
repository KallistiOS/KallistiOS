TARGET = fs-lifetime.elf
OBJS = fs-lifetime.o romdisk.o
KOS_ROMDISK_DIR = fixture

all: $(TARGET)
include $(KOS_BASE)/Makefile.rules
KOS_CFLAGS += -std=gnu17 -Werror
$(TARGET): $(OBJS) $(KOS_BASE)/lib/$(KOS_ARCH)/libkallisti.a
	kos-cc -Wl,-Map,$@.map -o $@ $(OBJS)
.PHONY: all clean run
clean:
	$(RM) $(TARGET) $(OBJS) $(TARGET).map romdisk.img romdisk_tmp.o
run: $(TARGET)
	$(KOS_LOADER) $(TARGET)
