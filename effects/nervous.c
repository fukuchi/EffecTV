/*
 * EffecTV - Realtime Digital Video Effector
 * Copyright (C) 2001 FUKUCHI Kentarou
 *
 * nervousTV - The name says it all...
 * Copyright (C) 2002 TANNENBAUM Edo
 *
 * 2002/2/9 
 *   Original code copied same frame twice, and did not use memcpy().
 *   I modifed those point.
 *   -Kentarou Fukuchi
 */

#include <stdlib.h>
#include <string.h>
#include "../EffecTV.h"
#include "utils.h"

#define PLANES 32


int nervousStart();
int nervousStop();
int nervousDraw();
int nervousEvent();

static char *effectname = "NervousTV";
static int state = 0;
static RGB32 *buffer;
static RGB32 *planetable[PLANES];
static int mode = 0;
static int plane;
static int stock;
static int timer;
static int stride;
static int readplane;

effect *nervousRegister()
{
	effect *entry;

	entry = (effect *)malloc(sizeof(effect));
	if(entry == NULL) {
		free(buffer);
		return NULL;
	}

	entry->name = effectname;
	entry->start = nervousStart;
	entry->stop = nervousStop;
	entry->draw = nervousDraw;
	entry->event = nervousEvent;

	return entry;
}

int nervousStart()
{
	int i;

	buffer = (RGB32 *)malloc(video_area*sizeof(RGB32)*PLANES);
	if(buffer == NULL)
		return -1;
	bzero(buffer, video_area*sizeof(RGB32)*PLANES);
	for(i=0;i<PLANES;i++)
		planetable[i] = &buffer[video_area*i];

	plane = 0;
	stock = 0;
	timer = 0;
	readplane = 0;
	if(video_grabstart())
		return -1;

	state = 1;
	return 0;
}

int nervousStop()
{
	if(state) {
		video_grabstop();
		if(buffer)
			free(buffer);
		state = 0;
	}

	return 0;
}

int nervousDraw()
{
	RGB32 *src, *dest;

	if(video_syncframe())
		return -1;

	src = (RGB32 *)video_getaddress();
	memcpy(planetable[plane], src, video_area * PIXEL_SIZE);
	if(stock < PLANES) {
		stock++;
	}

	if(video_grabframe())
		return -1;

	if(mode) {
		if(timer) {
			readplane = readplane + stride;
			while(readplane < 0) readplane += stock;
			while(readplane >= stock) readplane -= stock;
			timer--;
		} else {
			readplane = inline_fastrand() % stock;
			stride = inline_fastrand() % 5 - 2;
			if(stride >= 0) stride++;
			timer = inline_fastrand() % 6 + 2;
		}
	} else {
		if(stock > 0)
			readplane = inline_fastrand() % stock;
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
	memcpy(dest, planetable[readplane], video_area * PIXEL_SIZE);
	if(stretch) {
		image_stretch_to_screen();
	}
	if(screen_mustlock()) {
		screen_unlock();
	}
	plane++;
	if (plane == PLANES) plane=0;

	return 0;
}

int nervousEvent(SDL_Event *event)
{
	if(event->type == SDL_KEYDOWN) {
		switch(event->key.keysym.sym) {
		case SDLK_SPACE:
			mode ^= 1;
			break;
		default:
			break;
		}
	}
	return 0;
}
