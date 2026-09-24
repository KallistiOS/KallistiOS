CC ?= cc
CSTD ?= gnu17
CFLAGS ?= -O2
CFLAGS += -std=$(CSTD) -Wall -Wextra -Werror -pthread
ROOT := ../..
SOURCE := $(ROOT)/kernel/thread/workqueue.c
HEADERS := $(wildcard include/kos/*.h) $(ROOT)/include/kos/workqueue.h
.PHONY: test clean
test: workqueue-test
	./workqueue-test
workqueue-test: workqueue-test.c $(SOURCE) $(HEADERS)
	$(CC) $(CFLAGS) -Iinclude -o $@ workqueue-test.c $(SOURCE)
clean:
	$(RM) workqueue-test
