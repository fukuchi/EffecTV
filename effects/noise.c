/*
 * EffecTV - Realtime Digital Video Effector
 * Copyright (C) 2001-2003 FUKUCHI Kentaro
 *
 * NoiseTV - make incoming objects noisy.
 * Copyright (C) 2001-2002 FUKUCHI Kentaro
 *
 */

#include <stdlib.h>
#include "EffecTV.h"
#include "utils.h"

static int start(void);
static int stop(void);
static int draw(RGB32 *src, RGB32 *dest);
static int event();

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
	entry->start = start;
	entry->stop = stop;
	entry->draw = draw;
	entry->event = event;

	return entry;
}

static int start()
{
	image_set_threshold_y(40);
	if(setBackground())
		return -1;

	stat = 1;
	return 0;
}

static int stop()
{
	stat = 0;
	return 0;
}

static int draw(RGB32 *src, RGB32 *dest)
{
	int i;
	unsigned char *diff;

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

	return 0;
}

static int event(SDL_Event *event)
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
