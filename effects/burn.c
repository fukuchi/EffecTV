/*
 * EffecTV - Realtime Digital Video Effector
 * Copyright (C) 2001 FUKUCHI Kentarou
 *
 * BurningTV - burns incoming objects.
 * Copyright (C) 2001 FUKUCHI Kentarou
 *
 * Fire routine is taken from Frank Jan Sorensen's demo program.
 */

#include <stdlib.h>
#include <string.h>
#include "../EffecTV.h"
#include "utils.h"

int burnStart();
int burnStop();
int burnDraw();
int burnEvent(SDL_Event *);

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
		palette[i] = (r<<16)|(g<<8)|b;
	}
	for(i=MaxColor; i<256; i++) {
		if(r<255)r++;if(r<255)r++;if(r<255)r++;
		if(g<255)g++;
		if(g<255)g++;
		if(b<255)b++;
		if(b<255)b++;
		palette[i] = (r<<16)|(g<<8)|b;
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
	entry->start = burnStart;
	entry->stop = burnStop;
	entry->draw = burnDraw;
	entry->event = burnEvent;

	makePalette();

	return entry;
}

int burnStart()
{
	image_set_threshold_y(MAGIC_THRESHOLD);
	bzero(buffer, video_area);
	image_stretching_buffer_clear(0);
	screen_clear(0);
	if(video_grabstart())
		return -1;
	if(setBackground())
		return -1;

	state = 1;
	return 0;
}

int burnStop()
{
	if(state) {
		video_grabstop();
		state = 0;
	}
	return 0;
}

int burnDraw()
{
	int i, x, y;
	unsigned char v, w;
	RGB32 a, b;
	RGB32 *src, *dest;
	unsigned char *diff;

	if(video_syncframe())
		return -1;

	src = (RGB32 *)video_getaddress();
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
		w = 0;
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

	if(screen_mustlock()) {
		if(screen_lock() < 0) {
			return 0;
		}
	}
	if(stretch) {
		dest = stretching_buffer;
	} else {
		dest = (RGB32 *)screen_getaddress();
	}
	i = 1;
	for(y=0; y<video_height; y++) {
		for(x=1; x<video_width-1; x++) {
			a = src[i] & 0xfefeff;
			b = palette[buffer[i]] & 0xfefeff;
			a += b;
			b = a & 0x1010100;
			dest[i] = a | (b - (b >> 8));
			i++;
		}
		i += 2;
	}
	if(stretch) {
		image_stretch_to_screen();
	}
	if(screen_mustlock()) {
		screen_unlock();
	}
	if(video_grabframe())
		return -1;

	return 0;
}

int burnEvent(SDL_Event *event)
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
