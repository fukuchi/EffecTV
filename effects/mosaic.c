/*
 * EffecTV - Realtime Digital Video Effector
 * Copyright (C) 2001-2006 FUKUCHI Kentaro
 *
 * MosaicTV - censors incoming objects
 * Copyright (C) 2001-2002 FUKUCHI Kentaro
 *
 */

#include <stdlib.h>
#include <string.h>
#include "EffecTV.h"
#include "utils.h"

#define MAGIC_THRESHOLD 50
#define CENSOR_LEVEL 20

static int start(void);
static int stop(void);
static int draw(RGB32 *src, RGB32 *dest);
static int event(SDL_Event *event);

static char *effectname = "MosaicTV";
static int stat;
static int bgIsSet = 0;

static int setBackground(RGB32 *src)
{
	image_bgset_y(src);
	bgIsSet = 1;

	return 0;
}

effect *mosaicRegister(void)
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

static int start(void)
{
	image_set_threshold_y(MAGIC_THRESHOLD);
	bgIsSet = 0;

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
	int x, y, xx, yy, v;
	int count;
	RGB32 *p, *q;
	unsigned char *diff, *r;

	if(!bgIsSet) {
		setBackground(src);
	}

	diff = image_bgsubtract_y(src);

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
