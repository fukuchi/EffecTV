/*
 * EffecTV - Realtime Digital Video Effector
 * Copyright (C) 2001 FUKUCHI Kentarou
 *
 * edge.c: detects edge and display it in good old computer way.
 *
 * The idea of EdgeTV is taken from Adrian Likins's effector script for GIMP,
 * `Predator effect.'
 * The algorithm of the original script pixelizes the image at first, then
 * it adopts the edge detection filter to the image.
 * This code is highly optimized and employs many fake algorithms. For example,
 * it devides a value with 16 instead of using sqrt() in line 154. It is too
 * hard for me to write detailed comment in this code.
 */

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "../EffecTV.h"
#include "utils.h"


int edgeStart();
int edgeStop();
int edgeDraw();
int edgeDrawDouble();

static char *effectname = "EdgeTV";
static int stat;
static unsigned int *map;

#define MAP_WIDTH (SCREEN_WIDTH/4)
#define MAP_HEIGHT (SCREEN_HEIGHT/4)

effect *edgeRegister()
{
	effect *entry;
	
	sharedbuffer_reset();
	map = (unsigned int *)sharedbuffer_alloc(MAP_WIDTH*MAP_HEIGHT*sizeof(unsigned int)*2);
	if(map == NULL) {
		return NULL;
	}

	entry = (effect *)malloc(sizeof(effect));
	if(entry == NULL) {
		return NULL;
	}
	
	entry->name = effectname;
	entry->start = edgeStart;
	entry->stop = edgeStop;
	if(scale == 2) {
		entry->draw = edgeDrawDouble;
	} else {
		entry->draw = edgeDraw;
	}
	entry->event = NULL;

	return entry;
}

int edgeStart()
{
	screen_clear(0);
	bzero(map, MAP_WIDTH*MAP_HEIGHT*sizeof(unsigned int)*2);
	if(video_grabstart())
		return -1;

	stat = 1;
	return 0;
}

int edgeStop()
{
	if(stat) {
		video_grabstop();
		stat = 0;
	}

	return 0;
}

int edgeDraw()
{
	int  x, y;
	unsigned int *src, *dest;
	int r, g, b;
	unsigned int p, q;
	unsigned int v0, v1, v2, v3;

	if(video_syncframe())
		return -1;
	if(screen_mustlock()) {
		if(screen_lock() < 0) {
			return 0;
		}
	}
	src = (unsigned int *)video_getaddress();
	dest = (unsigned int *)screen_getaddress();

	src += SCREEN_WIDTH*4+4;
	dest += SCREEN_WIDTH*4+4;
	for(y=1; y<MAP_HEIGHT-1; y++) {
		for(x=1; x<MAP_WIDTH-1; x++) {
			p = *src;
			q = *(src - 4);

/* difference between the current pixel and right neighbor. */
			r = ((p&0xff0000) - (q&0xff0000))>>16;
			g = ((p&0xff00) - (q&0xff00))>>8;
			b = (p&0xff) - (q&0xff);
			r *= r;
			g *= g;
			b *= b;
			r = r>>5; /* To lack the lower bit for saturated addition,  */
			g = g>>5; /* devide the value with 32, instead of 16. It is */
			b = b>>4; /* same as `v2 &= 0xfefeff' */
			if(r>127) r = 127;
			if(g>127) g = 127;
			if(b>255) b = 255;
			v2 = (r<<17)|(g<<9)|b;

/* difference between the current pixel and upper neighbor. */
			q = *(src - SCREEN_WIDTH*4);
			r = ((p&0xff0000) - (q&0xff0000))>>16;
			g = ((p&0xff00) - (q&0xff00))>>8;
			b = (p&0xff) - (q&0xff);
			r *= r;
			g *= g;
			b *= b;
			r = r>>5;
			g = g>>5;
			b = b>>4;
			if(r>127) r = 127;
			if(g>127) g = 127;
			if(b>255) b = 255;
			v3 = (r<<17)|(g<<9)|b;

			v0 = map[(y-1)*MAP_WIDTH*2+x*2];
			v1 = map[y*MAP_WIDTH*2+(x-1)*2+1];
			map[y*MAP_WIDTH*2+x*2] = v2;
			map[y*MAP_WIDTH*2+x*2+1] = v3;
			r = v0 + v1;
			g = r & 0x01010100;
			dest[0] = r | (g - (g>>8));
			r = v0 + v3;
			g = r & 0x01010100;
			dest[1] = r | (g - (g>>8));
			dest[2] = v3;
			dest[3] = v3;
			r = v2 + v1;
			g = r & 0x01010100;
			dest[SCREEN_WIDTH] = r | (g - (g>>8));
			r = v2 + v3;
			g = r & 0x01010100;
			dest[SCREEN_WIDTH+1] = r | (g - (g>>8));
			dest[SCREEN_WIDTH+2] = v3;
			dest[SCREEN_WIDTH+3] = v3;
			dest[SCREEN_WIDTH*2] = v2;
			dest[SCREEN_WIDTH*2+1] = v2;
			dest[SCREEN_WIDTH*3] = v2;
			dest[SCREEN_WIDTH*3+1] = v2;

			src += 4;
			dest += 4;
		}
		src += SCREEN_WIDTH*3+8;
		dest += SCREEN_WIDTH*3+8;
	}
	if(screen_mustlock()) {
		screen_unlock();
	}
	if(video_grabframe())
		return -1;

	return 0;
}

