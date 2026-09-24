# Host-only production POSIX clock tests.
CC = cc
CFLAGS = -O2 -std=gnu17 -Wall -Wextra -Werror
PYTHON = python3
.PHONY: test
test:
	$(PYTHON) run.py --cc "$(CC)" --cflags "$(CFLAGS)"
