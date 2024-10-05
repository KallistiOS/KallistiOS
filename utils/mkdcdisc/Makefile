# KallistiOS ##version##
#
# utils/mkdcdisc/Makefile
# Copyright (C) 2024 Andress Barajas
#

SCRIPT_NAME = install_mkdcdisc.sh

all: run-script

clean:
	rm -rf ./mkdcdisc

run-script:
	@chmod +x $(SCRIPT_NAME)
	@./$(SCRIPT_NAME)
	@if [ -f ./mkdcdisc/builddir/mkdcdisc ]; then \
		install -m 755 ./mkdcdisc/builddir/mkdcdisc $(DC_TOOLS_BASE)/; \
	fi
