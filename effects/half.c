/*
 * EffecTV - Realtime Digital Video Effector
 * Copyright (C) 2001-2006 FUKUCHI Kentaro
 *
 * NervousHalf - Or your bitter half.
 * Copyright (C) 2002 TANNENBAUM Edo
 * Copyright (C) 2004 Kentaro Fukuchi
 *
 * 2004/11/27 
 *  The most of this code has been taken from Edo's NervousTV.
 */

#include <stdlib.h>
#include <string.h>
#include "EffecTV.h"
#include "utils.h"

#define PLANES 32

static int start(void);
static int stop(void);
static int draw(RGB32 *src, RGB32 *dest);
static int event(SDL_Event *event);

static char *effectname = "NervousHalf";
static int state = 0;
static RGB32 *buffer;
static RGB32 *planetable[PLANES];
static int mode = 0;
static int plane;
static int stock;

static int scratchTimer;
static int scratchStride;
static int scratchCurrent;

static int mirror = 1;
static int dir = 0;
static int delay = 10;

static int nextDelay(void);
static int nextNervous(void);
static int nextScratch(void);

static void left(RGB32 *src, RGB32 *buf, RGB32 *dest, int mirror);
static void right(RGB32 *src, RGB32 *buf, RGB32 *dest, int mirror);
static void upper(RGB32 *src, RGB32 *buf, RGB32 *dest, int mirror);
static void bottom(RGB32 *src, RGB32 *buf, RGB32 *dest, int mirror);

effect *nervousHalfRegister(void)
{
	effect *entry;

	entry = (effect *)malloc(sizeof(effect));
	if(entry == NULL) {
		return NULL;
	}

	entry->name = effectname;
	entry->start = start;
	entry->stop = stop;
	entry->draw = draw;
	entry->event = event;

	return entry;
}

static int start(void)
{
	int i;

	buffer = (RGB32 *)malloc(video_area*PIXEL_SIZE*PLANES);
	if(buffer == NULL)
		return -1;
	memset(buffer, 0, video_area*PIXEL_SIZE*PLANES);
	for(i=0;i<PLANES;i++)
		planetable[i] = &buffer[video_area*i];

	plane = 0;
	stock = 0;
	scratchTimer = 0;
	scratchCurrent = 0;

	state = 1;
	return 0;
}

static int stop(void)
{
	if(state) {
		if(buffer)
			free(buffer);
		state = 0;
	}

	return 0;
}

static int nextDelay(void)
{
	return (plane - delay + PLANES) % PLANES;
}

static int nextScratch(void)
{
	if(scratchTimer) {
		scratchCurrent = scratchCurrent + scratchStride;
		while(scratchCurrent < 0) scratchCurrent += stock;
		while(scratchCurrent >= stock) scratchCurrent -= stock;
		scratchTimer--;
	} else {
		scratchCurrent = inline_fastrand() % stock;
		scratchStride = inline_fastrand() % 5 - 2;
		if(scratchStride >= 0) scratchStride++;
		scratchTimer = inline_fastrand() % 6 + 2;
	}

	return scratchCurrent;
}

static int nextNervous(void)
{
	if(stock > 0)
		return inline_fastrand() % stock;

	return 0;
}

static int draw(RGB32 *src, RGB32 *dest)
{
	int readplane;
	RGB32 *buf;

	memcpy(planetable[plane], src, video_area * PIXEL_SIZE);
	if(stock < PLANES) {
		stock++;
	}

	switch(mode) {
		default:
		case 0:
			readplane = nextDelay();
			break;
		case 1:
			readplane = nextScratch();
			break;
		case 2:
			readplane = nextNervous();
			break;
	}
	buf = planetable[readplane];

	plane++;
	if (plane == PLANES) plane=0;

	switch(dir) {
		default:
		case 0:
			left(src, buf, dest, mirror);
			break;
		case 1:
			right(src, buf, dest, mirror);
			break;
		case 2:
			upper(src, buf, dest, mirror);
			break;
		case 3:
			bottom(src, buf, dest, mirror);
			break;
	}

	return 0;
}

