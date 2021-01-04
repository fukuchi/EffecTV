/*
 * EffecTV - Realtime Digital Video Effector
 * Copyright (C) 2001-2006 FUKUCHI Kentaro
 *
 * PredatorTV - makes incoming objects invisible like the Predator.
 * Copyright (C) 2001-2002 FUKUCHI Kentaro
 *
 */

#include <stdlib.h>
#include <string.h>
#include "EffecTV.h"
#include "utils.h"

static int start(void);
static int stop(void);
static int draw(RGB32 *src, RGB32 *dest);
static int event(SDL_Event *);
static void myFree(void);

#define MAGIC_THRESHOLD 40

static char *effectname = "PredatorTV";
static int state = 0;
static RGB32 *bgimage;
static int bgIsSet;

static void setBackground(RGB32 *src)
{
	memcpy(bgimage, src, video_area * PIXEL_SIZE);
	image_bgset_y(bgimage);
	bgIsSet = 1;
}

effect *predatorRegister(void)
{
	effect *entry;

	entry = (effect *)malloc(sizeof(effect));
	if(entry == NULL) {
		return NULL;
	}

	bgimage = (RGB32 *)malloc(video_area*PIXEL_SIZE);
	if(bgimage == NULL) {
		free(entry);
		return NULL;
	}

	entry->name = effectname;
	entry->start = start;
	entry->stop = stop;
	entry->draw = draw;
	entry->event = event;
	entry->free = myFree;

	return entry;
}

static int start(void)
{
	image_set_threshold_y(MAGIC_THRESHOLD);
	bgIsSet = 0;

	state = 1;
	return 0;
}

static int stop(void)
{
	state = 0;
	return 0;
}

static int draw(RGB32 *src, RGB32 *dest)
{
	int x, y;
	unsigned char *diff;

	if(!bgIsSet) {
		setBackground(src);
	}

	diff = image_bgsubtract_y(src);
	diff = image_diff_filter(diff);

	dest += video_width;
	diff += video_width;
	src = bgimage + video_width;
	for(y=1; y<video_height-1; y++) {
		for(x=0; x<video_width; x++) {
			if(*diff){
				*dest = src[4] & 0xfcfcfc;
			} else {
				*dest = *src;
			}
			diff++;
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

static void myFree(void)
{
	free(bgimage);
}
