# Makefile for userspace test programs
CC = gcc
CFLAGS = -Wall -g
LDLIBS = -lpthread # <-- Add this for linking pthread library

# List of all programs to build
TARGETS = test_msgbuf test_concurrent test_stress

all: $(TARGETS)

test_msgbuf: test_msgbuf.c msgbuf.h
	$(CC) $(CFLAGS) test_msgbuf.c -o test_msgbuf

test_concurrent: test_concurrent.c
	$(CC) $(CFLAGS) test_concurrent.c -o test_concurrent

test_stress: test_stress.c msgbuf.h
	$(CC) $(CFLAGS) test_stress.c -o test_stress $(LDLIBS) # <-- Note the $(LDLIBS)

clean:
	rm -f $(TARGETS)