# Makefile for EffecTV/tools

include ../config.mk

CC = gcc
NASM = nasm
INSTALL = /usr/bin/install -c

CFLAGS = $(CONFIG) $(CONFIG.arch) $(CFLAGS.opt)
LIBS =

V4L_CFLAGS = -I../v4lutils
V4L_LIBS = ../v4lutils/libv4lutils.a -lpthread

%.o: %.c
	$(CC) $(CFLAGS) -Wall -c -o $@ $<
%.o: %.nas
	$(NASM) -f elf $<

all-recursive:
	@list='$(SUBDIRS)'; for subdir in $$list; do \
	(cd $$subdir && $(MAKE) all-recursive) || exit 1;\
	done; \
	$(MAKE) all-am

all-am: palettecheck

palettecheck: palettecheck.o
	$(CC) -o $@ palettecheck.o $(V4L_LIBS)

palettecheck.o: palettecheck.c
	$(CC) $(CFLAGS) $(V4L_CFLAGS) -Wall -c -o $@ $<

clean:
	rm -f *.o palettecheck
