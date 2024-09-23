#Uncomment when debugging
#DEBUGBUILD = true

TARGET = pvrtex
OBJS = elbg.o mem.o log.o bprint.o avstring.o lfg.o crc.o md5.o stb_image_impl.o \
	stb_image_write_impl.o stb_image_resize_impl.o optparse_impl.o pvr_texture.o \
	dither.o tddither.o vqcompress.o mycommon.o file_common.o \
	file_pvr.o file_tex.o file_dctex.o pvr_texture_encoder.o main.o

ifdef $(DEBUGBUILD)
	OPTMODE= -Og -pg -g
else
	OPTMODE= -O3 -flto
endif

MYFLAGS=-flto=auto -Wall -Wextra -Wno-unused-parameter -Wno-sign-compare -Ilibavutil -I. -DCONFIG_MEMORY_POISONING=0 -DHAVE_FAST_UNALIGNED=0 -I${KOS_INC_PATHS}
MYCPPFLAGS=$(MYFLAGS)
MYCFLAGS=$(MYFLAGS) -Wno-pointer-sign


define textSegment2Header
	mkdir -p info
	awk '/$1/,/--------------------------------------------------------------------------/' README > $2/$3.txt
	$(KOS_BASE)/utils/bin2c/bin2c $2/$3.txt $2/$3.h $(3)_txt
	rm $2/$3.txt
endef

.PHONY: all clean

$(TARGET): $(OBJS)
	gcc $(OPTMODE) -o $(TARGET) \
		$(OBJS) $(PROGMAIN) -lm -lstdc++

main.o: main.c info/options.h info/examples.h
	gcc $(CFLAGS) $(MYCFLAGS) $(OPTMODE) -c $< -o $@

%.o: %.c
	gcc $(CFLAGS) $(MYCFLAGS) $(OPTMODE) -c $< -o $@

%.o: %.cpp
	gcc $(CFLAGS) $(MYCPPFLAGS) $(CXXFLAGS) $(OPTMODE) -c $< -o $@

clean:
	rm -f $(TARGET) $(OBJS) README

README: readme_unformatted.txt
	fmt -s readme_unformatted.txt > README

all: $(TARGET) README

install: all
	install -m 755 $(TARGET) $(DC_TOOLS_BASE)/

info/options.h: README Makefile
	$(call textSegment2Header,Command Line Options:,info,options)

info/examples.h: README Makefile
	$(call textSegment2Header,Usage Examples:,info,examples)
