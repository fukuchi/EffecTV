/*
 * EffecTV - Realtime Digital Video Effector
 * Copyright (C) 2001-2003 FUKUCHI Kentaro
 *
 * EdgeTV - detects edge and display it in good old computer way. 
 * Copyright (C) 2001-2002 FUKUCHI Kentaro
 *
 * The idea of EdgeTV is taken from Adrian Likins's effector script for GIMP,
 * `Predator effect.'
 *
 * The algorithm of the original script pixelizes the image at first, then
 * it adopts the edge detection filter to the image. It also adopts MaxRGB
 * filter to the image. This is not used in EdgeTV.
 * This code is highly optimized and employs many fake algorithms. For example,
 * it devides a value with 16 instead of using sqrt() in line 132-134. It is
 * too hard for me to write detailed comment in this code in English.
 */

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "EffecTV.h"
#include "utils.h"

static int start(void);
static int stop(void);
static int draw(RGB32 *src, RGB32 *dest);

static char *effectname = "EdgeTV";
static int stat;
static RGB32 *map;

static int map_width;
static int map_height;

static int video_width_margin;

effect *edgeRegister()
{
	effect *entry;
	
	map_width = video_width / 4;
	map_height = video_height / 4;
	video_width_margin = video_width - map_width * 4;

	sharedbuffer_reset();
	map = (RGB32 *)sharedbuffer_alloc(map_width*map_height*PIXEL_SIZE*2);
	if(map == NULL) {
		return NULL;
	}

	entry = (effect *)malloc(sizeof(effect));
	if(entry == NULL) {
		return NULL;
	}
	
	entry->name = effectname;
	entry->start = start;
	entry->stop = stop;
	entry->draw = draw;
	entry->event = NULL;

	return entry;
}

static int start()
{
	memset(map, 0, map_width * map_height * PIXEL_SIZE * 2);

	stat = 1;
	return 0;
}

static int stop()
{
	stat = 0;
	return 0;
}

static int draw(RGB32 *src, RGB32 *dest)
{
	int x, y;
	int r, g, b;
	RGB32 p, q;
	RGB32 v0, v1, v2, v3;

	src += video_width*4+4;
	dest += video_width*4+4;
	for(y=1; y<map_height-1; y++) {
		for(x=1; x<map_width-1; x++) {
			p = *src;
			q = *(src - 4);

/* difference between the current pixel and right neighbor. */
			r = ((int)(p & 0xff0000) - (int)(q & 0xff0000))>>16;
			g = ((int)(p & 0x00ff00) - (int)(q & 0x00ff00))>>8;
			b = ((int)(p & 0x0000ff) - (int)(q & 0x0000ff));
			r *= r; /* Multiply itself and divide it with 16, instead of */
			g *= g; /* using abs(). */
			b *= b;
			r = r>>5; /* To lack the lower bit for saturated addition,  */
			g = g>>5; /* devide the value with 32, instead of 16. It is */
			b = b>>4; /* same as `v2 &= 0xfefeff' */
			if(r>127) r = 127;
			if(g>127) g = 127;
			if(b>255) b = 255;
			v2 = (r<<17)|(g<<9)|b;

/* difference between the current pixel and upper neighbor. */
			q = *(src - video_width*4);
			r = ((int)(p & 0xff0000) - (int)(q & 0xff0000))>>16;
			g = ((int)(p & 0x00ff00) - (int)(q & 0x00ff00))>>8;
			b = ((int)(p & 0x0000ff) - (int)(q & 0x0000ff));
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

			v0 = map[(y-1)*map_width*2+x*2];
			v1 = map[y*map_width*2+(x-1)*2+1];
			map[y*map_width*2+x*2] = v2;
			map[y*map_width*2+x*2+1] = v3;
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
			dest[video_width] = r | (g - (g>>8));
			r = v2 + v3;
			g = r & 0x01010100;
			dest[video_width+1] = r | (g - (g>>8));
			dest[video_width+2] = v3;
			dest[video_width+3] = v3;
			dest[video_width*2] = v2;
			dest[video_width*2+1] = v2;
			dest[video_width*3] = v2;
			dest[video_width*3+1] = v2;

			src += 4;
			dest += 4;
		}
		src += video_width*3+8+video_width_margin;
		dest += video_width*3+8+video_width_margin;
	}

	return 0;
}
