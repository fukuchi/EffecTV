/*
 * EffecTV - Realtime Digital Video Effector
 * Copyright (C) 2001 FUKUCHI Kentarou
 *
 * QuarkTV - motion disolver.
 * Copyright (C) 2001 FUKUCHI Kentarou
 *
 */

#include <stdlib.h>
#include <string.h>
#include "../EffecTV.h"
#include "utils.h"

#define PLANES 16

int quarkStart();
int quarkStop();
int quarkDraw();

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
	entry->start = quarkStart;
	entry->stop = quarkStop;
	entry->draw = quarkDraw;
	entry->event = NULL;

	return entry;
}

int quarkStart()
{
	int i;
	
	buffer = (RGB32 *)malloc(video_area*sizeof(RGB32)*PLANES);
	if(buffer == NULL)
		return -1;
	bzero(buffer, video_area*sizeof(RGB32)*PLANES);
	for(i=0;i<PLANES;i++)
		planetable[i] = &buffer[video_area*i];
	plane = PLANES - 1;
	if(video_grabstart()) {
		free(buffer);
		buffer = NULL;
		return -1;
	}

	state = 1;
	return 0;
}

int quarkStop()
{
	if(state) {
		video_grabstop();
		if(buffer) {
			free(buffer);
			buffer = NULL;
		}
		state = 0;
	}

	return 0;
}

int quarkDraw()
{
	int i;
	int cf;
	RGB32 *src, *dest;

	if(video_syncframe())
		return -1;
	src = (RGB32 *)video_getaddress();
	memcpy(planetable[plane], src, video_area*sizeof(RGB32));
	if(video_grabframe())
		return -1;
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
	for(i=0; i<video_area; i++) {
		cf = (plane + (inline_fastrand()>>24))&(PLANES-1);
		dest[i] = (planetable[cf])[i];
		/* The reason why I use high order 8 bits is written in utils.c
		(or, 'man rand') */
	}
	if(stretch) {
		image_stretch_to_screen();
	}
	if(screen_mustlock()) {
		screen_unlock();
	}
	plane--;
	if(plane<0)
		plane = PLANES - 1;

	return 0;
}
