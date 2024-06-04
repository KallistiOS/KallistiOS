# OpenGL (c)2013-2014 Josh PH3NOM Pearson

TARGET = vq-example.elf
OBJS = vq-example.o fruit.o

all: rm-elf $(TARGET)

include $(KOS_BASE)/Makefile.rules

clean: rm-elf
	-rm -f $(OBJS) fruit.vq

rm-elf:
	-rm -f $(TARGET)

$(TARGET): $(OBJS)
	kos-cc -o $(TARGET) $(OBJS) -L$(KOS_BASE)/lib -lGL

fruit.o: fruit.vq
	$(KOS_BASE)/utils/bin2o/bin2o fruit.vq fruit fruit.o

fruit.vq: fruit.jpg
	$(KOS_BASE)/utils/vqenc/vqenc -t -v fruit.jpg

run: $(TARGET)
	$(KOS_LOADER) $(TARGET)

dist: $(TARGET)
	-rm -f $(OBJS)
	$(KOS_STRIP) $(TARGET)

