/*
 * EffecTV - Realtime Digital Video Effector
 * Copyright (C) 2001-2003 FUKUCHI Kentaro
 *
 * BurningTV - burns incoming objects.
 * Copyright (C) 2001-2002 FUKUCHI Kentaro
 *
 * Fire routine is taken from Frank Jan Sorensen's demo program.
 */

#include <stdlib.h>
#include <string.h>
#include "EffecTV.h"
#include "utils.h"

static int start(void);
static int stop(void);
static int draw(RGB32 *src, RGB32 *dest);
static int event(SDL_Event *);

#define MaxColor 120
#define Decay 15
#define MAGIC_THRESHOLD 50

static char *effectname = "BurningTV";
static int state = 0;
static unsigned char *buffer;
static RGB32 palette[256];

static void makePalette()
{
	int i, r, g, b;

	for(i=0; i<MaxColor; i++) {
		HSItoRGB(4.6-1.5*i/MaxColor, (double)i/MaxColor, (double)i/MaxColor, &r, &g, &b);
		palette[i] = ((r<<16)|(g<<8)|b) & 0xfefeff;
	}
	for(i=MaxColor; i<256; i++) {
		if(r<255)r++;if(r<255)r++;if(r<255)r++;
		if(g<255)g++;
		if(g<255)g++;
		if(b<255)b++;
		if(b<255)b++;
		palette[i] = ((r<<16)|(g<<8)|b) & 0xfefeff;
	}
}

static int setBackground()
{
	if(video_syncframe())
		return -1;
	image_bgset_y((RGB32 *)video_getaddress());
	if(video_grabframe())
		return -1;

	return 0;
}

effect *burnRegister()
{
	effect *entry;
	
	sharedbuffer_reset();
	buffer = (unsigned char *)sharedbuffer_alloc(video_area);
	if(buffer == NULL) {
		return NULL;
	}

	entry = (effect *)malloc(sizeof(effect));
	if(entry == NULL) {
		return NULL;
	}
	
	entry->name = effectname;
	entry->start = start;
	entry->stop = stop;
	entry->draw = draw;
	entry->event = event;

	makePalette();

	return entry;
}

static int start()
{
	image_set_threshold_y(MAGIC_THRESHOLD);
	memset(buffer, 0, video_area);
	if(setBackground())
		return -1;

	state = 1;
	return 0;
}

static int stop()
{
	state = 0;
	return 0;
}

static int draw(RGB32 *src, RGB32 *dest)
{
	int i, x, y;
	unsigned char v, w;
	RGB32 a, b;
	unsigned char *diff;

	diff = image_bgsubtract_y(src);
	for(x=1; x<video_width-1; x++) {
		v = 0;
		for(y=0; y<video_height-1; y++) {
			w = diff[y*video_width+x];
			buffer[y*video_width+x] |= v ^ w;
			v = w;
		}
	}
	for(x=1; x<video_width-1; x++) {
		i = video_width + x;
		for(y=1; y<video_height; y++) {
			v = buffer[i];
			if(v<Decay)
				buffer[i-video_width] = 0;
			else
				buffer[i-video_width+fastrand()%3-1] = v - (fastrand()&Decay);
			i += video_width;
		}
	}

	i = 1;
	for(y=0; y<video_height; y++) {
		for(x=1; x<video_width-1; x++) {
			a = (src[i] & 0xfefeff) + palette[buffer[i]];
			b = a & 0x1010100;
			dest[i] = a | (b - (b >> 8));
			i++;
		}
		i += 2;
	}

	return 0;
}

static int event(SDL_Event *event)
{
	if(event->type == SDL_KEYDOWN) {
		switch(event->key.keysym.sym) {
		case SDLK_SPACE:
			setBackground();
			break;
		default:
			break;
		}
	}
	return 0;
}
