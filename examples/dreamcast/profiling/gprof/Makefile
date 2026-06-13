# KallistiOS ##version##
#
# examples/dreamcast/profiling/gprof/Makefile
# Copyright (C) 2025 Andy Barajas
#

TARGET = gprof.elf
OBJS = gprof.o

# -pg instruments every function for profiling. -fno-omit-frame-pointer is
# REQUIRED only because GCC refuses -pg with an omitted frame pointer (KOS builds
# at -O2, which omits it by default); the call graph itself reads the caller from
# a register, so it does NOT depend on the frame pointer -- which is just as well,
# since recent SH4 GCC frame pointers are unreliable. -fno-inline keeps each function
# a distinct symbol in the report but is not, and -fno-ipa-icf stops GCC from folding
# hot_path() and warm_path() (which have identical bodies) into one symbol -- without
# it the call graph would show only one caller of spin() instead of the intended two.
#
# NOTE: -pg must also be on the LINK line (below). That is what makes the
# kos-cc wrapper auto-link the libgprof runtime -- you never type -lgprof,
# exactly like gcc auto-links -lgcov for --coverage.
CFLAGS = -pg -fno-omit-frame-pointer -fno-inline -fno-ipa-icf

all: rm-elf $(TARGET)

include $(KOS_BASE)/Makefile.rules

clean: rm-elf
	-rm -f $(OBJS) gmon.out gprof_output.txt graph.svg

rm-elf:
	-rm -f $(TARGET) gmon.out

# The -pg here is the trigger for the auto-linked runtime.
$(TARGET): $(OBJS)
	kos-cc -pg -o $(TARGET) $(OBJS)

# Run on the console. The program writes gmon.out to /pc, so /pc must map to a
# writable host directory -- "dc/kos-tool -m ." maps it to this one (where `make
# report` then looks).
run: $(TARGET)
	$(KOS_LOADER) $(TARGET) -m .

# Turn a run's gmon.out into a human-readable flat profile + call graph.
report: gmon.out
	$(KOS_GPROF) $(TARGET) gmon.out > gprof_output.txt
	@echo "Wrote gprof_output.txt"

# Optional: a call-graph image (needs gprof2dot + graphviz on the host).
graph: gmon.out
	$(KOS_GPROF) $(TARGET) gmon.out | gprof2dot | dot -Tsvg -o graph.svg
	@echo "Wrote graph.svg"

dist: $(TARGET)
	-rm -f $(OBJS)
	$(KOS_STRIP) $(TARGET)
