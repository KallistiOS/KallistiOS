# OpenGL (c)2013-2014 Josh PH3NOM Pearson

TARGET = vq-example.elf
OBJS = vq-example.o fruit.o
LDLIBS = -lGL

include $(KOS_BASE)/Makefile.rules

clean_vq:
	-rm -f fruit.vq

clean: clean_vq

fruit.o: fruit.vq
	$(KOS_BASE)/utils/bin2o/bin2o fruit.vq fruit fruit.o

fruit.vq: fruit.jpg
	$(KOS_BASE)/utils/vqenc/vqenc -t -v fruit.jpg
