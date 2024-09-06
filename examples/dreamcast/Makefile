# KallistiOS ##version##
#
# examples/dreamcast/Makefile
# Copyright (C) 2003 Megan Potter
# Copyright (C) 2024 Andy Barajas
#

DIRS = 2ndmix basic libdream kgl hello sound png vmu conio pvr video \
	   lua parallax dreameye filesystem sd lightgun keyboard sdl dev rumble \
	   micropython

ifneq ($(KOS_SUBARCH), naomi)
	DIRS += network modem g1ata cd
endif

ifdef KOS_CCPLUS
	DIRS += cpp tsunami
endif

.PHONY: all

all:
	@for dir in $(DIRS); do \
		$(KOS_MAKE) check-dir DIR=$$dir; \
	done;

	@error_count=$$(cat error_count.txt 2>/dev/null || echo 0); \
	if [ -f errors.txt ]; then \
		echo "\n-------------------------------------------------"; \
		echo "$$error_count error(s) occurred during the build process:"; \
		cat errors.txt; \
	fi; \
	rm -f errors.txt error_count.txt; \
	exit $$error_count

# ALGORITHM EXPLANATION:
# This script recursively checks each directory to determine if it should 
# execute its Makefile based on the presence of Makefiles in its subdirectories.
#
# Steps:
# 1. For each directory, identify all direct subdirectories.
# 2. Check each subdirectory for the existence of a Makefile.
# 3. If any subdirectory contains a Makefile, recursively perform this check on 
#    that subdirectory and do not execute the Makefile in the current directory.
# 4. If no subdirectories contain a Makefile, execute the Makefile in the 
#    current directory.
#
# Purpose:
# - This approach ensures that Makefiles are only executed in the most specific 
#   (deepest) directories that do not contain further subdirectories with 
#   Makefiles.
# - This prevents redundant builds in parent directories and ensures errors are
#   logged with specific directory paths, providing clear visibility into which 
#   particular build process failed without aggregating errors at a higher 
#   directory level.
check-dir:
	@should_make="yes"; \
	for subdir in $(DIR)/*; do \
		if [ -e "$$subdir/Makefile" ]; then \
			should_make="no"; \
			$(KOS_MAKE) check-dir DIR=$$subdir; \
		fi; \
	done; \
	if [ "$$should_make" = "yes" ]; then \
		$(KOS_MAKE) -C $(DIR); \
		rv=$$?; \
		if [ "$$rv" -ne 0 ]; then \
			echo "$(DIR): Build failed with return code $$rv" >> errors.txt; \
			echo $$(($$(cat error_count.txt 2>/dev/null || echo 0) + 1)) > error_count.txt; \
		fi; \
	fi;

clean:
	@for dir in $(DIRS); do \
		$(KOS_MAKE) -C $$dir clean; \
	done

dist:
	@for dir in $(DIRS); do \
		$(KOS_MAKE) -C $$dir dist; \
	done

