/*
 * EffecTV - Realtime Digital Video Effector
 * Copyright (C) 2001 FUKUCHI Kentarou
 *
 * DumbTV - no effect.
 * Copyright (C) 2001 FUKUCHI Kentarou
 *
 */

#include <stdlib.h>
#include <string.h>
#include "../EffecTV.h"
#include "utils.h"

int dumbStart();
int dumbStop();
int dumbDraw();

static char *effectname = "DumbTV";
static int state = 0;

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

	return entry;
}

int dumbStart()
{
	if(video_grabstart())
		return -1;
	state = 1;
	return 0;
}

int dumbStop()
{
	if(state) {
		video_grabstop();
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
	if(stretch) {
		image_stretch((RGB32 *)video_getaddress(), video_width, video_height,
		              (RGB32 *)screen_getaddress(),
					  screen_width, screen_height);
	} else {
		memcpy(screen_getaddress(), video_getaddress(),
		      video_width * video_height * sizeof(RGB32));
	}
	if(screen_mustlock()) {
		screen_unlock();
	}

	if(video_grabframe())
		return -1;

	return 0;
}
