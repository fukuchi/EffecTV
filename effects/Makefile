# Makefile for EffecTV

CC = gcc
NASM = nasm
CFLAGS = -mpentiumpro -O3 -fomit-frame-pointer -funroll-loops -Iv4lutils `sdl-config --cflags`
#CFLAGS = -g -Iv4lutils `sdl-config --cflags`
LIBS = v4lutils/libv4lutils.a -lm `sdl-config --libs`

LIBEFFECTS = libeffects.a
EFFECTS = $(QUARK) $(DUMB) $(FIRE) $(BURN) $(BLURZOOM) $(BALTAN) $(STREAK) \
          $(ONED) $(DOT)
UTILS = utils.o buffer.o
SUBDIRS =

%.o: %.c
	$(CC) $(CFLAGS) -Wall -c -o $@ $<
%.o: %.nas
	$(NASM) -f elf $<

all-recursive:
	@list='$(SUBDIRS)'; for subdir in $$list; do \
	(cd $$subdir && $(MAKE) all-recursive) \
	done; \
	$(MAKE) all-am

all-am: $(LIBEFFECTS)

DUMB = dumb.o
QUARK = quark.o
FIRE = fire.o
BURN = burn.o
BLURZOOM = blurzoom.o blurzoomcore.o
BALTAN = baltan.o
STREAK = streak.o
ONED = 1d.o
DOT = dot.o

$(LIBEFFECTS): $(EFFECTS) $(UTILS)
	ar rc $(LIBEFFECTS) $(EFFECTS) $(UTILS)

clean:
	rm -f *.o $(LIBEFFECTS)

$(EFFECTS): ../EffecTV.h ../screen.h ../video.h
