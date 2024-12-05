CC=gcc
CFLAGS=-std=c89 -pedantic -Wall -Werror -g
LDFLAGS=`pkg-config --libs --cflags sdl` -lSDL_mixer -lm
LDLIBS=
RM=rm -fv
.PHONY: all clean
all: chip8
%.o: %.c %.h
		$(CC) $(CFLAGS) -c -o $@ $<
chip8: main.c cpu.c
		$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) $(LDLIBS)
clean:
		$(RM) *.o