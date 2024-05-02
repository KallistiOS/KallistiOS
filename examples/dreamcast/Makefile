# KallistiOS ##version##
#
# examples/dreamcast/Makefile
# Copyright (C) 2003 Megan Potter
# Copyright (C) 2024 Andy Barajas
#

DIRS = 2ndmix basic libdream kgl hello sound png network vmu conio pvr video \
	   lua parallax modem dreameye sd g1ata lightgun keyboard sdl dev rumble \
	   micropython

ifdef KOS_CCPLUS
	DIRS += cpp tsunami
endif

ERROR_LOG = build_errors.log

.PHONY: all clean dist show_errors clean_up

all: show_errors clean_up

build_all:
	@echo "Starting build..."
	@rm -f $(ERROR_LOG)
	@for i in $(DIRS); do \
		$(KOS_MAKE) -C $$i || echo "Failed to build in $$i" >> $(ERROR_LOG); \
	done

show_errors: build_all
	@if [ -s $(ERROR_LOG) ]; then \
		echo "\n----------------------\nErrors occurred during build:"; \
		cat $(ERROR_LOG); \
	else \
		echo "Build completed successfully, no errors."; \
	fi

clean: clean_up
	@for i in $(DIRS); do \
		$(KOS_MAKE) -C $$i clean || echo "Failed to clean $$i" >> $(ERROR_LOG); \
	done

dist: clean_up
	@for i in $(DIRS); do \
		$(KOS_MAKE) -C $$i dist || echo "Failed to distribute $$i" >> $(ERROR_LOG); \
	done

clean_up:
	@rm -f $(ERROR_LOG)
