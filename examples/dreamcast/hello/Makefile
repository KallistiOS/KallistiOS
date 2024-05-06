#
# Basic KallistiOS skeleton / test program
# Copyright (C) 2001-2004 Megan Potter
# Copyright (C) 2024 Falco Girgis
#   

# Put the filename of the output binary here
TARGET = hello.elf

# List all of your C or C++ files here, but change the extension to ".o"
OBJS = hello.o

# Optional path to a directory of resources to bundle within your ELF binary.
# Its contents are accessible via the "/rd/" virtual directory at runtime.
#KOS_ROMDISK_DIR = romdisk

# Main rule which forces our ELF binary to be built
all: rm-elf $(TARGET)

# Include the common KOS Makefile rules and configuration
include $(KOS_BASE)/Makefile.rules

# Cleans the binary ELF file plus the intermediate .o files
clean: rm-elf
	-rm -f $(OBJS)

# Removes the binary ELF file
rm-elf:
	-rm -f $(TARGET)

# Invokes the compiler to build the target from our object files
$(TARGET): $(OBJS)
	kos-cc -o $(TARGET) $(OBJS)

# Attempts to run the target using the configured loader application
run: $(TARGET)
	$(KOS_LOADER) $(TARGET)

# Creates a distributable/release ELF which strips away its debug symbols
dist: $(TARGET)
	-rm -f $(OBJS)
	$(KOS_STRIP) $(TARGET)
