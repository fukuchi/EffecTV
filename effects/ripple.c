/*
 * EffecTV - Realtime Digital Video Effector
 * Copyright (C) 2001-2006 FUKUCHI Kentaro
 *
 * RippleTV - Water ripple effect.
 * Copyright (C) 2001-2002 FUKUCHI Kentaro
 *
 */

//#define DEBUG

#include <stdlib.h>
#include <string.h>
#include "EffecTV.h"
#include "utils.h"

#define MAGIC_THRESHOLD 70

static int start(void);
static int stop(void);
static int draw(RGB32 *src, RGB32 *dest);
static int event(SDL_Event *event);

static int mode = 0; // 0 = motion detection / 1 = rain
static char *effectname = "RippleTV";
static int stat;
static signed char *vtable;
static int *map;
static int *map1, *map2, *map3;
static int map_h, map_w;
static int sqrtable[256];
static const int point = 16;
static const int impact = 2;
static const int decay = 8;
static const int loopnum = 2;
static int bgIsSet = 0;

static void setTable(void)
{
	int i;

	for(i=0; i<128; i++) {
		sqrtable[i] = i*i;
	}
	for(i=1; i<=128; i++) {
		sqrtable[256-i] = -i*i;
	}
}

static int setBackground(RGB32 *src)
{
	image_bgset_y(src);
	bgIsSet = 1;

	return 0;
}

effect *rippleRegister(void)
{
	effect *entry;
	
	sharedbuffer_reset();
	map_h = video_height / 2 + 1;
	map_w = video_width / 2 + 1;
	map = (int *)sharedbuffer_alloc(map_h*map_w*3*sizeof(int));
	vtable = (signed char *)sharedbuffer_alloc(map_h*map_w*2*sizeof(signed char));
	if(map == NULL || vtable == NULL) {
		return NULL;
	}

	map3 = map + map_w * map_h * 2;

	entry = (effect *)malloc(sizeof(effect));
	if(entry == NULL) {
		return NULL;
	}
	
	entry->name = effectname;
	entry->start = start;
	entry->stop = stop;
	entry->draw = draw;
	entry->event = event;

	setTable();

	return entry;
}

