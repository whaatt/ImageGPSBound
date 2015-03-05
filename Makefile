CC=gcc
CFLAGS=-std=gnu99

all:
	$(CC) bound.c exif.c -o bound $(CFLAGS)
	
clean:
	rm bound.exe
