/*
 * EffecTV - Realtime Digital Video Effector
 * Copyright (C) 2001 FUKUCHI Kentarou
 *
 * NoiseTV - make incoming objects noisy.
 * Copyright (C) 2001 FUKUCHI Kentarou
 *
 */

#include <stdlib.h>
#include "../EffecTV.h"
#include "utils.h"

int noiseStart();
int noiseStop();
int noiseDraw();
int noiseEvent();

static char *effectname = "NoiseTV";
static int stat;

static int setBackground()
{
	if(video_syncframe())
		return -1;
	image_bgset_y((RGB32 *)video_getaddress());
	if(video_grabframe())
		return -1;

	return 0;
}

effect *noiseRegister()
{
	effect *entry;
	
	entry = (effect *)malloc(sizeof(effect));
	if(entry == NULL) {
		return NULL;
	}
	
	entry->name = effectname;
	entry->start = noiseStart;
	entry->stop = noiseStop;
	entry->draw = noiseDraw;
	entry->event = noiseEvent;

	return entry;
}

int noiseStart()
{
	image_set_threshold_y(40);
	if(video_grabstart())
		return -1;
	if(setBackground())
		return -1;

	stat = 1;
	return 0;
}

int noiseStop()
{
	if(stat) {
		video_grabstop();
		stat = 0;
	}

	return 0;
}

int noiseDraw()
{
	int i;
	RGB32 *src, *dest;
	unsigned char *diff;

	if(video_syncframe())
		return -1;
	if(screen_mustlock()) {
		if(screen_lock() < 0) {
			return 0;
		}
	}
	src = (RGB32 *)video_getaddress();
	if(stretch) {
		dest = stretching_buffer;
	} else {
		dest = (RGB32 *)screen_getaddress();
	}

	diff = image_diff_filter(image_bgsubtract_y(src));
	for(i=0; i<video_area; i++) {
		if(*diff++) {
			*dest = 0 - (inline_fastrand()>>31);
		} else {
			*dest = *src;
		}
		src++;
		dest++;
	}
	if(stretch) {
		image_stretch_to_screen();
	}
	if(screen_mustlock()) {
		screen_unlock();
	}
	if(video_grabframe())
		return -1;

	return 0;
}

int noiseEvent(SDL_Event *event)
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
