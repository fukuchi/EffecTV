# Makefile for EffecTV

CC = gcc
NASM = nasm
CFLAGS = -mpentiumpro -O3 -fomit-frame-pointer -funroll-loops -I.. -I../v4lutils `sdl-config --cflags`
#CFLAGS = -g -I.. -I../v4lutils `sdl-config --cflags`
LIBS = ../v4lutils/libv4lutils.a -lm `sdl-config --libs` -lm

LIBEFFECTS = libeffects.a
EFFECTS = $(QUARK) $(DUMB) $(FIRE) $(BURN) $(BLURZOOM) $(BALTAN) $(STREAK) \
          $(ONED) $(DOT) $(MOSAIC) $(PUZZLE) $(PREDATOR) $(SPIRAL) $(SIMURA) \
          $(EDGE) $(SHAGADELIC) $(NOISE) $(AGING) \
          $(TRANSFORM) $(LIFE) $(SPARK) $(WARP) $(HOLO) $(CYCLE) $(RIPPLE) \
          $(DICE)
SUBDIRS =

%.o: %.c
	$(CC) $(CFLAGS) -Wall -c -o $@ $<
%.o: %.nas
	$(NASM) -f elf $<

# just for checking optimized code
%.s: %.c
	$(CC) $(CFLAGS) -S -Wall -c -o $@ $<

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
MOSAIC = mosaic.o
PUZZLE = puzzle.o
PREDATOR = predator.o
SPIRAL = spiral.o
SIMURA = simura.o
EDGE = edge.o
SHAGADELIC = shagadelic.o
NOISE = noise.o
STROBE = strobe.o
AGING = aging.o
TRANSFORM = transform.o
LIFE = life.o
SPARK = spark.o
THREAD = thread.o
WARP = warp.o
HOLO = holo.o
CYCLE = cycle.o
RIPPLE = ripple.o
DICE = dice.o

$(LIBEFFECTS): $(EFFECTS)
	ar rc $(LIBEFFECTS) $(EFFECTS)

clean:
	rm -f *.o $(LIBEFFECTS)

$(EFFECTS): ../EffecTV.h ../screen.h ../video.h ../utils.h
