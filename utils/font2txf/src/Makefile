# Makefile for the font2txf program.

# Detect the host (extracted from dcload)
ROOTDIR := $(dir $(abspath $(lastword $(MAKEFILE_LIST))))
include $(ROOTDIR)/Makefile.hostdetect

# Include configuration
include $(ROOTDIR)/Makefile.cfg

# Name of the program
TARGET      = font2txf

# Utilities
CXX         = $(HOSTCXX)
STRIP       = $(HOSTSTRIP)

# Basic compiler flags
CCFLAGS     ?= -pipe -W

# Basic linker flags
LDFLAGS     ?= -L/usr/local/lib -L/usr/lib
LDLIBS      ?= -lfreetype

# Sources and objects (dynamic here)
SOURCES     = $(wildcard *.cpp)
OBJECTS     = $(SOURCES:.cpp=.o)

# Install location
INSTALLDIR  = $(TOOLINSTALLDIR)

# Name of the binary file generated
OUTPUT      = $(TARGET)$(EXECUTABLEEXTENSION)

# Debug flag
BUILD_FLAG := -O2
ifeq ($(DEBUG),1)
  BUILD_FLAG := -g
  CCFLAGS += -Wall -D_DEBUG
endif
CCFLAGS += $(BUILD_FLAG)

# Enable preview (uses GLUT) flag
ifeq ($(PREVIEW),1)
  PREVIEW_CCFLAGS = -DDISPLAY
  PREVIEW_LDLIBS = -lGL -lglut -lGLU

  ifeq ($(STANDALONE_BINARY),1)
    PREVIEW_CCFLAGS += -DFREEGLUT_STATIC
  endif

  ifdef WINDOWS
    # Standard Freeglut library (dynamically linked)
    PREVIEW_LDLIB_FREEGLUT := -lfreeglut

    # Handle static Freeglut
    ifeq ($(STANDALONE_BINARY),1)
      PREVIEW_LDLIB_FREEGLUT := -lfreeglut_static
    endif

    PREVIEW_LDLIBS = $(PREVIEW_LDLIB_FREEGLUT) -lopengl32 -lglu32 -lgdi32 -lwinmm
  endif

  CCFLAGS += $(PREVIEW_CCFLAGS)
  LDLIBS += $(PREVIEW_LDLIBS)
endif

ifdef MACOS
  CCFLAGS += -std=gnu++11
endif

# Static flag
ifeq ($(STANDALONE_BINARY),1)
  CCFLAGS += -Bstatic -static
  LDLIBS += -lpng -lz -lm
endif

# Final flags
INC_FLAGS   := -I/usr/local/include/freetype2 -I/usr/local/include -I/usr/include/freetype2 -I/usr/include
CXXFLAGS    := $(CCFLAGS) -DPROGRAM_VERSION=\"$(VERSION)\" $(INC_FLAGS)

# Makefile targets

all: $(TARGET)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) -o $(OUTPUT) $(OBJECTS) $(LDFLAGS) $(LDLIBS)	

install: $(TARGET)
	mkdir -p $(INSTALLDIR)
	$(STRIP) $(OUTPUT)
	mv $(OUTPUT) $(INSTALLDIR)	

.PHONY: clean
clean:
	-rm -f $(OUTPUT) *.o
