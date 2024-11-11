all: stream.drv

aica_fw.h: stream.drv
	$(KOS_BASE)/utils/bin2c/bin2c stream.drv aica_fw.h.tmp
	echo 'unsigned char aica_fw[] = {' > aica_fw.h
	cat aica_fw.h.tmp >> aica_fw.h
	echo '};' >> aica_fw.h
	-rm aica_fw.h.tmp

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

prog.elf: crt0.o main.o aica.o
	$(DC_ARM_CC) -Wl,-Ttext,0x00000000,-Map,prog.map,-N -nostartfiles -nostdlib -e reset -o prog.elf crt0.o main.o aica.o -lgcc

%.o: %.c
	$(DC_ARM_CC) $(DC_ARM_CFLAGS) $(DC_ARM_INCS) -I $(KOS_BASE)/kernel/arch/dreamcast/include/dc/sound -c $< -o $@

%.o: %.s
	$(DC_ARM_AS) $(DC_ARM_AFLAGS) $< -o $@

clean:
	-rm -f *.o *.srec *.elf 1ST_READ.BIN prog.bin *.bck prog.map stream.drv
