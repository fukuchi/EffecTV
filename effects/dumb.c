/*
 * EffecTV - Realtime Digital Video Effector
 * Copyright (C) 2001 FUKUCHI Kentarou
 *
 * dumb.c: dumb effector
 *
 */

#include <stdlib.h>
#include <string.h>
#include "../EffecTV.h"

int dumbStart();
int dumbStop();
int dumbDraw();
int dumbDrawDouble();

static char *effectname = "DumbTV";
static int state = 0;
static int framelength;

effect *dumbRegister()
{
	effect *entry;

	entry = (effect *)malloc(sizeof(effect));
	if(entry == NULL) return NULL;
	
	entry->name = effectname;
	entry->start = dumbStart;
	entry->stop = dumbStop;
	entry->event = NULL;
	if(scale == 2) {
		if(hireso) {
			framelength = SCREEN_AREA*PIXEL_SIZE*4;
			entry->draw = dumbDraw;
		} else {
			entry->draw = dumbDrawDouble;
		}
	} else {
		framelength = SCREEN_AREA*PIXEL_SIZE;
		entry->draw = dumbDraw;
	}

	return entry;
}

int dumbStart()
{
	if(hireso) {
		if(video_changesize(SCREEN_WIDTH*2, SCREEN_HEIGHT*2))
			return -1;
	}
	if(video_grabstart())
		return -1;
	state = 1;
	return 0;
}

int dumbStop()
{
	if(state) {
		video_grabstop();
		if(hireso){
			video_changesize(0, 0);
		}
		state = 0;
	}

	return 0;
}

int dumbDraw()
{
	if(video_syncframe())
		return -1;
	if(screen_mustlock()) {
		if(screen_lock() < 0) {
			return video_grabframe();
		}
	}
	bcopy(video_getaddress(), screen_getaddress(), framelength);
	if(screen_mustlock()) {
		screen_unlock();
	}

	if(video_grabframe())
		return -1;

	return 0;
}

int dumbDrawDouble()
{
	int x, y;
	unsigned int *src, *dest;
	unsigned int v;

	if(video_syncframe())
		return -1;
	if(screen_mustlock()) {
		if(screen_lock() < 0) {
			return video_grabframe();
		}
	}
	src = (unsigned int *)video_getaddress();
	dest = (unsigned int *)screen_getaddress();
	for(y=0; y<SCREEN_HEIGHT; y++) {
		for(x=0; x<SCREEN_WIDTH; x++) {
			v = *src++;
			*dest = v;
			dest[1] = v;
			dest[SCREEN_WIDTH*2] = v;
			dest[SCREEN_WIDTH*2+1] = v;
			dest += 2;
		}
		dest += SCREEN_WIDTH*2;
	}
	if(screen_mustlock()) {
		screen_unlock();
	}

	if(video_grabframe())
		return -1;

	return 0;
}