static int start(void)
{
	memset(map, 0, map_h*map_w*3*sizeof(int));
	memset(vtable, 0, map_h*map_w*2*sizeof(signed char));
	map1 = map;
	map2 = map + map_h*map_w;
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

static void motiondetect(RGB32 *src)
{
	unsigned char *diff;
	int width;
	int *p, *q;
	int x, y, h;

	if(!bgIsSet) {
		setBackground(src);
	}

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

static inline void drop(int power)
{
	int x, y;
	int *p, *q;

	x = fastrand()%(map_w-4)+2;
	y = fastrand()%(map_h-4)+2;
	p = map1 + y*map_w + x;
	q = map2 + y*map_w + x;
	*p = power;
	*q = power;
	*(p-map_w) = *(p-1) = *(p+1) = *(p+map_w) = power/2;
	*(p-map_w-1) = *(p-map_w+1) = *(p+map_w-1) = *(p+map_w+1) = power/4;
	*(q-map_w) = *(q-1) = *(q+1) = *(q+map_w) = power/2;
	*(q-map_w-1) = *(q-map_w+1) = *(q+map_w-1) = *(p+map_w+1) = power/4;
}

static void raindrop(void)
{
	static int period = 0;
	static int rain_stat = 0;
	static unsigned int drop_prob = 0;
	static int drop_prob_increment = 0;
	static int drops_per_frame_max = 0;
	static int drops_per_frame = 0;
	static int drop_power = 0;

	int i;

	if(period == 0) {
		switch(rain_stat) {
		case 0:
			period = (fastrand()>>23)+100;
			drop_prob = 0;
			drop_prob_increment = 0x00ffffff/period;
			drop_power = (-(fastrand()>>28)-2)<<point;
			drops_per_frame_max = 2<<(fastrand()>>30); // 2,4,8 or 16
			rain_stat = 1;
			break;
		case 1:
			drop_prob = 0x00ffffff;
			drops_per_frame = 1;
			drop_prob_increment = 1;
			period = (drops_per_frame_max - 1) * 16;
			rain_stat = 2;
			break;
		case 2:
			period = (fastrand()>>22)+1000;
			drop_prob_increment = 0;
			rain_stat = 3;
			break;
		case 3:
			period = (drops_per_frame_max - 1) * 16;
			drop_prob_increment = -1;
			rain_stat = 4;
			break;
		case 4:
			period = (fastrand()>>24)+60;
			drop_prob_increment = -(drop_prob/period);
			rain_stat = 5;
			break;
		case 5:
		default:
			period = (fastrand()>>23)+500;
			drop_prob = 0;
			rain_stat = 0;
			break;
		}
	}
	switch(rain_stat) {
	default:
	case 0:
		break;
	case 1:
	case 5:
		if((fastrand()>>8)<drop_prob) {
			drop(drop_power);
		}
		drop_prob += drop_prob_increment;
		break;
	case 2:
	case 3:
	case 4:
		for(i=drops_per_frame/16; i>0; i--) {
			drop(drop_power);
		}
		drops_per_frame += drop_prob_increment;
		break;
	}
	period--;
}

static int draw(RGB32 *src, RGB32 *dest)
{
	int x, y, i;
	int dx, dy;
	int h, v;
	int width, height;
	int *p, *q, *r;
	signed char *vp;
#ifdef DEBUG
	RGB32 *dest2;
#endif

	/* impact from the motion or rain drop */
	if(mode) {
		raindrop();
	} else {
		motiondetect(src);
	}

	/* simulate surface wave */
	width = map_w;
	height = map_h;
	
	/* This function is called only 30 times per second. To increase a speed
	 * of wave, iterates this loop several times. */
	for(i=loopnum; i>0; i--) {
		/* wave simulation */
		p = map1 + width + 1;
		q = map2 + width + 1;
		r = map3 + width + 1;
		for(y=height-2; y>0; y--) {
			for(x=width-2; x>0; x--) {
				h = *(p-width-1) + *(p-width+1) + *(p+width-1) + *(p+width+1)
				  + *(p-width) + *(p-1) + *(p+1) + *(p+width) - (*p)*9;
				h = h >> 3;
				v = *p - *q;
				v += h - (v >> decay);
				*r = v + *p;
				p++;
				q++;
				r++;
			}
			p += 2;
			q += 2;
			r += 2;
		}

		/* low pass filter */
		p = map3 + width + 1;
		q = map2 + width + 1;
		for(y=height-2; y>0; y--) {
			for(x=width-2; x>0; x--) {
				h = *(p-width) + *(p-1) + *(p+1) + *(p+width) + (*p)*60;
				*q = h >> 6;
				p++;
				q++;
			}
			p+=2;
			q+=2;
		}

		p = map1;
		map1 = map2;
		map2 = p;
	}

	vp = vtable;
	p = map1;
	for(y=height-1; y>0; y--) {
		for(x=width-1; x>0; x--) {
			/* difference of the height between two voxel. They are twiced to
			 * emphasise the wave. */
			vp[0] = sqrtable[((p[0] - p[1])>>(point-1))&0xff]; 
			vp[1] = sqrtable[((p[0] - p[width])>>(point-1))&0xff]; 
			p++;
			vp+=2;
		}
		p++;
		vp+=2;
	}

	height = video_height;
	width = video_width;
	vp = vtable;

#ifndef DEBUG
	/* draw refracted image. The vector table is stretched. */
	for(y=0; y<height; y+=2) {
		for(x=0; x<width; x+=2) {
			h = (int)vp[0];
			v = (int)vp[1];
			dx = x + h;
			dy = y + v;
			if(dx<0) dx=0;
			if(dy<0) dy=0;
			if(dx>=width) dx=width-1;
			if(dy>=height) dy=height-1;
			dest[0] = src[dy*width+dx];

			i = dx;

			dx = x + 1 + (h+(int)vp[2])/2;
			if(dx<0) dx=0;
			if(dx>=width) dx=width-1;
			dest[1] = src[dy*width+dx];

			dy = y + 1 + (v+(int)vp[map_w*2+1])/2;
			if(dy<0) dy=0;
			if(dy>=height) dy=height-1;
			dest[width] = src[dy*width+i];

			dest[width+1] = src[dy*width+dx];
			dest+=2;
			vp+=2;
		}
		dest += video_width;
		vp += 2;
	}
#else
	dest2 = dest;
	p = map1;
	for(y=0; y<height; y+=2) {
		for(x=0; x<width; x+=2) {
			h = ((p[0]>>(point-5))+128)<<8;
//			h = ((int)vp[1]+128)<<8;
			dest[0] = h;
			dest[1] = h;
			dest[width] = h;
			dest[width+1] = h;
			p++;
			dest+=2;
			vp+=2;
		}
		dest += video_width;
		vp += 2;
		p++;
	}
	h = map_h/2;
	dest2 += video_height/2*video_width;
	p = map1 + h * map_w;
	for(x=0; x<map_w-1; x++) {
		y = p[x]>>(point-2);
		if(y>=h) y = h - 1;
		if(y<=-h) y = -h + 1;
		dest2[y*video_width+x*2] = 0xffffff;
		dest2[y*video_width+x*2+1] = 0xffffff;
	}
#endif

	return 0;
}

static int event(SDL_Event *event)
{
	if(event->type == SDL_KEYDOWN) {
		switch(event->key.keysym.sym) {
		case SDLK_SPACE:
			memset(map, 0, map_h*map_w * 2 * sizeof(int));
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
