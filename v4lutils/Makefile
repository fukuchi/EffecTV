# Makefile for v4lutils

include ../config.mk

CC = gcc
CFLAGS = $(CONFIG) $(CONFIG.arch) $(CFLAGS.opt)
LIBS =

%.o: %.c
	$(CC) $(CFLAGS) -Wall -c -o $@ $<

all-recursive: all
all: libv4lutils.a

install: libv4lutils.a
	cp -f libv4lutils.a /usr/local/lib/
	cp -f v4lutils.h /usr/local/include/

libv4lutils.a: v4lutils.o
	ar rc libv4lutils.a v4lutils.o

v4lutils.o: v4lutils.c v4lutils.h

clean:
	rm -f *.o libv4lutils.a
