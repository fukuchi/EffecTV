/*
 * EffecTV - Realtime Digital Video Effector
 * Copyright (C) 2001 FUKUCHI Kentarou
 *
 * dot.c: convert gray scale image into 80x60 dots image
 *
 */

#include <stdlib.h>
#include <string.h>
#include "../EffecTV.h"
#include "utils.h"

int dotStart();
int dotStop();
int dotDraw();

#define DOTDEPTH 5
#define DOTMAX (1<<DOTDEPTH)

static char *effectname = "DotTV";
static int state;
static int palette[16];
static unsigned int pattern[DOTMAX][4];
#ifdef ENABLE_PALETTECHANGE
static int format;
#endif

#ifndef ENABLE_PALETTECHANGE
inline static unsigned char inline_RGBtoY(int rgb)
{
	int i;

	i = RtoY[(rgb>>16)&0xff];
	i += GtoY[(rgb>>8)&0xff];
	i += BtoY[rgb&0xff];
	return i;
}
#endif

static void makePalette()
{
	int i;

	for(i=0; i<16; i++) {
		palette[i] = i<<20 | i<<12 | i<<4;
	}
}

static void makePattern()
{
	int i, x, y, c;
	int u, v;
	double p, q, r;
	unsigned int d[4];

	for(i=0; i<DOTMAX; i++) {
		r = 1.0*i/DOTMAX+3.0;
		r = r*r;
		for(y=0; y<4; y++) {
			for(x=0; x<4; x++) {
				c = 0;
				for(u=0; u<4; u++) {
					p = (double)u/4.0 + y;
					for(v=0; v<4; v++) {
						q = (double)v/4.0 + x;
						if(p*p+q*q<r) {
							c++;
						}
					}
				}
				c = (c>15)?15:c;
				d[x] = c;
			}
			pattern[i][3-y]= d[3]<<24 | d[2]<<20 | d[1]<<16 | d[0]<<12
			               | d[1]<<8 | d[2]<<4 | d[3];
		}
	}
}

effect *dotRegister()
{
	effect *entry;
	
	yuvTableInit();

	entry = (effect *)malloc(sizeof(effect));
	if(entry == NULL) {
		return NULL;
	}
	
	entry->name = effectname;
	entry->start = dotStart;
	entry->stop = dotStop;
	entry->draw = dotDraw;
	entry->event = NULL;

	makePalette();
	makePattern();

	return entry;
}

int dotStart()
{
	screen_clear(0);
#ifdef ENABLE_PALETTECHANGE
	format = video_getformat();
	if(video_setformat(VIDEO_PALETTE_GREY))
		return -1;
	if(video_changesize(80, 60))
		return -1;
#endif
	if(video_grabstart())
		return -1;

	state = 1;
	return 0;
}

int dotStop()
{
	if(state) {
		video_grabstop();
#ifdef ENABLE_PALETTECHANGE
		video_setformat(format);
		video_changesize(0, 0);
#endif
		state = 0;
	}

	return 0;
}

static void drawDot(int x, int y, unsigned char c, unsigned int *dest)
{
	int v, w;
	int i, j;

	c = (c>>(8-DOTDEPTH));
	dest = dest + y*8*SCREEN_WIDTH+x*8;
	for(i=0; i<4; i++) {
		v = pattern[c][i];
		for(j=0; j<8; j++) {
			w = v & 0xf;
			v = v>>4;
			dest[j] = palette[w];
		}
		dest += SCREEN_WIDTH;
	}
	for(i=2; i>=0; i--) {
		v = pattern[c][i];
		for(j=0; j<8; j++) {
			w = v & 0xf;
			v = v>>4;
			dest[j] = palette[w];
		}
		dest += SCREEN_WIDTH;
	}
}

static void drawDotDouble(int x, int y, unsigned char c, unsigned int *dest)
{
	int v, w;
	int i, j;

	c = (c>>(8-DOTDEPTH));
	dest = dest + y*8*SCREEN_WIDTH*2+x*8;
	for(i=0; i<4; i++) {
		v = pattern[c][i];
		for(j=0; j<8; j++) {
			w = v & 0xf;
			v = v>>4;
			dest[j] = palette[w];
		}
		dest += SCREEN_WIDTH*2;
	}
	for(i=2; i>=0; i--) {
		v = pattern[c][i];
		for(j=0; j<8; j++) {
			w = v & 0xf;
			v = v>>4;
			dest[j] = palette[w];
		}
		dest += SCREEN_WIDTH*2;
	}
}

int dotDraw()
{
	int x, y;
#ifdef ENABLE_PALETTECHANGE
	unsigned char *src;
#else
	unsigned int *src;
#endif
	unsigned int *dest;

	if(video_syncframe())
		return -1;
	if(screen_mustlock()) {
		if(screen_lock() < 0) {
			return video_grabframe();
		}
	}
#ifdef ENABLE_PALETTECHANGE
	src = video_getaddress();
#else
	src = (unsigned int *)video_getaddress();
#endif
	dest = (unsigned int *)screen_getaddress();

#ifdef ENABLE_PALETTECHANGE
	if(scale == 2) {
		for(y=0; y<60; y++) {
			for(x=0; x<80; x++) {
				drawDotDouble(x, y, src[y*80+x], dest);
			}
		}
	} else {
		for(y=0; y<30; y++) {
			for(x=0; x<40; x++) {
				drawDot(x, y, src[y*2*80+x*2], dest);
			}
		}
	}
#else
	if(scale == 2) {
		for(y=0; y<60; y++) {
			for(x=0; x<80; x++) {
				drawDotDouble(x, y, inline_RGBtoY(src[y*4*SCREEN_WIDTH+x*4]), dest);
			}
		}
	} else {
		for(y=0; y<30; y++) {
			for(x=0; x<40; x++) {
				drawDot(x, y, inline_RGBtoY(src[y*8*SCREEN_WIDTH+x*8]), dest);
			}
		}
	}
#endif
	if(screen_mustlock()) {
		screen_unlock();
	}
	if(video_grabframe())
		return -1;

	return 0;
}
