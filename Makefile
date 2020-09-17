# Makefile for EffecTV

include ./config.mk

CC = gcc
INSTALL = /usr/bin/install -c

CFLAGS = $(CONFIG) $(CONFIG.arch) $(CFLAGS.opt) -Iv4lutils `pkg-config --cflags sdl libv4l2`
LIBS = v4lutils/libv4lutils.a -lm -lpthread `pkg-config --libs sdl libv4l2` $(LIBS.extra)

PROGRAM = effectv

COREOBJS = main.o screen.o video.o
VLOOPBACKOBJS = vloopback.o
UTILS = utils.o yuv.o buffer.o image.o

OBJS = $(COREOBJS) $(UTILS)

ifeq ($(USE_VLOOPBACK), yes)
OBJS += $(VLOOPBACKOBJS)
endif

LIBEFFECTS = effects/libeffects.a
SUBDIRS = effects v4lutils

### rules

%.o: %.c
	$(CC) $(CFLAGS) -Wall -c -o $@ $<

all-recursive:
	@list='$(SUBDIRS)'; for subdir in $$list; do \
	(cd $$subdir && $(MAKE) all-recursive) || exit 1;\
	done; \
	$(MAKE) all-am

all-am: $(PROGRAM)

$(PROGRAM): $(OBJS) $(LIBEFFECTS) v4lutils/libv4lutils.a
	$(CC) -o $@ $(OBJS) $(LIBEFFECTS) $(LIBS)

$(OBJS): EffecTV.h screen.h video.h utils.h

install: all-am
	$(INSTALL) -s $(PROGRAM) $(bindir)/
	$(INSTALL) effectv.1 $(mandir)/man1/

clean:
	rm -f *.o $(PROGRAM)
	cd effects && $(MAKE) clean
	cd v4lutils && $(MAKE) clean
