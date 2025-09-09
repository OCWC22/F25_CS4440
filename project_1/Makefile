# Makefile for CS4440 Project 1 (Parts 1-3)

CC ?= cc
CFLAGS ?= -std=c11 -O2 -Wall -Wextra -Werror -pedantic
LDFLAGS ?=

.PHONY: all clean

PROGS := MyCompress MyDecompress ForkCompress

all: $(PROGS)

MyCompress: MyCompress.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

MyDecompress: MyDecompress.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

ForkCompress: ForkCompress.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

clean:
	rm -f $(PROGS) *.o
