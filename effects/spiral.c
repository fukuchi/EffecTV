/*
 * EffecTV - Realtime Digital Video Effector
 * Copyright (C) 2001 FUKUCHI Kentarou
 *
 * spiral.c: a 'spiraling' effect (even though it isn't really a spiral)
 *  heavily based upon quark.c; changes copyright (c) 2001 Sam Mertens
 *
 *  Note: When processing interlaced video (as from the input of my TV card),
 *  only the double-size mode seems to work as intended.
 *
 *  I haven't even bothered to try to optimize this thing yet; one sqrt()
 *  per pixel seems rather expensive, but my Duron 650 takes it in stride.
 *  There are many, many ways of speeding this thing up if needed, however.
 *  (A precalculated depth field, for instance.)
 *
 *  I also haven't cleaned the code up much, either.  It is a work in progress.
 *
 *	-Sam Mertens
 */

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "../EffecTV.h"
#include "utils.h"

#define PLANES 16       // A lot of code depends upon this being a power of 2
#define PLANE_MASK  (PLANES - 1)
#define PLANE_MAX   (PLANES - 1)

int spiralStart();
int spiralStop();
int spiralDraw();
int spiralDrawDouble();
int spiralEvent();

static char *effectname = "SpiralTV";
static int state = 0;
static unsigned int *buffer;
static unsigned int *planetable[PLANES];
static int plane;
static int format;
static int *phasetable;
static int *ptab;
static int mode = 0;

effect *spiralRegister()
{
	effect *entry;
    
	sharedbuffer_reset();
	phasetable = (int *)sharedbuffer_alloc(SCREEN_AREA*2*sizeof(int));
	if(phasetable == NULL) {
		return NULL;
	}

	entry = (effect *)malloc(sizeof(effect));
	if(entry == NULL) {
		free(buffer);
		return NULL;
	}

	entry->name = effectname;
	entry->start = spiralStart;
	entry->stop = spiralStop;
	if(scale == 2)
		entry->draw = spiralDrawDouble;
	else
		entry->draw = spiralDraw;
	entry->event = spiralEvent;

	return entry;
}

int spiralStart()
{
	int i;
	int x, y, yy, w, v;
	
	buffer = (unsigned int *)malloc(SCREEN_AREA * PIXEL_SIZE * PLANES);
	if(buffer == NULL)
		return -1;
	for(i=0;i<PLANES;i++) {
		planetable[i] = &buffer[SCREEN_AREA * i];
    }

	ptab = phasetable;
	for(y=0; y<SCREEN_HHEIGHT; y++) {
		yy = y*y;
		for(x=0; x<SCREEN_HWIDTH; x++) {
			v = ((int)sqrt(yy + x * x)) & PLANE_MASK;
			ptab[(SCREEN_HHEIGHT-1-y)*SCREEN_WIDTH+SCREEN_HWIDTH-1-x] = v;
			ptab[(SCREEN_HHEIGHT-1-y)*SCREEN_WIDTH+SCREEN_HWIDTH+x  ] = v;
			ptab[(SCREEN_HHEIGHT+y)*SCREEN_WIDTH  +SCREEN_HWIDTH-1-x] = v;
			ptab[(SCREEN_HHEIGHT+y)*SCREEN_WIDTH  +SCREEN_HWIDTH+x  ] = v;
		}
	}
	ptab = phasetable + SCREEN_AREA;
	w = 1+sqrt(SCREEN_HWIDTH*SCREEN_HWIDTH+SCREEN_HHEIGHT*SCREEN_HHEIGHT)/PLANES;
	for(y=0; y<SCREEN_HHEIGHT; y++) {
		yy = y*y;
		for(x=0; x<SCREEN_HWIDTH; x++) {
			v = (((int)sqrt(yy + x*x)) / w) & PLANE_MASK;
			ptab[(SCREEN_HHEIGHT-1-y)*SCREEN_WIDTH+SCREEN_HWIDTH-1-x] = v;
			ptab[(SCREEN_HHEIGHT-1-y)*SCREEN_WIDTH+SCREEN_HWIDTH+x  ] = v;
			ptab[(SCREEN_HHEIGHT+y)*SCREEN_WIDTH  +SCREEN_HWIDTH-1-x] = v;
			ptab[(SCREEN_HHEIGHT+y)*SCREEN_WIDTH  +SCREEN_HWIDTH+x  ] = v;
		}
	}
    
	plane = PLANE_MAX;
	ptab = phasetable + mode*SCREEN_AREA;
	format = video_getformat();
	if(video_setformat(VIDEO_PALETTE_RGB32))
		return -1;
	if(video_grabstart())
		return -1;

	state = 1;
	return 0;


#if 0
    // Wave definition
    wave_adder = 1;
    wave_value = 0;
	for(i=0;i<256;i++) {
        // Sawtooth:
		// phasetable[i] = i & PLANE_MASK;

        phasetable[i] = wave_value;

        wave_value += wave_adder;
        // The following means of testing causes duplicate values of
        // wave_value at each peak and valley.
        if ( (~ PLANE_MASK) & wave_value)
        {
            wave_adder = ~ wave_adder;
            wave_value += wave_adder;
        }
	}
#endif
}

