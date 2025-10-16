# Makefile for Project 2 — Problem 2 (Mother Hubbard)
# Targets:
#   make        -> build ./mh
#   make clean  -> remove build artifacts

CC=gcc
CFLAGS=-Wall -Wextra -pthread
TARGET=mh
SRC=src/mh.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

clean:
	rm -f $(TARGET) *.o


# macOS native build (named semaphores)
macos:
	$(CC) $(CFLAGS) -o mh_macos src/mh_macos.c
