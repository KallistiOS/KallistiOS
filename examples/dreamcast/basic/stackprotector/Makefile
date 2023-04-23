TARGET = stackprotector.elf
OBJS = stackprotector.o
KOS_CFLAGS += -fstack-protector-all

GCC_MAJOR = $(basename $(basename $(KOS_GCCVER)))

clean:
	-rm -f $(TARGET) $(OBJS)

ifeq ($(GCC_MAJOR), 4) 

all dist:
	$(warning GCC4 missing stackprotector patch, skip building example)

else

all: rm-elf $(TARGET)

include $(KOS_BASE)/Makefile.rules

rm-elf:
	-rm -f $(TARGET)

$(TARGET): $(OBJS)
	kos-cc -o $@ $^

run: $(TARGET)
	$(KOS_LOADER) $(TARGET)

dist: $(TARGET)
	rm -f $(OBJS)
	$(KOS_STRIP) $(TARGET)


.PHONY: run dist clean rm-elf

endif