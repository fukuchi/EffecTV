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
static signed char *vtable;
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
	map_h = video_height / 2 + 1;
	map_w = video_width / 2 + 1;
	map = (int *)sharedbuffer_alloc(map_h*map_w*2*sizeof(int));
	vtable = (signed char *)sharedbuffer_alloc(map_h*map_w*2*sizeof(signed char));
	if(map == NULL || vtable == NULL) {
		return NULL;
	}

	bzero(vtable, map_h*map_w*2*sizeof(signed char));

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
		p+=2;
		q+=2;
	}
}

static void raindrop()
{
	static int period = 0;
	static int rain_stat = 0;
	static unsigned int drop_prob = 0;
	static int drop_prob_increment = 0;
	static int drop_power = 0;

	int x, y;
	int *p;

	if(period == 0) {
		switch(rain_stat) {
		case 0:
			period = (fastrand()>>20)+100;
			drop_prob = 0;
			drop_prob_increment = ((fastrand()>>18)+0x7fff)/period;
			drop_power = (-(fastrand()>>28)-2)<<point;
			rain_stat = 1;
			break;
		case 1:
			period = (fastrand()>>22)+1000;
			drop_prob_increment = 0;
			rain_stat = 2;
			break;
		case 2:
			period = (fastrand()>>24)+60;
			drop_prob_increment = -(drop_prob/period);
			rain_stat = 3;
			break;
		case 3:
		default:
			period = (fastrand()>>23)+500;
			drop_prob = 0;
			rain_stat = 0;
		}
	}
	if(rain_stat>0) {
		if((fastrand()>>16)<drop_prob) {
			x = fastrand()%(map_w-4)+2;
			y = fastrand()%(map_h-4)+2;
			p = map1 + y*map_w + x;
			*p = drop_power;
			*(p-map_w) = *(p-1) = *(p+1) = *(p+map_w) = drop_power/2;
		}
		drop_prob += drop_prob_increment;
	}
	period--;
}

int rippleDraw()
{
	int x, y, i;
	int dx, dy;
	int h, v;
	int width, height;
	int *p, *q;
	RGB32 *src, *dest;
	signed char *vp;

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
	vp = vtable;
	p = map1;
	for(y=height-1; y>0; y--) {
		for(x=width-1; x>0; x--) {
			vp[0] = sqrtable[((p[0] - p[1] + (1<<(point-3)))>>(point-2))&0xff]; 
			vp[1] = sqrtable[((p[0] - p[width] + (1<<(point-3)))>>(point-2))&0xff]; 
			p++;
			vp+=2;
		}
		p++;
		vp+=2;
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

	height = video_height;
	width = video_width;
	vp = vtable;
#ifndef DEBUG
	for(y=0; y<height; y+=2) {
		for(x=0; x<width; x+=2) {
			h = (int)vp[0];
			v = (int)vp[1];
			dx = x + h;
			dy = y + v;
			if(dx<0) dx=0;
			if(dy<0) dy=0;
			if(dx>width) dx=width;
			if(dy>height) dy=height;
			dest[0] = src[dy*width+dx];

			i = dx;

			dx = x + (h+(int)vp[2])/2;
			if(dx<0) dx=0;
			if(dx>width) dx=width;
			dest[1] = src[dy*width+dx];

			dy = y + (v+(int)vp[map_w*2+1])/2;
			if(dy<0) dy=0;
			dest[width] = src[dy*width+i];

			dest[width+1] = src[dy*width+dx];
			dest+=2;
			vp+=2;
		}
		dest += video_width;
		vp += 2;
	}
#else
	for(y=0; y<height; y+=2) {
		for(x=0; x<width; x+=2) {
			h = (int)vp[0] + (int)vp[1] + 128;
			if(h<0) h=0;
			if(h>255) h=255;
			dest[0] = h<<8;

			h = (int)vp[2] + (int)vp[1] + 128;
			if(h<0) h=0;
			if(h>255) h=255;
			dest[1] = h<<8;

			h = (int)vp[0] + (int)vp[map_w*2+1] + 128;
			if(h<0) h=0;
			if(h>255) h=255;
			dest[width] = h<<8;

			h = (int)vp[2] + (int)vp[map_w*2+1] + 128;
			if(h<0) h=0;
			if(h>255) h=255;
			dest[width+1] = h<<8;

			dest+=2;
			vp+=2;
		}
		dest += video_width;
		vp += 2;
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
