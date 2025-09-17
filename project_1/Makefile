# Makefile for CS4440 Project 1 (All Tasks 1-9, excluding Task 10 analysis only)

CC ?= gcc
CFLAGS ?= -std=c11 -O2 -Wall -Wextra -Werror -pedantic
LDFLAGS ?=
LDFLAGS_PTHREAD = -lpthread

.PHONY: all clean

PROGS := task_1/MyCompress task_2/MyDecompress task_3/ForkCompress task_4/PipeCompress task_5/ParFork task_6/MinShell task_7/MoreShell task_8/DupShell task_9/ParThread

all: $(PROGS)

task_1/MyCompress: task_1/MyCompress.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

task_2/MyDecompress: task_2/MyDecompress.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

task_3/ForkCompress: task_3/ForkCompress.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

task_4/PipeCompress: task_4/PipeCompress.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

task_5/ParFork: task_5/ParFork.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

task_6/MinShell: task_6/MinShell.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

task_7/MoreShell: task_7/MoreShell.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

task_8/DupShell: task_8/DupShell.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

task_9/ParThread: task_9/ParThread.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS) $(LDFLAGS_PTHREAD)

clean:
	rm -f $(PROGS)
