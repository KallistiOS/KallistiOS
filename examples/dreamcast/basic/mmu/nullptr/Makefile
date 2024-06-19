#
# MMU test program: NULL pointer detection
# (c)2002 Megan Potter
#   

TARGET = nullptr.elf
OBJS = nullptr.o
# This is required to tell GCC not to optimize away our
# NULL dereference and that it can actually be a valid
# memory address.
KOS_CFLAGS += -fno-delete-null-pointer-checks

all: rm-elf $(TARGET)

include $(KOS_BASE)/Makefile.rules

clean: rm-elf
	-rm -f $(OBJS)

rm-elf:
	-rm -f $(TARGET)

$(TARGET): $(OBJS)
	kos-cc -o $(TARGET) $(OBJS)

run: $(TARGET)
	$(KOS_LOADER) $(TARGET)

dist: $(TARGET)
	-rm -f $(OBJS)
	$(KOS_STRIP) $(TARGET)

