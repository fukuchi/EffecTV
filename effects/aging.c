/*
 * EffecTV - Realtime Digital Video Effector
 * Copyright (C) 2001-2003 FUKUCHI Kentaro
 *
 * AgingTV - film-aging effect.
 * Copyright (C) 2001-2002 FUKUCHI Kentaro
 *
 */

#include <stdlib.h>
#include <string.h>
#include "EffecTV.h"
#include "utils.h"

static int start(void);
static int stop(void);
static int draw(RGB32 *src, RGB32 *dest);

static char *effectname = "AgingTV";
static int state = 0;
static int area_scale;

static int aging_mode;

static void coloraging(RGB32 *src, RGB32 *dest)
{
	RGB32 a, b;
	int i;

	for(i=0; i<video_area; i++) {
		a = *src++;
		b = (a & 0xfcfcfc)>>2;
		*dest++ = a - b + 0x181818 + ((fastrand()>>8)&0x101010);
	}
}

typedef struct _scratch
{
	int life;
	int x;
	int dx;
	int init;
} scratch;

#define SCRATCH_MAX 20
static scratch scratches[SCRATCH_MAX];
static int scratch_lines;

static void scratching(RGB32 *dest)
{
	int i, y, y1, y2;
	RGB32 *p, a, b;
	const int width = video_width;
	const int height = video_height;

	for(i=0; i<scratch_lines; i++) {
		if(scratches[i].life) {
			scratches[i].x = scratches[i].x + scratches[i].dx;
			if(scratches[i].x < 0 || scratches[i].x > width*256) {
				scratches[i].life = 0;
				break;
			}
			p = dest + (scratches[i].x>>8);
			if(scratches[i].init) {
				y1 = scratches[i].init;
				scratches[i].init = 0;
			} else {
				y1 = 0;
			}
			scratches[i].life--;
			if(scratches[i].life) {
				y2 = height;
			} else {
				y2 = fastrand() % height;
			}
			for(y=y1; y<y2; y++) {
				a = *p & 0xfefeff;
				a += 0x202020;
				b = a & 0x1010100;
				*p = a | (b - (b>>8));
				p += width;
			}
		} else {
			if((fastrand()&0xf0000000) == 0) {
				scratches[i].life = 2 + (fastrand()>>27);
				scratches[i].x = fastrand() % (width * 256);
				scratches[i].dx = ((int)fastrand())>>23;
				scratches[i].init = (fastrand() % (height-1))+1;
			}
		}
	}
}

static int dx[8] = { 1, 1, 0, -1, -1, -1,  0, 1};
static int dy[8] = { 0, -1, -1, -1, 0, 1, 1, 1};
static int dust_interval = 0;

static void dusts(RGB32 *dest)
{
	int i, j;
	int dnum;
	int d, len;
	int x, y;
	const int width = video_width;
	const int height = video_height;

	if(dust_interval == 0) {
		if((fastrand()&0xf0000000) == 0) {
			dust_interval = fastrand()>>29;
		}
		return;
	}

	dnum = area_scale*4 + (fastrand()>>27);
	for(i=0; i<dnum; i++) {
		x = fastrand()%width;
		y = fastrand()%height;
		d = fastrand()>>29;
		len = fastrand()%area_scale + 5;
		for(j=0; j<len; j++) {
			dest[y*width + x] = 0x101010;
			y += dy[d];
			x += dx[d];
			if(x<0 || x>=width) break;
			if(y<0 || y>=height) break;
			d = (d + fastrand()%3 - 1) & 7;
		}
	}
	dust_interval--;
}

static int pits_interval = 0;

static void pits(RGB32 *dest)
{
	int i, j;
	int pnum, size, pnumscale;
	int x, y;
	const int width = video_width;
	const int height = video_height;

	pnumscale = area_scale * 2;
	if(pits_interval) {
		pnum = pnumscale + (fastrand()%pnumscale);
		pits_interval--;
	} else {
		pnum = fastrand()%pnumscale;
		if((fastrand()&0xf8000000) == 0) {
			pits_interval = (fastrand()>>28) + 20;
		}
	}
	for(i=0; i<pnum; i++) {
		x = fastrand()%(width-1);
		y = fastrand()%(height-1);
		size = fastrand()>>28;
		for(j=0; j<size; j++) {
			x = x + fastrand()%3-1;
			y = y + fastrand()%3-1;
			if(x<0 || x>=width) break;
			if(y<0 || y>=height) break;
			dest[y*width + x] = 0xc0c0c0;
		}
	}
}

effect *agingRegister()
{
	effect *entry;

	entry = (effect *)malloc(sizeof(effect));
	if(entry == NULL) return NULL;
	
	entry->name = effectname;
	entry->start = start;
	entry->stop = stop;
	entry->draw = draw;
	entry->event = NULL;

	return entry;
}

static void aging_mode_switch()
{
	switch(aging_mode) {
		default:
		case 0:
			scratch_lines = 7;
	/* Most of the parameters are tuned for 640x480 mode */
	/* area_scale is set to 10 when screen size is 640x480. */
			area_scale = video_width * video_height / 64 / 480;
	}
	if(area_scale <= 0)
		area_scale = 1;
}

static int start()
{
	aging_mode = 0;
	aging_mode_switch();

	state = 1;
	return 0;
}

static int stop()
{
	state = 0;

	return 0;
}

static int draw(RGB32 *src, RGB32 *dest)
{
	coloraging(src, dest);

	scratching(dest);
	pits(dest);
	if(area_scale > 1)
		dusts(dest);

	return 0;
}
