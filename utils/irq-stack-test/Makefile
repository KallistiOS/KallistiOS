# Standalone host regression: no graphics or fork-specific test dependencies.
CC ?= cc
HOST_CSTD ?= gnu17
CFLAGS ?= -O2
CFLAGS += -std=$(HOST_CSTD) -Wall -Wextra -Werror
CPPFLAGS += -Iinclude -I../../kernel/arch/dreamcast/include
TARGET = irq-stack-test
HEADER = ../../kernel/arch/dreamcast/include/arch/irq.h
SHIMS = include/kos/irq.h include/kos/cdefs.h

.PHONY: all test clean dreamcast
all: $(TARGET) $(TARGET)-plain
$(TARGET): irq-stack-test.c $(HEADER) $(SHIMS)
	$(CC) $(CFLAGS) $(CPPFLAGS) -D__SH_ATOMIC_MODEL_SOFT_GUSA__=1 $< -o $@
$(TARGET)-plain: irq-stack-test.c $(HEADER) $(SHIMS)
	$(CC) $(CFLAGS) $(CPPFLAGS) $< -o $@
test: all
	./$(TARGET)
	./$(TARGET)-plain
dreamcast: irq-stack-test.elf
irq-stack-test.elf: irq-stack-test.c $(HEADER) $(KOS_BASE)/lib/$(KOS_ARCH)/libkallisti.a
	kos-cc $(KOS_CSTD) -Wall -Wextra -Werror $< -o $@
clean:
	$(RM) $(TARGET) $(TARGET)-plain irq-stack-test.elf
