# KallistiOS ##version##
#
# examples/xbox/hello/Makefile
# Copyright (C) 2026 Andress Barajas
#

# A runnable Xbox console program, loaded over the network by kos-tool's
# xbox-load-ip.
#
# There is no KOS Xbox arch/build environment yet, so this does NOT use kos-cc /
# Makefile.rules like the Dreamcast/PS2 examples. It drives the i686-pc-xbox
# cross toolchain directly, following kos-tool's guest ABI: link a PE image at
# the guest load address, then objcopy it to ELF for the loader to consume.

XBOX_TOOLCHAIN ?= /opt/toolchains/xbox/i686-pc-xbox
XBOX_TARGET    ?= i686-pc-xbox
CC      = $(XBOX_TOOLCHAIN)/bin/$(XBOX_TARGET)-gcc
OBJCOPY = $(XBOX_TOOLCHAIN)/bin/$(XBOX_TARGET)-objcopy

# Memory map -- must match kos-tool's mk/memory.mk:
#   XBOX_KOSLOAD_BASE   loader's guest-facing header (magic + syscall ptr)
#   XBOX_LOAD_ADDR      where xbox-load-ip loads uploaded guest programs
XBOX_KOSLOAD_BASE ?= 0x00011000
XBOX_LOAD_ADDR    ?= 0x00400000

TARGET = hello.elf
INTER  = hello.exe

CFLAGS  = -march=pentium3 -m32 -O2 -ffreestanding -fno-pic -fno-builtin \
          -fno-zero-initialized-in-bss -nostdlib -Wall -Wextra \
          -DXBOX_KOSLOAD_BASE=$(XBOX_KOSLOAD_BASE)
# Freestanding guest: no crt/libc; override the toolchain's default image base
# (0x10000) with the guest load address, entry _start, 0x20 alignment.
LDFLAGS = -nostdlib -nostartfiles \
          -Wl,--image-base,$(XBOX_LOAD_ADDR) \
          -Wl,--section-alignment,0x20 -Wl,--file-alignment,0x20 \
          -Wl,-e,_start

all: $(TARGET)

$(INTER): hello.c
	$(CC) $(CFLAGS) $(LDFLAGS) $< -o $@

$(TARGET): $(INTER)
	$(OBJCOPY) -O elf32-i386 $< $@

# Upload and run on a real Xbox via kos-tool (xbox-load-ip must be running):
run: $(TARGET)
	kos-tool -x $(TARGET)

clean:
	-rm -f $(TARGET) $(INTER)

.PHONY: all run clean
