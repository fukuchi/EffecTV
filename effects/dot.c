/*
 * EffecTV - Realtime Digital Video Effector
 * Copyright (C) 2001-2003 FUKUCHI Kentaro
 *
 * DotTV: convert gray scale image into a set of dots
 * Copyright (C) 2001-2002 FUKUCHI Kentaro
 *
 */

#include <stdlib.h>
#include <string.h>
#include "EffecTV.h"
#include "utils.h"
#include "heart.inc"

static int start(void);
static int stop(void);
static int draw(RGB32 *src, RGB32 *dest);
static int event();

#define DOTDEPTH 5
#define DOTMAX (1<<DOTDEPTH)

static char *effectname = "DotTV";
static int state;
static RGB32 *pattern;
static RGB32 *heart_pattern;
static int dots_width;
static int dots_height;
static int dot_size;
static int dot_hsize;
static int *sampx, *sampy;
static int mode = 0;

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

static void makeOneHeart(int val, unsigned char *bigheart)
{
	int x, y;
	int xx, yy;
	int f1x, f1y;
	int f2x, f2y;
	double s1x, s1y;
	double s2x, s2y;
	double d1x, d1y;
	double d2x, d2y;
	double sum, hsum;
	double w, h;
	RGB32 *pat;
	RGB32 c;
#define SFACT 4

	pat = heart_pattern + val * dot_size * dot_hsize;
	s2y = (double)(-dot_hsize) / dot_size * (31.9 + (double)(DOTMAX-val)/SFACT)
		+ 31.9;
	f2y = (int)s2y;
	for(y=0; y<dot_size; y++) {
		s1y = s2y;
		f1y = f2y;

		s2y = (double)(y+1-dot_hsize) / dot_size
			* (31.9 + (double)(DOTMAX-val)/SFACT) + 31.9;
		f2y = (int)s2y;
		d1y = 1.0 - (s1y - (double)f1y);
		d2y = s2y - (double)f2y;
		h = s2y - s1y;

		s2x = (double)(-dot_hsize) / dot_size
			* (31.9 + (double)(DOTMAX-val)/SFACT) + 31.9;
		f2x = (int)s2x;
		for(x=0; x<dot_hsize; x++) {
			s1x = s2x;
			f1x = f2x;
			s2x = (double)(x+1-dot_hsize) / dot_size
				* (31.9 + (double)(DOTMAX-val)/SFACT) + 31.9;
			f2x = (int)s2x;
			d1x = 1.0 - (s1x - (double)f1x);
			d2x = s2x - (double)f2x;
			w = s2x - s1x;

			sum = 0.0;
			for(yy = f1y; yy <= f2y; yy++) {
				hsum = d1x * bigheart[yy*32+f1x];
				for(xx = f1x+1; xx < f2x; xx++) {
					hsum += bigheart[yy*32+xx];
				}
				hsum += d2x * bigheart[yy*32+f2x];

				if(yy == f1y) {
					sum += hsum * d1y;
				} else if(yy == f2y) {
					sum += hsum * d2y;
				} else {
					sum += hsum;
				}
			}
			c = (RGB32)(sum / w / h);
			if(c<0) c = 0;
			if(c>255) c = 255;
			*pat++ = c<<16;
		}
	}
}

static void makeHeartPattern()
{
	int i, x, y;
	unsigned char *bigheart;

	bigheart = (unsigned char *)malloc(sizeof(unsigned char) * 64 * 32);
	memset(bigheart, 0, 64 * 32 * sizeof(unsigned char));
	for(y=0; y<32; y++) {
		for(x=0; x<16;x++) {
			bigheart[(y+16)*32+x+16] = half_heart[y*16+x];
		}
	}

	for(i=0; i<DOTMAX; i++) {
		makeOneHeart(i, bigheart);
	}

	free(bigheart);
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
	heart_pattern = (RGB32 *)malloc(DOTMAX * dot_hsize * dot_size * PIXEL_SIZE);
	if(heart_pattern == NULL) {
		free(pattern);
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
	entry->start = start;
	entry->stop = stop;
	entry->draw = draw;
	entry->event = event;

	makePattern();
	makeHeartPattern();

	return entry;
}

static int start()
{
	init_sampxy_table();

	state = 1;
	return 0;
}

static int stop()
{
	state = 0;
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

static void drawHeart(int xx, int yy, unsigned char c, RGB32 *dest)
{
	int x, y;
	RGB32 *pat;

	c = (c>>(8-DOTDEPTH));
	pat = heart_pattern + c * dot_size * dot_hsize;
	dest = dest + yy * dot_size * screen_width + xx * dot_size;
	for(y=0; y<dot_size; y++) {
		for(x=0; x<dot_hsize; x++) {
			*dest++ = *pat++;
		}
		pat--;
		for(x=0; x<dot_hsize; x++) {
			*dest++ = *pat--;
		}
		dest += screen_width - dot_size;
		pat += dot_hsize + 1;
	}
}

static int draw(RGB32 *src, RGB32 *dest)
{
	int x, y;
	int sx, sy;

	dest = (RGB32 *)screen_getaddress(); // cheater! cheater!

	if(mode) {
		for(y=0; y<dots_height; y++) {
			sy = sampy[y];
			for(x=0; x<dots_width; x++) {
				sx = sampx[x];
				drawHeart(x, y, inline_RGBtoY(src[sy*video_width+sx]), dest);
			}
		}
	} else {
		for(y=0; y<dots_height; y++) {
			sy = sampy[y];
			for(x=0; x<dots_width; x++) {
				sx = sampx[x];
				drawDot(x, y, inline_RGBtoY(src[sy*video_width+sx]), dest);
			}
		}
	}

	return 1; // undocumented feature ;-)
}

static int event(SDL_Event *event)
{
	if(event->type == SDL_KEYDOWN) {
		switch(event->key.keysym.sym) {
		case SDLK_SPACE:
			mode ^= 1;
			break;
		default:
			break;
		}
	}

	return 0;
}
