TARGET = stackprotector.elf
OBJS = stackprotector.o
KOS_CFLAGS += -fstack-protector-all

GCC_MAJOR = $(basename $(basename $(KOS_GCCVER)))

ifeq ($(GCC_MAJOR), 4) 

all dist clean:
	$(warning GCC4 missing stackprotector patch, skipping)

else

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


.PHONY: run dist clean rm-elf

endif