int spiralStop()
{
	if(state) {
		video_grabstop();
		video_setformat(format);
		if(buffer)
			free(buffer);
		state = 0;
	}

	return 0;
}

int spiralDraw()
{
    int x, y, i;
	int cf;
	unsigned int *src, *dest;

	if(video_syncframe())
		return -1;
	src = (unsigned int *)video_getaddress();
	dest = (unsigned int *)screen_getaddress();
	bcopy(src, planetable[plane], SCREEN_AREA * PIXEL_SIZE);
	if(video_grabframe())
		return -1;
	if(screen_mustlock()) {
		if(screen_lock() < 0) {
			return 0;
		}
	}

	i = 0;
	for(y = 0; y < SCREEN_HEIGHT; y++) {
		for(x = 0; x < SCREEN_WIDTH; x++) {
			cf = (plane + ptab[i]) & PLANE_MASK;
			dest[i] = (planetable[cf])[i];
			i++;
		}
	}
    
	if(screen_mustlock()) {
		screen_unlock();
	}
	screen_update();
	plane--;
	plane &= PLANE_MASK;

	return 0;
}

int spiralDrawDouble()
{
	int i, x, y;
	int cf;
	unsigned int *src, *dest;

	if(video_syncframe())
		return -1;
	src = (unsigned int *)video_getaddress();
	dest = (unsigned int *)screen_getaddress();
	bcopy(src, planetable[plane], SCREEN_AREA*PIXEL_SIZE);
	if(video_grabframe())
		return -1;
	if(screen_mustlock()) {
		if(screen_lock() < 0) {
			return 0;
		}
	}
	i=0;
	for(y=0; y<SCREEN_HEIGHT; y++) {
		for(x=0; x<SCREEN_WIDTH*2; x+=2) {
			cf = (plane + ptab[i]) & PLANE_MASK;
			dest[x] = dest[x+1] = (planetable[cf])[i];
			dest[x+SCREEN_WIDTH*2] = (planetable[cf])[i];
			dest[x+SCREEN_WIDTH*2+1] = (planetable[cf])[i++];
		}
		dest += SCREEN_WIDTH*4;
	}
	if(screen_mustlock()) {
		screen_unlock();
	}
	screen_update();
	plane--;
	plane &= PLANE_MASK;

	return 0;
}

int spiralEvent(SDL_Event *event)
{
	if(event->type == SDL_KEYDOWN) {
		switch(event->key.keysym.sym) {
		case SDLK_SPACE:
			mode = mode ^ 1;
			ptab = phasetable + mode * SCREEN_AREA;
			break;
		default:
			break;
		}
	}
	return 0;
}
