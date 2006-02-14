/*
 * EffecTV - Realtime Digital Video Effector
 * Copyright (C) 2001-2006 FUKUCHI Kentaro
 *
 * nervousTV - The name says it all...
 * Copyright (C) 2002 TANNENBAUM Edo
 *
 * 2002/2/9 
 *   Original code copied same frame twice, and did not use memcpy().
 *   I modifed those point.
 *   -Kentaro Fukuchi
 */

#include <stdlib.h>
#include <string.h>
#include "EffecTV.h"
#include "utils.h"

#define PLANES 32


static int start(void);
static int stop(void);
static int draw(RGB32 *src, RGB32 *dest);
static int event(SDL_Event *event);

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

effect *nervousRegister(void)
{
	effect *entry;

	entry = (effect *)malloc(sizeof(effect));
	if(entry == NULL) {
		return NULL;
	}

	entry->name = effectname;
	entry->start = start;
	entry->stop = stop;
	entry->draw = draw;
	entry->event = event;

	return entry;
}

static int start(void)
{
	int i;

	buffer = (RGB32 *)malloc(video_area*PIXEL_SIZE*PLANES);
	if(buffer == NULL)
		return -1;
	memset(buffer, 0, video_area*PIXEL_SIZE*PLANES);
	for(i=0;i<PLANES;i++)
		planetable[i] = &buffer[video_area*i];

	plane = 0;
	stock = 0;
	timer = 0;
	readplane = 0;

	state = 1;
	return 0;
}

static int stop(void)
{
	if(state) {
		if(buffer)
			free(buffer);
		state = 0;
	}

	return 0;
}

static int draw(RGB32 *src, RGB32 *dest)
{
	memcpy(planetable[plane], src, video_area * PIXEL_SIZE);
	if(stock < PLANES) {
		stock++;
	}

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
	memcpy(dest, planetable[readplane], video_area * PIXEL_SIZE);
	plane++;
	if (plane == PLANES) plane=0;

	return 0;
}

static int event(SDL_Event *event)
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
