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

static char *effectname = "DumbTV";
static int state = 0;
static int format;
static int framelength;

effect *dumbRegister()
{
	effect *entry;

	entry = (effect *)malloc(sizeof(effect));
	if(entry == NULL) return NULL;
	
	entry->name = effectname;
	entry->start = dumbStart;
	entry->stop = dumbStop;
	entry->draw = dumbDraw;
	entry->event = NULL;
	if(scale == 2) {
		framelength = SCREEN_AREA*PIXEL_SIZE*4;
	} else {
		framelength = SCREEN_AREA*PIXEL_SIZE;
	}

	return entry;
}

int dumbStart()
{
	format = video_getformat();
	video_setformat(VIDEO_PALETTE_RGB32);
	if(scale == 2){
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
		video_setformat(format);
		if(scale == 2){
			video_changesize(0, 0);
		}
		state = 0;
	}

	return 0;
}

int dumbDraw()
{
	video_syncframe();

	if(screen_mustlock()) {
		if(screen_lock() < 0) {
			return video_grabframe();
		}
	}
	bcopy(video_getaddress(), screen_getaddress(), framelength);
	if(screen_mustlock()) {
		screen_unlock();
	}
	screen_update();
	return video_grabframe();
}
