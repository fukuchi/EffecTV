# Makefile for EffecTV

prefix = /usr/local
exec_prefix = ${prefix}

bindir = ${exec_prefix}/bin
mandir = ${prefix}/man
man1dir = ${mandir}/man1

INSTALL = /usr/bin/install -c

# comment next line if you want to disable vloopback support.
CONFIG += -DVLOOPBACK
# choose vloopback version (only one).
## version 0.90 or later
#CONFIG += -DVLOOPBACK_VERSION=90 
## version 0.83 or former
CONFIG += -DVLOOPBACK_VERSION=83

CC = gcc
NASM = nasm
CFLAGS = $(CONFIG) -mpentiumpro -O3 -fomit-frame-pointer -funroll-loops -Iv4lutils `sdl-config --cflags`
#CFLAGS = $(CONFIG) -g -Iv4lutils `sdl-config --cflags`
LIBS = v4lutils/libv4lutils.a -lm `sdl-config --libs`

VLOOPBACKOBJS = vloopback.o
UTILS = utils.o yuv.o buffer.o image.o

PROGRAM = effectv
OBJS = main.o screen.o video.o frequencies.o palette.o $(UTILS) $(VLOOPBACKOBJS)
LIBEFFECTS = effects/libeffects.a
SUBDIRS = effects v4lutils tools

%.o: %.c
	$(CC) $(CFLAGS) -Wall -c -o $@ $<
%.o: %.nas
	$(NASM) -f elf $<

all-recursive:
	@list='$(SUBDIRS)'; for subdir in $$list; do \
	(cd $$subdir && $(MAKE) all-recursive) || exit 1;\
	done; \
	$(MAKE) all-am

all-am: $(PROGRAM)

$(PROGRAM): $(OBJS) $(LIBEFFECTS) v4lutils/libv4lutils.a
	$(CC) -o $@ $(OBJS) $(LIBEFFECTS) $(LIBS)

$(OBJS): EffecTV.h screen.h video.h palette.h frequencies.h vloopback.h

install: all-am
	$(INSTALL) -s $(PROGRAM) $(bindir)
	$(INSTALL) effectv.1 $(man1dir)

clean:
	rm -f *.o $(PROGRAM)
	cd effects && $(MAKE) clean
	cd v4lutils && $(MAKE) clean
