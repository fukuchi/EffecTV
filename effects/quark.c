/*
 * EffecTV - Realtime Digital Video Effector
 * Copyright (C) 2001-2003 FUKUCHI Kentaro
 *
 * QuarkTV - motion disolver.
 * Copyright (C) 2001-2002 FUKUCHI Kentaro
 *
 */

#include <stdlib.h>
#include <string.h>
#include "EffecTV.h"
#include "utils.h"

#define PLANES 16

static int start(void);
static int stop(void);
static int draw(RGB32 *src, RGB32 *dest);

static char *effectname = "QuarkTV";
static int state = 0;
static RGB32 *buffer;
static RGB32 *planetable[PLANES];
static int plane;

effect *quarkRegister()
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
	entry->event = NULL;

	return entry;
}

static int start()
{
	int i;
	
	buffer = (RGB32 *)malloc(video_area * PIXEL_SIZE * PLANES);
	if(buffer == NULL)
		return -1;
	memset(buffer, 0, video_area * PIXEL_SIZE * PLANES);
	for(i=0;i<PLANES;i++)
		planetable[i] = &buffer[video_area*i];
	plane = PLANES - 1;

	state = 1;

	return 0;
}

static int stop()
{
	if(state) {
		if(buffer) {
			free(buffer);
			buffer = NULL;
		}
		state = 0;
	}

	return 0;
}

static int draw(RGB32 *src, RGB32 *dest)
{
	int i;
	int cf;

	memcpy(planetable[plane], src, video_area * PIXEL_SIZE);

	for(i=0; i<video_area; i++) {
		cf = (plane + (inline_fastrand()>>24))&(PLANES-1);
		dest[i] = (planetable[cf])[i];
		/* The reason why I use high order 8 bits is written in utils.c
		(or, 'man rand') */
	}

	plane--;
	if(plane<0)
		plane = PLANES - 1;

	return 0;
}
