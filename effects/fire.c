/*
 * EffecTV - Realtime Digital Video Effector
 * Copyright (C) 2001-2002 FUKUCHI Kentaro
 *
 * FireTV - clips incoming objects and burn them.
 * Copyright (C) 2001-2002 FUKUCHI Kentaro
 *
 * Fire routine is taken from Frank Jan Sorensen's demo program.
 */

#include <stdlib.h>
#include <string.h>
#include "../EffecTV.h"
#include "utils.h"

int fireStart();
int fireStop();
int fireDraw();
int fireEvent(SDL_Event *);

#define MaxColor 120
#define Decay 15
#define MAGIC_THRESHOLD 50

static char *effectname = "FireTV";
static int state = 0;
static unsigned char *buffer;
static RGB32 palette[256];

static int setBackground()
{
	if(video_syncframe())
		return -1;
	image_bgset_y((RGB32 *)video_getaddress());
	if(video_grabframe())
		return -1;

	return 0;
}

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

effect *fireRegister()
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
	entry->start = fireStart;
	entry->stop = fireStop;
	entry->draw = fireDraw;
	entry->event = fireEvent;

	makePalette();

	return entry;
}

int fireStart()
{
	image_set_threshold_y(MAGIC_THRESHOLD);
	bzero(buffer, video_area);
	screen_clear(0);
	image_stretching_buffer_clear(0);
	if(video_grabstart())
		return -1;
	if(setBackground())
		return -1;

	state = 1;
	return 0;
}

int fireStop()
{
	if(state) {
		video_grabstop();
		state = 0;
	}

	return 0;
}

int fireDraw()
{
	int i, x, y;
	unsigned char v;
	RGB32 *dest;
	unsigned char *diff;

	if(video_syncframe())
		return -1;

	diff = image_bgsubtract_y((RGB32 *)video_getaddress());
	for(i=0; i<video_area-video_width; i++) {
		buffer[i] |= diff[i];
	}
	if(video_grabframe())
		return -1;

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

	if(screen_mustlock()) {
		if(screen_lock() < 0) {
			return video_grabframe();
		}
	}
	if(stretch) {
		dest = stretching_buffer;
	} else {
		dest = (RGB32 *)screen_getaddress();
	}
	for(y=0; y<video_height; y++) {
		for(x=1; x<video_width-1; x++) {
			dest[y*video_width+x] = palette[buffer[y*video_width+x]];
		}
	}
	if(stretch) {
		image_stretch_to_screen();
	}
	if(screen_mustlock()) {
		screen_unlock();
	}

	return 0;
}

int fireEvent(SDL_Event *event)
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
