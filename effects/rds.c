/*
 * EffecTV - Realtime Digital Video Effector
 * Copyright (C) 2001-2006 FUKUCHI Kentarou
 *
 * RandomDotStereoTV - makes random dot stereogram.
 * Copyright (C) 2002 FUKUCHI Kentarou
 *
 */

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "EffecTV.h"
#include "utils.h"

static int start(void);
static int stop(void);
static int draw(RGB32 *src, RGB32 *dest);
static int event(SDL_Event *event);

static char *effectname = "RandomDotStereoTV";
static int stat;
static int stride = 40;
static int method = 0;

effect *rdsRegister(void)
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
	int x, y, i;
	RGB32 *target;
	RGB32 v;
	RGB32 R, G, B;

	memset(dest, 0, video_area * PIXEL_SIZE);
	target = dest;

	if(method) {
		for(y=0; y<video_height; y++) {
			for(i=0; i<stride; i++) {
				if(inline_fastrand()&0xc0000000)
					continue;

				x = video_width / 2 + i;
				*(dest + x) = 0xffffff;
	
				while(x + stride/2 < video_width) {
					v = *(src + x + stride/2);
					R = (v&0xff0000)>>(16+6);
					G = (v&0xff00)>>(8+6);
					B = (v&0xff)>>7;
					x += stride + R + G + B;
					if(x >= video_width) break;
					*(dest + x) = 0xffffff;
				}

				x = video_width / 2 + i;
				while(x - stride/2 >= 0) {
					v = *(src + x - stride/2);
					R = (v&0xff0000)>>(16+6);
					G = (v&0xff00)>>(8+6);
					B = (v&0xff)>>7;
					x -= stride + R + G + B;
					if(x < 0) break;
					*(dest + x) = 0xffffff;
				}
			}
			src += video_width;
			dest += video_width;
		}
	} else {
		for(y=0; y<video_height; y++) {
			for(i=0; i<stride; i++) {
				if(inline_fastrand()&0xc0000000)
					continue;

				x = video_width / 2 + i;
				*(dest + x) = 0xffffff;
	
				while(x + stride/2 < video_width) {
					v = *(src + x + stride/2);
					R = (v&0xff0000)>>(16+6);
					G = (v&0xff00)>>(8+6);
					B = (v&0xff)>>7;
					x += stride - R - G - B;
					if(x >= video_width) break;
					*(dest + x) = 0xffffff;
				}

				x = video_width / 2 + i;
				while(x - stride/2 >= 0) {
					v = *(src + x - stride/2);
					R = (v&0xff0000)>>(16+6);
					G = (v&0xff00)>>(8+6);
					B = (v&0xff)>>7;
					x -= stride - R - G - B;
					if(x < 0) break;
					*(dest + x) = 0xffffff;
				}
			}
			src += video_width;
			dest += video_width;
		}
	}

	target += video_width + (video_width - stride) / 2;
	for(y=0; y<4; y++) {
		for(x=0; x<4; x++) {
			target[x] = 0xff0000;
			target[x+stride] = 0xff0000;
		}
		target += video_width;
	}

	return 0;
}

static int event(SDL_Event *event)
{
	if(event->type == SDL_KEYDOWN) {
		switch(event->key.keysym.sym) {
		case SDLK_SPACE:
			method ^= 1;
			break;
		default:
			break;
		}
	}
	return 0;
}
