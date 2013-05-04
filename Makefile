TARGET=kobo-remote tap-screen
CC=arm-none-linux-gnueabi-gcc
CFLAGS=-Os -Wall

all: $(TARGET)

clean:
	rm -f $(TARGET)
