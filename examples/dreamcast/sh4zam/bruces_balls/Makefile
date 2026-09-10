#
# Bruce's Balls - SH4ZAM TnL Example
#
# Copyright (C) 2024 Bruceleet
# Copyright (C) 2024 Falco Girgis
#   

TARGET = bruces_balls.elf
OBJS = bruces_balls.o   

CFLAGS += -std=gnu2x -O3 \
          -fomit-frame-pointer -flto -fbuiltin -ffast-math -ffp-contract=fast -mfsrra -mfsca \
          -fmerge-all-constants -fno-PIE -fno-PIC -fipa-pta -ftree-vectorize -funroll-loops
KOS_GCCVER_MIN = 13.0.0

include $(KOS_BASE)/Makefile.rules

all: rm-elf $(TARGET)

clean: rm-elf
	-rm -f $(OBJS)

rm-elf:
	-rm -f $(TARGET)  

$(TARGET): $(OBJS)
	kos-cc -o $(TARGET) $(OBJS) -lsh4zam -Wl,--gc-sections

run: $(TARGET)
	$(KOS_LOADER) $(TARGET)

dist: $(TARGET)
	-rm -f $(OBJS)  
	$(KOS_STRIP) $(TARGET)
