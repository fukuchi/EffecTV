/*
 * EffecTV - Realtime Digital Video Effector
 * Copyright (C) 2001 FUKUCHI Kentarou
 *
 * MosaicTV - censors incoming objects
 * Copyright (C) 2001 FUKUCHI Kentarou
 *
 */

#include <stdlib.h>
#include <string.h>
#include "../EffecTV.h"
#include "utils.h"

#define MAGIC_THRESHOLD 50
#define CENSOR_LEVEL 20

int mosaicStart();
int mosaicStop();
int mosaicDraw();
int mosaicEvent();

static char *effectname = "MosaicTV";
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

effect *mosaicRegister()
{
	effect *entry;
	
	entry = (effect *)malloc(sizeof(effect));
	if(entry == NULL) {
		return NULL;
	}
	
	entry->name = effectname;
	entry->start = mosaicStart;
	entry->stop = mosaicStop;
	entry->draw = mosaicDraw;
	entry->event = mosaicEvent;

	return entry;
}

int mosaicStart()
{
	image_set_threshold_y(MAGIC_THRESHOLD);
	if(video_grabstart())
		return -1;
	if(setBackground())
		return -1;

	stat = 1;
	return 0;
}

int mosaicStop()
{
	if(stat) {
		video_grabstop();
		stat = 0;
	}

	return 0;
}

int mosaicDraw()
{
	int x, y, xx, yy, v;
	int count;
	RGB32 *src, *dest;
	RGB32 *p, *q;
	unsigned char *diff, *r;

	if(video_syncframe())
		return -1;
	if(screen_mustlock()) {
		if(screen_lock() < 0) {
			return 0;
		}
	}
	src = (RGB32 *)video_getaddress();
	diff = image_bgsubtract_y(src);

	if(stretch) {
		dest = stretching_buffer;
	} else {
		dest = (RGB32 *)screen_getaddress();
	}

	for(y=0; y<video_height-7; y+=8) {
		for(x=0; x<video_width-7; x+=8) {
			count = 0;
			p = &src[y*video_width+x];
			q = &dest[y*video_width+x];
			r = &diff[y*video_width+x];
			for(yy=0; yy<8; yy++) {
				for(xx=0; xx<8; xx++) {
					count += r[yy*video_width+xx];
				}
			}
			if(count > CENSOR_LEVEL*255) {
				v = p[3*video_width+3];
				for(yy=0; yy<8; yy++) {
					for(xx=0; xx<8; xx++){
						q[yy*video_width+xx] = v;
					}
				}
			} else {
				for(yy=0; yy<8; yy++) {
					for(xx=0; xx<8; xx++){
						q[yy*video_width+xx] = p[yy*video_width+xx];
					}
				}
			}
		}
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

int mosaicEvent(SDL_Event *event)
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
