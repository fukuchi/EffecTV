/*
 * EffecTV - Realtime Digital Video Effector
 * Copyright (C) 2001 FUKUCHI Kentarou
 *
 * RippleTV - Water ripple effect.
 * Copyright (C) 2001 FUKUCHI Kentarou
 *
 */

#include <stdlib.h>
#include <string.h>
#include "../EffecTV.h"
#include "utils.h"

#define MAGIC_THRESHOLD 70

int rippleStart();
int rippleStop();
int rippleDraw();
int rippleEvent(SDL_Event *event);

static int mode = 0; // 0 = motion detection / 1 = rain
static char *effectname = "RippleTV";
static int stat;
static int *buffer;
static int *map;
static int *map1, *map2;
static int map_h, map_w;
static int sqrtable[256];
static const int point = 16;
static const int impact = 1;
static const int decay = 7;

static void setTable()
{
	int i;

	for(i=0; i<128; i++) {
		sqrtable[i] = i*i;
	}
	for(i=1; i<=128; i++) {
		sqrtable[256-i] = -i*i;
	}
}

effect *rippleRegister()
{
	effect *entry;
	
	sharedbuffer_reset();
	map_h = video_height / 2;
	map_w = video_width / 2;
	map = (int *)sharedbuffer_alloc(map_h*map_w*2*sizeof(int));
	buffer = (int *)sharedbuffer_alloc(video_area*sizeof(int));
	if(map == NULL || buffer == NULL) {
		return NULL;
	}

	entry = (effect *)malloc(sizeof(effect));
	if(entry == NULL) {
		return NULL;
	}
	
	entry->name = effectname;
	entry->start = rippleStart;
	entry->stop = rippleStop;
	entry->draw = rippleDraw;
	entry->event = rippleEvent;

	setTable();

	return entry;
}

int rippleStart()
{
	bzero(map, map_h*map_w*2*sizeof(int));
	map1 = map;
	map2 = map + map_h*map_w;
	image_set_threshold_y(MAGIC_THRESHOLD);
	if(video_grabstart())
		return -1;
	stat = 1;
	return 0;
}

int rippleStop()
{
	if(stat) {
		video_grabstop();
		stat = 0;
	}

	return 0;
}

static void motiondetect(RGB32 *src)
{
	unsigned char *diff;
	int width;
	int *p, *q;
	int x, y, h;

	diff = image_bgsubtract_update_y(src);
	width = video_width;
	p = map1+map_w+1;
	q = map2+map_w+1;
	diff += width+2;

	for(y=map_h-2; y>0; y--) {
		for(x=map_w-2; x>0; x--) {
			h = (int)*diff + (int)*(diff+1) + (int)*(diff+width) + (int)*(diff+width+1);
			if(h>0) {
				*p = h<<(point + impact - 8);
				*q = *p;
			}
			p++;
			q++;
			diff += 2;
		}
		diff += width+2;
		p++;
		q++;
	}
}

static void raindrop()
{
}

int rippleDraw()
{
	int x, y, i;
	int dx, dy;
	int h, v;
	int width, height;
	int *p, *q;
	RGB32 *src, *dest;

	if(video_syncframe())
		return -1;

	src = (RGB32 *)video_getaddress();

	if(mode) {
		raindrop();
	} else {
		motiondetect(src);
	}

	width = map_w;
	height = map_h;
	
	for(i=2; i>0; i--) {
		p = map1 + width + 1;
		q = map2 + width + 1;
		for(y=height-2; y>0; y--) {
			for(x=width-2; x>0; x--) {
				h = *(p-width) + *(p-1) + *(p+1) + *(p+width) - (*p)*5;
				h = h / 4;
				v = *p - *q;
				v += h - (v >> decay) + *p;
				*q = v;
				p++;
				q++;
			}
			p += 2;
			q += 2;
		}
		p = map1;
		map1 = map2;
		map2 = p;
	}

	i = 0;
	p = map1;
	for(y=height-1; y>0; y--) {
		for(x=width-1; x>0; x--) {
			h = p[0];
			buffer[i] = h;
			buffer[i+1] = (h+p[1])/2;
			buffer[i+video_width] = (h+p[width])/2;
			buffer[i+video_width+1] = (h+p[1]+p[width]+p[width+1])/4;
			p++;
			i += 2;
		}
		p++;
		i += video_width+2;
	}
	if(screen_mustlock()) {
		if(screen_lock() < 0) {
			return 0;
		}
	}
	if(stretch) {
		dest = stretching_buffer;
	} else {
		dest = (RGB32 *)screen_getaddress();
	}

	i = 0;
	height = video_height;
	width = video_width;
#ifdef DEBUG
	for(y=0; y<height-1; y++) {
		for(x=0; x<width-1; x++) {
			dx = x + sqrtable[((buffer[i] - buffer[i+1] + (1<<(point-3)))>>(point-2))&0xff];
			dy = y + sqrtable[((buffer[i] - buffer[i+width] + (1<<(point-3)))>>(point-2))&0xff];
			if(dx<0) dx=0;
			if(dy<0) dy=0;
			if(dx>width) dx=width;
			if(dy>height) dy=height;
			*dest++ = src[dy*width+dx];
			i++;
		}
		i++;
		dest++;
	}
#else
	for(y=0; y<height-1; y++) {
		for(x=0; x<width-1; x++) {
			dx = x + sqrtable[((buffer[i] - buffer[i+1] + (1<<(point-3)))>>(point-2))&0xff];
			dy = y + sqrtable[((buffer[i] - buffer[i+width] + (1<<(point-3)))>>(point-2))&0xff];
			if(dx<0) dx=0;
			if(dy<0) dy=0;
			if(dx>width) dx=width;
			if(dy>height) dy=height;
			*dest++ = src[dy*width+dx];
			i++;
		}
		i++;
		dest++;
	}
#endif
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

int rippleEvent(SDL_Event *event)
{
	if(event->type == SDL_KEYDOWN) {
		switch(event->key.keysym.sym) {
		case SDLK_SPACE:
			bzero(map, map_h*map_w*2*sizeof(int));
			break;
		case SDLK_r:
			mode = ~mode;
			break;
		default:
			break;
		}
	}
	return 0;
}
