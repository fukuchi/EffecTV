/*
 * EffecTV - Realtime Digital Video Effector
 * Copyright (C) 2001-2006 FUKUCHI Kentaro
 *
 * PUPTV - comes from "Partial UPdate", certain part of image is updated at a
 *         frame. This techniques is also used by 1DTV, but the result is
 *         (slightly) different.
 * Copyright (C) 2003 FUKUCHI Kentaro
 *
 */

#include <stdlib.h>
#include <string.h>
#include "EffecTV.h"
#include "utils.h"

static int start(void);
static int stop(void);
static int draw(RGB32 *src, RGB32 *dest);
static int event(SDL_Event *);

static char *effectname = "PUPTV";
static int state = 0;
static RGB32 *buffer;
static int bgIsSet;
static int mode = 0;

static int paramInc = 0;

static void randomPup(RGB32 *);
static void diagonalPup(RGB32 *);
static void dissolutionPup(RGB32 *);
static void verticalPup(RGB32 *);
static void horizontalPup(RGB32 *);
static void rasterPup(RGB32 *);
//static void sonicPup(RGB32 *);

static int resetBuffer(RGB32 *src)
{
	memcpy(buffer, src, video_area * PIXEL_SIZE);
	bgIsSet = 1;

	return 0;
}

effect *pupRegister(void)
{
	effect *entry;

	sharedbuffer_reset();
	buffer = (RGB32 *)sharedbuffer_alloc(video_area * PIXEL_SIZE);
	if(buffer == NULL)
		return NULL;

	entry = (effect *)malloc(sizeof(effect));
	if(entry == NULL)
		return NULL;
	
	entry->name = effectname;
	entry->start = start;
	entry->stop = stop;
	entry->draw = draw;
	entry->event = event;

	return entry;
}

static int start(void)
{
	bgIsSet = 0;
	state = 1;

	return 0;
}

static int stop(void)
{
	state = 0;

	return 0;
}

static int draw(RGB32 *src, RGB32 *dest)
{
	if(!bgIsSet) {
		resetBuffer(src);
	}

	switch(mode) {
		case 0:
			verticalPup(src);
			break;
		case 1:
			horizontalPup(src);
			break;
		case 2:
			diagonalPup(src);
			break;
		case 3:
			dissolutionPup(src);
			break;
		case 4:
			randomPup(src);
			break;
		case 5:
			rasterPup(src);
			break;
		default:
			break;
	}

	memcpy(dest, buffer, video_area * PIXEL_SIZE);

	return 0;
}

static int event(SDL_Event *event)
{
	if(event->type == SDL_KEYDOWN) {
		switch(event->key.keysym.sym) {
		case SDLK_SPACE:
			bgIsSet = 0;
			break;
		case SDLK_1:
		case SDLK_2:
		case SDLK_3:
		case SDLK_4:
		case SDLK_5:
		case SDLK_6:
//		case SDLK_7:
//		case SDLK_8:
//		case SDLK_9:
			mode = event->key.keysym.sym - SDLK_1;
			break;
		case SDLK_KP1:
		case SDLK_KP2:
		case SDLK_KP3:
		case SDLK_KP4:
		case SDLK_KP5:
		case SDLK_KP6:
//		case SDLK_KP7:
//		case SDLK_KP8:
//		case SDLK_KP9:
			mode = event->key.keysym.sym - SDLK_KP1;
			break;
		case SDLK_INSERT:
			paramInc = 1;
			break;
		case SDLK_DELETE:
			paramInc = -1;
			break;
		default:
			break;
		}
	} else if(event->type == SDL_KEYUP) {
		switch(event->key.keysym.sym) {
		case SDLK_INSERT:
		case SDLK_DELETE:
			paramInc = 0;
			break;
		default:
			break;
		}
	}

	return 0;
}

static void randomPup(RGB32 *src)
{
	int i, x;
	static int pixNum = 5000;

	if(paramInc != 0) {
		pixNum += (paramInc) * 100;
		if(pixNum < 100) pixNum = 100;
		if(pixNum > 10000) pixNum = 10000;
	}

	for(i=pixNum; i>0; i--) {
		x = inline_fastrand() % video_area;
		buffer[x] = src[x];
	}
}

