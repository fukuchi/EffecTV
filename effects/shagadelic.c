/*
 * EffecTV - Realtime Digital Video Effector
 * Copyright (C) 2001-2002 FUKUCHI Kentaro
 *
 * ShagadelicTV - makes you shagadelic! Yeah baby yeah!
 * Copyright (C) 2001-2002 FUKUCHI Kentaro
 *
 * Inspired by Adrian Likin's script for the GIMP.
 */

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "../EffecTV.h"
#include "utils.h"

int shagadelicStart();
int shagadelicStop();
int shagadelicDraw();
int shagadelicEvent();

static char *effectname = "ShagadelicTV";
static int stat;
static char *ripple;
static char *spiral;
static unsigned char phase;
static int rx, ry;
static int bx, by;
static int rvx, rvy;
static int bvx, bvy;
static int mask;

effect *shagadelicRegister()
{
	effect *entry;
	
	sharedbuffer_reset();
	ripple = (char *)sharedbuffer_alloc(video_area*4);
	spiral = (char *)sharedbuffer_alloc(video_area);
	if(ripple == NULL || spiral == NULL) {
		return NULL;
	}

	entry = (effect *)malloc(sizeof(effect));
	if(entry == NULL) {
		return NULL;
	}
	
	entry->name = effectname;
	entry->start = shagadelicStart;
	entry->stop = shagadelicStop;
	entry->draw = shagadelicDraw;
	entry->event = shagadelicEvent;

	return entry;
}

int shagadelicStart()
{
	int i, x, y;
#ifdef PS2
	float xx, yy;
#else
	double xx, yy;
#endif

	mask = 0xffffff;

	if(video_grabstart())
		return -1;
	i = 0;
	for(y=0; y<video_height*2; y++) {
		yy = y - video_height;
		yy *= yy;
		for(x=0; x<video_width*2; x++) {
			xx = x - video_width;
#ifdef PS2
			ripple[i++] = ((unsigned int)(sqrtf(xx*xx+yy)*8))&255;
#else
			ripple[i++] = ((unsigned int)(sqrt(xx*xx+yy)*8))&255;
#endif
		}
	}
	i = 0;
	for(y=0; y<video_height; y++) {
		yy = y - video_height/2;
		for(x=0; x<video_width; x++) {
			xx = x - video_width/2;
#ifdef PS2
			spiral[i++] = ((unsigned int)
				((atan2f(xx, yy)/((float)M_PI)*256*9) + (sqrtf(xx*xx+yy*yy)*5)))&255;
#else
			spiral[i++] = ((unsigned int)
				((atan2(xx, yy)/M_PI*256*9) + (sqrt(xx*xx+yy*yy)*5)))&255;
#endif
/* Here is another Swinger!
 * ((atan2(xx, yy)/M_PI*256) + (sqrt(xx*xx+yy*yy)*10))&255;
 */
		}
	}
	rx = fastrand()%video_width;
	ry = fastrand()%video_height;
	bx = fastrand()%video_width;
	by = fastrand()%video_height;
	rvx = -2;
	rvy = -2;
	bvx = 2;
	bvy = 2;
	phase = 0;

	stat = 1;
	return 0;
}

int shagadelicStop()
{
	if(stat) {
		video_grabstop();
		stat = 0;
	}

	return 0;
}

int shagadelicDraw()
{
	RGB32 *src, *dest;
	int x, y;
	RGB32 v;
	unsigned char r, g, b;
	char *pr, *pg, *pb;

	if(video_syncframe())
		return -1;
	if(screen_mustlock()) {
		if(screen_lock() < 0) {
			return video_grabframe();
		}
	}
	src = (RGB32 *)video_getaddress();
	if(stretch) {
		dest = stretching_buffer;
	} else {
		dest = (RGB32 *)screen_getaddress();
	}

	pr = &ripple[ry*video_width*2 + rx];
	pg = spiral;
	pb = &ripple[by*video_width*2 + bx];

	for(y=0; y<video_height; y++) {
		for(x=0; x<video_width; x++) {
			v = *src++ | 0x1010100;
			v = (v - 0x707060) & 0x1010100;
			v -= v>>8;
/* Try another Babe! 
 * v = *src++;
 * *dest++ = v & ((r<<16)|(g<<8)|b);
 */
			r = (char)(*pr+phase*2)>>7;
			g = (char)(*pg+phase*3)>>7;
			b = (char)(*pb-phase)>>7;
			*dest++ = v & ((r<<16)|(g<<8)|b) & mask;
			pr++;
			pg++;
			pb++;
		}
		pr += video_width;
		pb += video_width;
	}
	if(stretch) {
		image_stretch_to_screen();
	}
	if(screen_mustlock()) {
		screen_unlock();
	}
	if(video_grabframe())
		return -1;

	phase -= 8;
	if((rx+rvx)<0 || (rx+rvx)>=video_width) rvx =-rvx;
	if((ry+rvy)<0 || (ry+rvy)>=video_height) rvy =-rvy;
	if((bx+bvx)<0 || (bx+bvx)>=video_width) bvx =-bvx;
	if((by+bvy)<0 || (by+bvy)>=video_height) bvy =-bvy;
	rx += rvx;
	ry += rvy;
	bx += bvx;
	by += bvy;

	return 0;
}

int shagadelicEvent(SDL_Event *event)
{
	if(event->type == SDL_KEYDOWN) {
		switch(event->key.keysym.sym) {
		case SDLK_1:
			mask &= 0xffff00;
			break;
		case SDLK_2:
			mask &= 0xff00ff;
			break;
		case SDLK_3:
			mask &= 0xffff;
			break;
		default:
			break;
		}
	} else if(event->type == SDL_KEYUP) {
		switch(event->key.keysym.sym) {
		case SDLK_1:
			mask |= 0xff;
			break;
		case SDLK_2:
			mask |= 0xff00;
			break;
		case SDLK_3:
			mask |= 0xff0000;
			break;
		default:
			break;
		}
	}
	return 0;
}
