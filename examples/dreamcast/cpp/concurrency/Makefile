#
# C++ Concurrency Test/Example
# (c) 2023 Falco Girgis
#   

TARGET = concurrency.elf
OBJS = concurrency.o
KOS_CPPFLAGS += -std=c++20 

all: rm-elf $(TARGET)

include $(KOS_BASE)/Makefile.rules

clean:
	-rm -f $(TARGET) $(OBJS) 

rm-elf:
	-rm -f $(TARGET) 

$(TARGET): $(OBJS) 
	kos-c++ -o $(TARGET) $(OBJS) -lm

run: $(TARGET)
	$(KOS_LOADER) $(TARGET)

dist:
	rm -f $(OBJS)
	$(KOS_STRIP) $(TARGET)

