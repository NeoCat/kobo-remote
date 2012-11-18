TARGET=kobo-remote tap-screen
CC=arm-none-linux-gnueabi-gcc
CFLAGS=-Os

all: $(TARGET)

clean:
	rm -f $(TARGET)
