# Makefile for EffecTV

CONFIG = -DVLOOPBACK

CC = gcc
NASM = nasm
CFLAGS = $(CONFIG) -mpentiumpro -O3 -fomit-frame-pointer -funroll-loops -Iv4lutils `sdl-config --cflags`
#CFLAGS = $(CONFIG) -g -Iv4lutils `sdl-config --cflags`
LIBS = v4lutils/libv4lutils.a -lm `sdl-config --libs`

VLOOPBACKOBJS = vloopback.o

PROGRAM = effectv
OBJS = main.o screen.o video.o frequencies.o $(VLOOPBACKOBJS)
LIBEFFECTS = effects/libeffects.a
SUBDIRS = effects v4lutils

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

$(OBJS): EffecTV.h

clean:
	rm -f *.o $(PROGRAM)
	cd effects && $(MAKE) clean
	cd v4lutils && $(MAKE) clean
