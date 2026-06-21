CFLAGS += -I ..

all: test_lftpd_io

test: all
	./test_lftpd_io

test_lftpd_io: test_lftpd_io.o ../lftpd_io.o

clean:
	rm -f *.o