static int event(SDL_Event *event)
{
	if(event->type == SDL_KEYDOWN) {
		switch(event->key.keysym.sym) {
		case SDLK_SPACE:
			mirror++;
			if(mirror > 2) mirror = 0;
			break;
		case SDLK_1:
		case SDLK_2:
		case SDLK_3:
		case SDLK_4:
			dir = event->key.keysym.sym - SDLK_1;
			break;
		case SDLK_KP1:
		case SDLK_KP2:
		case SDLK_KP3:
		case SDLK_KP4:
			dir = event->key.keysym.sym - SDLK_KP1;
			break;
		case SDLK_q:
			mode = 0;
			break;
		case SDLK_w:
			mode = 1;
			break;
		case SDLK_e:
			mode = 2;
			break;
		case SDLK_INSERT:
			if(delay < PLANES - 1) delay++;
			break;
		case SDLK_DELETE:
			if(delay > 0) delay--;
			break;
		default:
			break;
		}
	}
	return 0;
}

static void upper(RGB32 *src, RGB32 *buf, RGB32 *dest, int mirror)
{
	int len;
	int y;
	RGB32 *p;

	len = video_height / 2 * video_width;
	memcpy(dest, src, len * PIXEL_SIZE);

	switch(mirror) {
		case 1:
			p = buf + len - video_width;
			dest += len;
			len = PIXEL_SIZE * video_width;
			for(y = video_height / 2; y > 0; y--) {
				memcpy(dest, p, len);
				p -= video_width;
				dest += video_width;
			}
			break;
		case 2:
			memcpy(dest + len, buf, len * PIXEL_SIZE);
			break;
		case 0:
		default:
			memcpy(dest + len, buf + len, len * PIXEL_SIZE);
			break;
	}
}

static void bottom(RGB32 *src, RGB32 *buf, RGB32 *dest, int mirror)
{
	int len;
	int y;
	RGB32 *p;

	len = video_height / 2 * video_width;
	memcpy(dest + len, src + len, len * PIXEL_SIZE);

	switch(mirror) {
		case 1:
			p = buf + video_area - video_width;
			len = PIXEL_SIZE * video_width;
			for(y = video_height / 2; y > 0; y--) {
				memcpy(dest, p, len);
				p -= video_width;
				dest += video_width;
			}
			break;
		case 2:
			memcpy(dest, buf + len, len * PIXEL_SIZE);
			break;
		case 0:
		default:
			memcpy(dest, buf, len * PIXEL_SIZE);
			break;
	}
}

static void left(RGB32 *src, RGB32 *buf, RGB32 *dest, int mirror)
{
	int x, y, len, st;
	RGB32 *s1, *s2, *d, *d1;

	len = video_width / 2;
	st = len * PIXEL_SIZE;

	switch(mirror) {
		case 1:
			s1 = src;
			s2 = buf + len;
			d = dest;
			for(y=0; y<video_height; y++) {
				memcpy(d, s1, st);
				d1 = d + len;
				for(x=0; x<len; x++) {
					*d1++ = *s2--;
				}
				d += video_width;
				s1 += video_width;
				s2 += video_width + len;
			}
			break;
		case 2:
			s1 = src;
			s2 = buf;
			d = dest;
			for(y=0; y<video_height; y++) {
				memcpy(d, s1, st);
				memcpy(d + len, s2, st);
				d += video_width;
				s1 += video_width;
				s2 += video_width;
			}
			break;
		case 0:
		default:
			s1 = src;
			s2 = buf + len;
			d = dest;
			for(y=0; y<video_height; y++) {
				memcpy(d, s1, st);
				memcpy(d + len, s2, st);
				d += video_width;
				s1 += video_width;
				s2 += video_width;
			}
			break;
	}
}

static void right(RGB32 *src, RGB32 *buf, RGB32 *dest, int mirror)
{
	int x, y, len, st;
	RGB32 *s1, *s2, *d, *d1;

	len = video_width / 2;
	st = len * PIXEL_SIZE;

	switch(mirror) {
		case 1:
			s1 = src + len;
			s2 = buf + video_width - 1;
			d = dest;
			for(y=0; y<video_height; y++) {
				memcpy(d + len, s1, st);
				d1 = d;
				for(x=0; x<len; x++) {
					*d1++ = *s2--;
				}
				d += video_width;
				s1 += video_width;
				s2 += video_width + len;
			}
			break;
		case 2:
			s1 = src + len;
			s2 = buf + len;
			d = dest;
			for(y=0; y<video_height; y++) {
				memcpy(d + len, s1, st);
				memcpy(d, s2, st);
				d += video_width;
				s1 += video_width;
				s2 += video_width;
			}
			break;
		case 0:
		default:
			s1 = src + len;
			s2 = buf;
			d = dest;
			for(y=0; y<video_height; y++) {
				memcpy(d + len, s1, st);
				memcpy(d, s2, st);
				d += video_width;
				s1 += video_width;
				s2 += video_width;
			}
			break;
	}
}
