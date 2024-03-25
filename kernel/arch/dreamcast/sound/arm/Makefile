TARGET = stream.drv
OBJS = main.o

.PHONY: all clean aicaos

CC = $(DC_ARM_CC)
AS = $(DC_ARM_AS)
CPPFLAGS = $(DC_ARM_INCS) -I $(KOS_BASE)/kernel/arch/dreamcast/include/dc/sound
CFLAGS = $(DC_ARM_CFLAGS)
ASFLAGS = $(DC_ARM_AFLAGS)
LDFLAGS = $(DC_ARM_LDFLAGS) $(DC_ARM_LIB_PATHS) -Wl,-Map,prog.map
LDLIBS = $(DC_ARM_LIBS)

all: $(TARGET)

aicaos:
	$(MAKE) -C aicaos

ARM_CC_IS_AVAILABLE=0
ifdef DC_ARM_CC
  ifneq ("$(wildcard $(DC_ARM_CC))", "") 
    ARM_CC_IS_AVAILABLE=1
  endif
endif

ifeq ($(ARM_CC_IS_AVAILABLE), 1)
# Only compile this if we have an ARM compiler handy
stream.drv: prog.elf
	$(DC_ARM_OBJCOPY) -O binary $< $@
else
# Otherwise use precompiled ARM binary
stream.drv: stream.drv.prebuilt
	cp $< $@
endif

prog.elf: $(OBJS)
	$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@

$(OBJS): aicaos

clean-aicaos:
	$(MAKE) -C aicaos clean

clean: clean-aicaos
	-rm -f $(OBJS) prog.elf prog.map $(TARGET)
