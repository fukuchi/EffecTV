/*
 * EffecTV - Realtime Digital Video Effector
 * Copyright (C) 2001 FUKUCHI Kentarou
 *
 * quark.c: particle effect
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
int quarkDrawDouble();

static char *effectname = "QuarkTV";
static int state = 0;
static unsigned int *buffer;
static unsigned int *planetable[PLANES];
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
	if(scale == 2)
		entry->draw = quarkDrawDouble;
	else
		entry->draw = quarkDraw;
	entry->event = NULL;

	return entry;
}

int quarkStart()
{
	int i;
	
	buffer = (unsigned int *)malloc(SCREEN_AREA*PIXEL_SIZE*PLANES);
	if(buffer == NULL)
		return -1;
	for(i=0;i<PLANES;i++)
		planetable[i] = &buffer[SCREEN_AREA*i];
	plane = PLANES - 1;
	if(video_grabstart())
		return -1;

	state = 1;
	return 0;
}

int quarkStop()
{
	if(state) {
		video_grabstop();
		if(buffer)
			free(buffer);
		state = 0;
	}

	return 0;
}

int quarkDraw()
{
	int i;
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
	for(i=0;i<SCREEN_AREA;i++) {
/* The reason why I use high order 8 bits is written in utils.c
   (or, do 'man rand') */
		cf = (plane + (fastrand()>>24))&(PLANES-1);
		dest[i] = (planetable[cf])[i];
	}
	if(screen_mustlock()) {
		screen_unlock();
	}
	plane--;
	if(plane<0)
		plane = PLANES - 1;

	return 0;
}

int quarkDrawDouble()
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
			cf = (plane + (fastrand()>>24))&(PLANES-1);
			dest[x] = (planetable[cf])[i];
			dest[x+1] = (planetable[cf])[i];
			dest[x+SCREEN_WIDTH*2] = (planetable[cf])[i];
			dest[x+SCREEN_WIDTH*2+1] = (planetable[cf])[i++];
		}
		dest += SCREEN_WIDTH*4;
	}
	if(screen_mustlock()) {
		screen_unlock();
	}
	plane--;
	if(plane<0)
		plane = PLANES - 1;

	return 0;
}
