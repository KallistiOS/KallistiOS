# KallistiOS ##version##
#
# examples/dreamcast/Makefile
# Copyright (C) 2003 Megan Potter
# Copyright (C) 2024 Andy Barajas
# Copyright (C) 2024 Paul Cercueil
#

# Get a list of the sub-directories that contain a Makefile
get_subdirs=$(foreach each,$(wildcard $(1)/*),$(if $(wildcard $(each)/Makefile),$(each),) $(call get_subdirs,$(each)))

DIRS := $(call get_subdirs, $(shell pwd)/$(KOS_ARCH))

.PHONY: all $(DIRS)

all: $(DIRS)
	@error_count=$$(cat error_count.txt 2>/dev/null || echo 0); \
	if [ -f errors.txt ]; then \
		echo "\n-------------------------------------------------"; \
		echo "$$error_count error(s) occurred during the build process:"; \
		cat errors.txt; \
	fi; \
	rm -f errors.txt error_count.txt; \
	exit $$error_count

$(DIRS):
	@$(MAKE) -C $@ all_$(KOS_SUBARCH) ; \
	rv=$$? ; \
	if [ "$$rv" -ne 0 ]; then \
		echo "$(subst $(shell pwd)/,,$@): Build failed with return code $$rv" >> errors.txt; \
		echo $$(($$(cat error_count.txt 2>/dev/null || echo 0) + 1)) > error_count.txt; \
	fi

clean:
	@for dir in $(DIRS); do \
		$(MAKE) -C $$dir clean; \
	done

dist:
	@for dir in $(DIRS); do \
		$(MAKE) -C $$dir dist; \
	done
