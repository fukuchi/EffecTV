/*
 * EffecTV - Realtime Digital Video Effector
 * Copyright (C) 2001-2006 FUKUCHI Kentaro
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
static int bgIsSet = 0;
static int mode = 1;

static void makePalette(void)
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

static int setBackground(RGB32 *src)
{
	image_bgset_y(src);
	bgIsSet = 1;

	return 0;
}

effect *burnRegister(void)
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

static int start(void)
{
	image_set_threshold_y(MAGIC_THRESHOLD);
	memset(buffer, 0, video_area);
	bgIsSet = 0;

	state = 1;
	return 0;
}

static int stop(void)
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

	if(!bgIsSet) {
		setBackground(src);
	}

	if(mode == 0) {
		diff = image_bgsubtract_y(src);
	} else {
		diff = image_y_over(src);
	}

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
			if(mode == 0) {
				bgIsSet = 0;
			}
			break;
		case SDLK_1:
		case SDLK_KP1:
			mode = 0;
			break;
		case SDLK_2:
		case SDLK_KP2:
			mode = 1;
			break;
		default:
			break;
		}
	}
	return 0;
}
