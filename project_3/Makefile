# Makefile for CS4440 Project 3: Secondary Storage and Filesystem Server

CC ?= gcc
CFLAGS ?= -std=c11 -O2 -Wall -Wextra -pedantic
LDFLAGS ?=
LDFLAGS_PTHREAD = -lpthread

.PHONY: all clean

PROGS := \
	reverse_server \
	reverse_client \
	ls_server \
	ls_client \
	disk_server \
	disk_client_cli \
	disk_client_random \
	fs_server \
	fs_client

all: $(PROGS)

reverse_server: reverse_server.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS) $(LDFLAGS_PTHREAD)

reverse_client: reverse_client.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

ls_server: ls_server.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

ls_client: ls_client.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

disk_server: disk_server.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

disk_client_cli: disk_client_cli.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

disk_client_random: disk_client_random.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

fs_server: fs_server.c fs_defs.h
	$(CC) $(CFLAGS) -o $@ fs_server.c $(LDFLAGS) $(LDFLAGS_PTHREAD)

fs_client: fs_client.c fs_defs.h
	$(CC) $(CFLAGS) -o $@ fs_client.c $(LDFLAGS)

clean:
	rm -f $(PROGS)
