# Makefile for EffecTV

include ./config.mk

# the following lines set destination directory for a compiled program and
# manual page.
prefix = /usr/local
exec_prefix = ${prefix}

bindir = ${exec_prefix}/bin
mandir = ${prefix}/man
man1dir = ${mandir}/man1

INSTALL = /usr/bin/install -c

CC = gcc
NASM = nasm
CFLAGS = $(CONFIG) -mpentiumpro -O3 -fomit-frame-pointer -funroll-loops -Iv4lutils `sdl-config --cflags`
#CFLAGS = $(CONFIG) -g -Iv4lutils `sdl-config --cflags`
LIBS = v4lutils/libv4lutils.a -lm `sdl-config --libs`

VLOOPBACKOBJS = vloopback.o
UTILS = utils.o yuv.o buffer.o image.o

PROGRAM = effectv
OBJS = main.o screen.o video.o frequencies.o palette.o $(UTILS)

ifeq ($(USE_VLOOPBACK), yes)
OBJS += $(VLOOPBACKOBJS)
endif

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
