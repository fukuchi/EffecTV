/*
 * EffecTV - Realtime Digital Video Effector
 * Copyright (C) 2001 FUKUCHI Kentarou
 *
 * DotTV: convert gray scale image into a set of dots
 * Copyright (C) 2001 FUKUCHI Kentarou
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
static RGB32 *pattern;
static int dots_width;
static int dots_height;
static int dot_size;
static int dot_hsize;
static int *sampx, *sampy;

inline static unsigned char inline_RGBtoY(int rgb)
{
	int i;

	i = RtoY[(rgb>>16)&0xff];
	i += GtoY[(rgb>>8)&0xff];
	i += BtoY[rgb&0xff];
	return i;
}

static void init_sampxy_table()
{
	int i, j;

	j = dot_hsize;
	for(i=0; i<dots_width; i++) {
		sampx[i] = j * video_width / screen_width;
		j += dot_size;
	}
	j = dot_hsize;
	for(i=0; i<dots_height; i++) {
		sampy[i] = j * video_height / screen_height;
		j += dot_size;
	}
}

static void makePattern()
{
	int i, x, y, c;
	int u, v;
	double p, q, r;
	RGB32 *pat;

	for(i=0; i<DOTMAX; i++) {
/* Generated pattern is a quadrant of a disk. */
		pat = pattern + (i+1) * dot_hsize * dot_hsize - 1;
		r = (0.2 * i / DOTMAX + 0.8) * dot_hsize;
		r = r*r;
		for(y=0; y<dot_hsize; y++) {
			for(x=0; x<dot_hsize; x++) {
				c = 0;
				for(u=0; u<4; u++) {
					p = (double)u/4.0 + y;
					p = p*p;
					for(v=0; v<4; v++) {
						q = (double)v/4.0 + x;
						if(p+q*q<r) {
							c++;
						}
					}
				}
				c = (c>15)?15:c;
				*pat-- = c<<20 | c<<12 | c<<4;
/* The upper left part of a disk is needed, but generated pattern is a bottom
 * right part. So I spin the pattern. */
			}
		}
	}
}

effect *dotRegister()
{
	effect *entry;
	double scale;
	
	if(screen_scale > 0) {
		scale = screen_scale;
	} else {
		scale = (double)screen_width / video_width;
		if(scale > (double)screen_height / video_height) {
			scale = (double)screen_height / video_height;
		}
	}
	dot_size = 8 * scale;
	dot_size = dot_size & 0xfe;
	dot_hsize = dot_size / 2;
	dots_width = screen_width / dot_size;
	dots_height = screen_height / dot_size;
	
	pattern = (RGB32 *)malloc(DOTMAX * dot_hsize * dot_hsize * sizeof(RGB32));
	if(pattern == NULL) {
		return NULL;
	}

	sharedbuffer_reset();
	sampx = (int *)sharedbuffer_alloc(video_width*sizeof(int));
	sampy = (int *)sharedbuffer_alloc(video_height*sizeof(int));
	if(sampx == NULL || sampy == NULL) {
		return NULL;
	}

	entry = (effect *)malloc(sizeof(effect));
	if(entry == NULL) {
		return NULL;
	}
	
	entry->name = effectname;
	entry->start = dotStart;
	entry->stop = dotStop;
	entry->draw = dotDraw;
	entry->event = NULL;

	makePattern();

	return entry;
}

int dotStart()
{
	screen_clear(0);
	init_sampxy_table();
	if(stretch) {
		image_stretching_buffer_clear(0);
	}
	if(video_grabstart())
		return -1;

	state = 1;
	return 0;
}

int dotStop()
{
	if(state) {
		video_grabstop();
		state = 0;
	}

	return 0;
}

static void drawDot(int xx, int yy, unsigned char c, RGB32 *dest)
{
	int x, y;
	RGB32 *pat;

	c = (c>>(8-DOTDEPTH));
	pat = pattern + c * dot_hsize * dot_hsize;
	dest = dest + yy * dot_size * screen_width + xx * dot_size;
	for(y=0; y<dot_hsize; y++) {
		for(x=0; x<dot_hsize; x++) {
			*dest++ = *pat++;
		}
		pat -= 2;
		for(x=0; x<dot_hsize-1; x++) {
			*dest++ = *pat--;
		}
		dest += screen_width - dot_size + 1;
		pat += dot_hsize + 1;
	}
	pat -= dot_hsize*2;
	for(y=0; y<dot_hsize-1; y++) {
		for(x=0; x<dot_hsize; x++) {
			*dest++ = *pat++;
		}
		pat -= 2;
		for(x=0; x<dot_hsize-1; x++) {
			*dest++ = *pat--;
		}
		dest += screen_width - dot_size + 1;
		pat += -dot_hsize + 1;
	}
}

int dotDraw()
{
	int x, y;
	int sx, sy;
	RGB32 *src, *dest;

	if(video_syncframe())
		return -1;
	if(screen_mustlock()) {
		if(screen_lock() < 0) {
			return video_grabframe();
		}
	}
	src = (RGB32 *)video_getaddress();
	dest = (RGB32 *)screen_getaddress();

	for(y=0; y<dots_height; y++) {
		sy = sampy[y];
		for(x=0; x<dots_width; x++) {
			sx = sampx[x];
			drawDot(x, y, inline_RGBtoY(src[sy*video_width+sx]), dest);
		}
	}
	if(screen_mustlock()) {
		screen_unlock();
	}
	if(video_grabframe())
		return -1;

	return 0;
}