static void diagonalPup(RGB32 *src)
{
	static int phase = 0;
	static int step = 16;
	int x, y, s;
	RGB32 *p;

	if(paramInc != 0) {
		step += paramInc;
		if(step < -100) step = -100;
		if(step > 100) step = 100;
	}

	if(step == 0) {
		memcpy(buffer, src, video_area * PIXEL_SIZE);
		return;
	}

	s = abs(step);

	p = buffer;
	for(y=0; y<video_height; y++) {
		if(step > 0) {
			x = (phase + y) % s;
		} else {
			x = (phase - y) % s;
		}
		for(; x<video_width; x+=s) {
			p[x] = src[x];
		}
		src += video_width;
		p += video_width;
	}

	phase++;
	if(phase >= s)
		phase = 0;
}

static void dissolutionPup(RGB32 *src)
{
	static int phase = 0;
	static int step = 13;
	int i;

	if(paramInc != 0) {
		step += paramInc;
		if(step < 1) step = 1;
		if(step > 100) step = 100;
	}

	for(i=phase; i<video_area; i+=step) {
		buffer[i] = src[i];
	}

	phase++;
	if(phase >= step)
		phase = 0;
}

static void verticalPup(RGB32 *src)
{
	static int phase = 0;
	static int step = 16;
	int x, y;
	RGB32 *dest;

	if(paramInc != 0) {
		step += paramInc;
		if(step < 2) step = 2;
		if(step > video_width) step = video_width;
		phase %= step;
	}

	dest = buffer;
	for(y=0; y<video_height; y++) {
		for(x=phase; x<video_width; x+=step) {
			dest[x] = src[x];
		}
		src += video_width;
		dest += video_width;
	}

	phase++;
	while(phase >= step) {
		phase -= step;
	}
}

static void horizontalPup(RGB32 *src)
{
	static int phase = 0;
	static int step = 16;
	int y;
	RGB32 *dest;

	if(paramInc != 0) {
		step += paramInc;
		if(step < 2) step = 2;
		if(step > video_height) step = video_height;
		phase %= step;
	}

	src += video_width * phase;
	dest = buffer + video_width * phase;
	for(y=phase; y<video_height; y+=step) {
		memcpy(dest, src, video_width * PIXEL_SIZE);
		src += video_width * step;
		dest += video_width * step;
	}

	phase++;
	while(phase >= step) {
		phase -= step;
	}
}

static void rasterPup(RGB32 *src)
{
	static int phase = 0;
	static int step = 16;
	int x, y;
	unsigned int offset;
	RGB32 *dest;

	if(paramInc != 0) {
		step += paramInc;
		if(step < 2) step = 2;
		if(step > video_height) step = video_height;
		phase %= step;
	}

	offset = 0;

	dest = buffer;
	for(y=0; y<video_height; y++) {
		if(y&1) {
			for(x=phase; x<video_width; x+=step) {
				dest[x] = src[x];
			}
		} else {
			for(x=video_width-1-phase;x>=0; x-=step) {
				dest[x] = src[x];
			}
		}
		src += video_width;
		dest += video_width;
	}

	phase++;
	while(phase >= step) {
		phase -= step;
	}
}

#if 0
static void sonicPup(RGB32 *src)
{
	static int phase = 0;
	static int step = 16;
	int x, y;
	unsigned int offset;
	RGB32 *dest;

	if(paramInc != 0) {
		step += paramInc;
		if(step < 2) step = 2;
		if(step > video_height) step = video_height;
		phase %= step;
	}

	offset = 0;

	dest = buffer;
	for(y=0; y<video_height; y++) {
		x = ((offset >> 16) + phase)% step;
		for(; x<video_width; x+=step) {
			dest[x] = src[x];
		}
		src += video_width;
		dest += video_width;
		offset = offset*1103515245+12345;
	}

	phase++;
	while(phase >= step) {
		phase -= step;
	}
}
#endif
