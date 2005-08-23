# Makefile for EffecTV

include ../config.mk

CC = gcc
NASM = nasm
CFLAGS = $(CONFIG) $(CONFIG.arch) $(CFLAGS.opt) -I.. -I../v4lutils `sdl-config --cflags`
LIBS = ../v4lutils/libv4lutils.a -lm `sdl-config --libs`

LIBEFFECTS = libeffects.a
EFFECTS = $(DUMB) $(QUARK) $(FIRE) $(BURN) $(BLURZOOM) $(BALTAN) $(STREAK) \
		  $(ONED) $(DOT) $(MOSAIC) $(PUZZLE) $(PREDATOR) $(SPIRAL) \
		  $(SIMURA) $(EDGE) $(SHAGADELIC) $(NOISE) $(AGING) $(TRANSFORM) \
		  $(LIFE) $(SPARK) $(WARP) $(HOLO) $(CYCLE) $(RIPPLE) \
          $(DICE) $(DIZZY) $(DEINTERLACE) $(NERVOUS) $(RNDM) $(REV) $(RDS) \
          $(LENS) $(DIFF) $(SCROLL) $(WARHOL) $(MATRIX) $(PUP) \
		  $(CHAMELEON) $(OP) $(HALF) $(SLOFAST) $(DISPLAYWALL) $(BLUESCREEN) \
		  $(COLSTREAK) $(TIMEDIST) $(EDGEBLUR)

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
BLURZOOM = blurzoom.o
ifeq ($(USE_NASM), yes)
BLURZOOM += blurzoomcore.o
endif
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
WARP = warp.o
HOLO = holo.o
CYCLE = cycle.o
RIPPLE = ripple.o
DICE = dice.o
DIZZY = dizzy.o
DEINTERLACE = Deinterlace.o
NERVOUS = nervous.o
RNDM = rndm.o
REV = rev.o
RDS = rds.o
LENS = lens.o
DIFF = diff.o
SCROLL = scroll.o
WARHOL = warhol.o
MATRIX = matrix.o
PUP = pup.o
CHAMELEON = chameleon.o
OP = op.o
HALF = half.o
SLOFAST = slofast.o
DISPLAYWALL = displaywall.o
BLUESCREEN = bluescreen.o
COLSTREAK = colstreak.o
TIMEDIST = timedist.o
EDGEBLUR = edgeblur.o

$(LIBEFFECTS): $(EFFECTS)
	ar rc $(LIBEFFECTS) $(EFFECTS)

clean:
	rm -f *.o $(LIBEFFECTS)

$(EFFECTS): ../EffecTV.h ../screen.h ../video.h ../utils.h
