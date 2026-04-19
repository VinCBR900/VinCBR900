CC ?= cc
CFLAGS ?= -std=c99 -Wall -Wextra -pedantic -O2

all: hexdebug

hexdebug: hexdebug.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f hexdebug