int edgeDrawDouble()
{
	int  x, y;
	unsigned int *src, *dest;
	int r, g, b;
	unsigned int p, q;
	unsigned int v0, v1, v2, v3;

	if(video_syncframe())
		return -1;
	if(screen_mustlock()) {
		if(screen_lock() < 0) {
			return 0;
		}
	}
	src = (unsigned int *)video_getaddress();
	dest = (unsigned int *)screen_getaddress();

	src += SCREEN_WIDTH*4+4;
	dest += SCREEN_WIDTH*2*8+8;
	for(y=1; y<MAP_HEIGHT-1; y++) {
		for(x=1; x<MAP_WIDTH-1; x++) {
			p = *src;
			q = *(src - 4);

/* difference between the current pixel and right neighbor. */
			r = ((p&0xff0000) - (q&0xff0000))>>16;
			g = ((p&0xff00) - (q&0xff00))>>8;
			b = (p&0xff) - (q&0xff);
			r *= r;
			g *= g;
			b *= b;
			r = r>>5; /* To lack the lower bit for saturated addition,  */
			g = g>>5; /* devide the value with 32, instead of 16. It is */
			b = b>>4; /* same as `v2 &= 0xfefeff' */
			if(r>127) r = 127;
			if(g>127) g = 127;
			if(b>255) b = 255;
			v2 = (r<<17)|(g<<9)|b;

/* difference between the current pixel and upper neighbor. */
			q = *(src - SCREEN_WIDTH*4);
			r = ((p&0xff0000) - (q&0xff0000))>>16;
			g = ((p&0xff00) - (q&0xff00))>>8;
			b = (p&0xff) - (q&0xff);
			r *= r;
			g *= g;
			b *= b;
			r = r>>5;
			g = g>>5;
			b = b>>4;
			if(r>127) r = 127;
			if(g>127) g = 127;
			if(b>255) b = 255;
			v3 = (r<<17)|(g<<9)|b;

			v0 = map[(y-1)*MAP_WIDTH*2+x*2];
			v1 = map[y*MAP_WIDTH*2+(x-1)*2+1];
			map[y*MAP_WIDTH*2+x*2] = v2;
			map[y*MAP_WIDTH*2+x*2+1] = v3;
			r = v0 + v1;
			g = r & 0x01010100;
			r |= g - (g>>8);
			dest[0] = r;
			dest[1] = r;
			dest[SCREEN_WIDTH*2] = r;
			dest[SCREEN_WIDTH*2+1] = r;
			r = v0 + v3;
			g = r & 0x01010100;
			r |= g - (g>>8);
			dest[2] = r;
			dest[3] = r;
			dest[SCREEN_WIDTH*2+2] = r;
			dest[SCREEN_WIDTH*2+3] = r;

			dest[4] = v3;
			dest[5] = v3;
			dest[6] = v3;
			dest[7] = v3;
			dest[SCREEN_WIDTH*2+4] = v3;
			dest[SCREEN_WIDTH*2+5] = v3;
			dest[SCREEN_WIDTH*2+6] = v3;
			dest[SCREEN_WIDTH*2+7] = v3;

			r = v2 + v1;
			g = r & 0x01010100;
			r |=  g - (g>>8);
			dest[SCREEN_WIDTH*2*2] = r;
			dest[SCREEN_WIDTH*2*2+1] = r;
			dest[SCREEN_WIDTH*2*3] = r;
			dest[SCREEN_WIDTH*2*3+1] = r;

			r = v2 + v3;
			g = r & 0x01010100;
			r |= g - (g>>8);
			dest[SCREEN_WIDTH*2*2+2] = r;
			dest[SCREEN_WIDTH*2*2+3] = r;
			dest[SCREEN_WIDTH*2*3+2] = r;
			dest[SCREEN_WIDTH*2*3+3] = r;

			dest[SCREEN_WIDTH*2*2+4] = v3;
			dest[SCREEN_WIDTH*2*2+5] = v3;
			dest[SCREEN_WIDTH*2*2+6] = v3;
			dest[SCREEN_WIDTH*2*2+7] = v3;
			dest[SCREEN_WIDTH*2*3+4] = v3;
			dest[SCREEN_WIDTH*2*3+5] = v3;
			dest[SCREEN_WIDTH*2*3+6] = v3;
			dest[SCREEN_WIDTH*2*3+7] = v3;

			dest[SCREEN_WIDTH*2*4] = v2;
			dest[SCREEN_WIDTH*2*4+1] = v2;
			dest[SCREEN_WIDTH*2*4+2] = v2;
			dest[SCREEN_WIDTH*2*4+3] = v2;
			dest[SCREEN_WIDTH*2*5] = v2;
			dest[SCREEN_WIDTH*2*5+1] = v2;
			dest[SCREEN_WIDTH*2*5+2] = v2;
			dest[SCREEN_WIDTH*2*5+3] = v2;
			dest[SCREEN_WIDTH*2*6] = v2;
			dest[SCREEN_WIDTH*2*6+1] = v2;
			dest[SCREEN_WIDTH*2*6+2] = v2;
			dest[SCREEN_WIDTH*2*6+3] = v2;
			dest[SCREEN_WIDTH*2*7] = v2;
			dest[SCREEN_WIDTH*2*7+1] = v2;
			dest[SCREEN_WIDTH*2*7+2] = v2;
			dest[SCREEN_WIDTH*2*7+3] = v2;

			src += 4;
			dest += 8;
		}
		src += SCREEN_WIDTH*3+8;
		dest += SCREEN_WIDTH*2*7+16;
	}
	if(screen_mustlock()) {
		screen_unlock();
	}
	if(video_grabframe())
		return -1;

	return 0;
}
