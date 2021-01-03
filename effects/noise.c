/*
 * EffecTV - Realtime Digital Video Effector
 * Copyright (C) 2001-2006 FUKUCHI Kentaro
 *
 * NoiseTV - make incoming objects noisy.
 * Copyright (C) 2001-2002 FUKUCHI Kentaro
 *
 */

#include <stdlib.h>
#include "EffecTV.h"
#include "utils.h"
#include "bgsub.h"

#define MAGIC_THRESHOLD (40)

static int start(void);
static int stop(void);
static int draw(RGB32 *src, RGB32 *dest);
static int event(SDL_Event *event);

static char *effectname = "NoiseTV";
static int stat;
static BgSubtractor *bgsub;
static int bgIsSet = 0;

static int setBackground(RGB32 *src)
{
	bgsub_bgset_y(bgsub, src);
	bgIsSet = 1;

	return 0;
}

effect *noiseRegister(void)
{
	effect *entry;
	
	entry = (effect *)malloc(sizeof(effect));
	if(entry == NULL) {
		return NULL;
	}

	bgsub = bgsub_new(video_width, video_height);
	if(bgsub == NULL) {
		free(entry);
		return NULL;
	}
	
	entry->name = effectname;
	entry->start = start;
	entry->stop = stop;
	entry->draw = draw;
	entry->event = event;

	bgsub_set_threshold_y(bgsub, MAGIC_THRESHOLD);
	bgIsSet = 0;

	return entry;
}

static int start(void)
{
	stat = 1;
	return 0;
}

static int stop(void)
{
	stat = 0;
	return 0;
}

static int draw(RGB32 *src, RGB32 *dest)
{
	int x, y;
	unsigned char *diff;

	if(!bgIsSet) {
		setBackground(src);
	}

	diff = image_diff_denoise(bgsub_subtract_y(bgsub, src));
	for(y=video_height; y>0; y--) {
		for(x=video_width; x>0; x--) {
			if(*diff++) {
				*dest = 0 - ((inline_fastrand()>>31) & y);
			} else {
				*dest = *src;
			}
			src++;
			dest++;
		}
	}

	return 0;
}

static int event(SDL_Event *event)
{
	if(event->type == SDL_KEYDOWN) {
		switch(event->key.keysym.sym) {
		case SDLK_SPACE:
			bgIsSet = 0;
			break;
		default:
			break;
		}
	}
	return 0;
}
