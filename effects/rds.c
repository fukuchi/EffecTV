/*
 * EffecTV - Realtime Digital Video Effector
 * Copyright (C) 2002 FUKUCHI Kentarou
 *
 * RandomDotStereoTV - makes random dot stereogram.
 * Copyright (C) 2002 FUKUCHI Kentarou
 *
 */

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "../EffecTV.h"
#include "utils.h"

int rdsStart();
int rdsStop();
int rdsDraw();
int rdsEvent();

static char *effectname = "RandomDotStereoTV";
static int stat;
static int stride = 40;
static int method = 0;

effect *rdsRegister()
{
	effect *entry;
	
	entry = (effect *)malloc(sizeof(effect));
	if(entry == NULL) {
		return NULL;
	}
	
	entry->name = effectname;
	entry->start = rdsStart;
	entry->stop = rdsStop;
	entry->draw = rdsDraw;
	entry->event = rdsEvent;

	return entry;
}

int rdsStart()
{
	screen_clear(0);
	if(video_grabstart())
		return -1;

	stat = 1;
	return 0;
}

int rdsStop()
{
	if(stat) {
		video_grabstop();
		stat = 0;
	}

	return 0;
}

int rdsDraw()
{
	int x, y, i;
	RGB32 *target;
	RGB32 *src, *dest;
	RGB32 v;
	RGB32 R, G, B;

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

int rdsEvent(SDL_Event *event)
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
