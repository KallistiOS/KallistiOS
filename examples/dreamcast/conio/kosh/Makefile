#
# Kosh
# (c)2001 Jordan DeLong
#   

TARGET = kosh.elf
OBJS = kosh.o

all: rm-elf $(TARGET)

include $(KOS_BASE)/Makefile.rules

clean: rm-elf
	-rm -f $(OBJS)

rm-elf:
	-rm -f $(TARGET)

$(TARGET): $(OBJS) 
	$(KOS_CC) $(KOS_CFLAGS) $(KOS_LDFLAGS) -o $(TARGET) $(KOS_START) \
		$(OBJS) $(OBJEXTRA) -lkosh -lconio -lm $(KOS_LIBS)

run: $(TARGET)
	$(KOS_LOADER) $(TARGET)

dist:
	-rm -f $(OBJS)
	$(KOS_STRIP) $(TARGET)

