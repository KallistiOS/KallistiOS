# KallistiOS ##version##
#
# examples/dreamcast/profiling/gcov/Makefile
# Copyright (C) 2026 Andress Barajas
#

TARGET = gcov.elf
OBJS = gcov.o math/ops.o util/str.o util/text/format.o
KOS_GCCVER_MIN = 12.0.0

# --coverage is all you need. kos-cc auto-adds -fprofile-info-section=gcov_info
# at compile time and links libkosgcov + GCC's libgcov at link time.
CFLAGS = --coverage

include $(KOS_BASE)/Makefile.rules

all: rm-elf $(TARGET)

clean: rm-elf
	-rm -f $(OBJS)
	-find . \( -name '*.gcno' -o -name '*.gcda' \) -delete
	-rm -f coverage.info
	-rm -rf html

rm-elf:
	-rm -f $(TARGET)

$(TARGET): $(OBJS)
	kos-cc --coverage -o $(TARGET) $(OBJS)

# Each .gcda is written at its full build-time path under /pc.
run: $(TARGET)
	$(KOS_LOADER) $(TARGET)

dist: $(TARGET)
	-rm -f $(OBJS)
	$(KOS_STRIP) $(TARGET)

# HTML report. The lcov flags keep lcov 2.x from aborting on the data quirks
# normal for optimized/inlined and header code.
report:
	lcov --capture --directory . --gcov-tool $(KOS_GCOV) --rc geninfo_unexecuted_blocks=1 --ignore-errors inconsistent,unused,source,empty --output-file coverage.info
	genhtml coverage.info --output-directory html --ignore-errors inconsistent,source
	@echo "open html/index.html"
