/*
 * EffecTV - Realtime Digital Video Effector
 * Copyright (C) 2001 FUKUCHI Kentarou
 *
 * shagadelic.c: Makes you shagadelic! Yeah baby yeah!
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
int shagadelicDrawDouble();

static char *effectname = "ShagadelicTV";
static int stat;
static char *ripple;
static char *spiral;
static unsigned char phase;
static int rx, ry;
static int bx, by;
static int rvx, rvy;
static int bvx, bvy;

effect *shagadelicRegister()
{
	effect *entry;
	
	sharedbuffer_reset();
	ripple = (char *)sharedbuffer_alloc(SCREEN_AREA*4);
	spiral = (char *)sharedbuffer_alloc(SCREEN_AREA);
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
	entry->event = NULL;

	return entry;
}

int shagadelicStart()
{
	int i, x, y;
	double xx, yy;

	if(video_grabstart())
		return -1;
	i = 0;
	for(y=0; y<SCREEN_HEIGHT*2; y++) {
		yy = y-SCREEN_HEIGHT;
		yy *= yy;
		for(x=0; x<SCREEN_WIDTH*2; x++) {
			xx = x-SCREEN_WIDTH;
			xx *= xx;
			ripple[i++] = (unsigned int)(sqrt(xx+yy)*8)&255;
		}
	}
	i = 0;
	for(y=0; y<SCREEN_HEIGHT; y++) {
		yy = y - SCREEN_HHEIGHT;
		for(x=0; x<SCREEN_WIDTH; x++) {
			xx = x - SCREEN_HWIDTH;
			spiral[i++] = (unsigned int)
				((atan2(xx, yy)/M_PI*256*9) + (sqrt(xx*xx+yy*yy)*5))&255;
/* Here is another Swinger!
 * ((atan2(xx, yy)/M_PI*256) + (sqrt(xx*xx+yy*yy)*10))&255;
 */
		}
	}
	rx = fastrand()%SCREEN_WIDTH;
	ry = fastrand()%SCREEN_HEIGHT;
	bx = fastrand()%SCREEN_WIDTH;
	by = fastrand()%SCREEN_HEIGHT;
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
	unsigned int *src, *dest;
	int x, y;
	unsigned int v;
	unsigned char r, g, b;

	if(video_syncframe())
		return -1;
	if(screen_mustlock()) {
		if(screen_lock() < 0) {
			return video_grabframe();
		}
	}
	src = (unsigned int *)video_getaddress();
	dest = (unsigned int *)screen_getaddress();
	if(scale == 2) {
		for(y=0; y<SCREEN_HEIGHT; y++) {
			for(x=0; x<SCREEN_WIDTH; x++) {
				v = *src++ | 0x1010100;
				v = (v - 0x707060) & 0x1010100;
				v -= v>>8;
				r = (char)(ripple[(ry+y)*SCREEN_WIDTH*2+rx+x]+phase*2)>>7;
				g = (char)(spiral[y*SCREEN_WIDTH+x]+phase*3)>>7;
				b = (char)(ripple[(by+y)*SCREEN_WIDTH*2+bx+x]+phase)>>7;
				v &= ((r<<16)|(g<<8)|b);
				dest[0] = v;
				dest[1] = v;
				dest[SCREEN_WIDTH*2] = v;
				dest[SCREEN_WIDTH*2+1] = v;
				dest += 2;
			}
			dest += SCREEN_WIDTH*2;
		}
	} else {
		for(y=0; y<SCREEN_HEIGHT; y++) {
			for(x=0; x<SCREEN_WIDTH; x++) {
				v = *src++ | 0x1010100;
				v = (v - 0x707060) & 0x1010100;
				v -= v>>8;
/* Try another Babe! 
 * v = *src++;
 * *dest++ = v & ((r<<16)|(g<<8)|b);
 */
				r = (char)(ripple[(ry+y)*SCREEN_WIDTH*2+rx+x]+phase*2)>>7;
				g = (char)(spiral[y*SCREEN_WIDTH+x]+phase*3)>>7;
				b = (char)(ripple[(by+y)*SCREEN_WIDTH*2+bx+x]+phase)>>7;
				*dest++ = v & ((r<<16)|(g<<8)|b);
			}
		}
	}
	if(screen_mustlock()) {
		screen_unlock();
	}
	if(video_grabframe())
		return -1;

	phase -= 8;
	if((rx+rvx)<0 || (rx+rvx)>=SCREEN_WIDTH) rvx =-rvx;
	if((ry+rvy)<0 || (ry+rvy)>=SCREEN_HEIGHT) rvy =-rvy;
	if((bx+bvx)<0 || (bx+bvx)>=SCREEN_WIDTH) bvx =-bvx;
	if((by+bvy)<0 || (by+bvy)>=SCREEN_HEIGHT) bvy =-bvy;
	rx += rvx;
	ry += rvy;
	bx += bvx;
	by += bvy;

	return 0;
}
