/*
 * EffecTV - Realtime Digital Video Effector
 * Copyright (C) 2001-2006 FUKUCHI Kentaro
 *
 * ChameleonTV - Vanishing into the wall!!
 * Copyright (C) 2003 FUKUCHI Kentaro
 *
 */

#include <stdlib.h>
#include <string.h>
#include "EffecTV.h"
#include "utils.h"

#define PLANES_DEPTH 6
#define PLANES (1<<PLANES_DEPTH) 

static int start(void);
static int stop(void);
static int draw(RGB32 *src, RGB32 *dest);
static int event(SDL_Event *);

static char *effectname = "ChameleonTV";
static int state = 0;
static int mode = 0;
static RGB32 *bgimage = NULL;
static int bgIsSet;
static unsigned int *sum = NULL;
static unsigned char *timebuffer = NULL;
static int plane;
static void setBackground(RGB32 *src);
static void drawDisappearing(RGB32 *src, RGB32 *dest);
static void drawAppearing(RGB32 *src, RGB32 *dest);

effect *chameleonRegister(void)
{
	effect *entry;
	
	sharedbuffer_reset();
	sum = (unsigned int *)sharedbuffer_alloc(video_area * sizeof(unsigned int));
	bgimage = (RGB32 *)sharedbuffer_alloc(video_area * PIXEL_SIZE);
	if(sum == NULL || bgimage == NULL)
		return NULL;

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
	timebuffer = (unsigned char *)malloc(video_area * PLANES);
	if(timebuffer == NULL)
		return -1;

	memset(timebuffer, 0, video_area * PLANES);
	memset(sum, 0, video_area * sizeof(unsigned int));
	bgIsSet = 0;
	plane = 0;

	state = 1;
	return 0;
}

static int stop(void)
{
	if(state) {
		if(timebuffer) {
			free(timebuffer);
			timebuffer = NULL;
		}
		state = 0;
	}

	return 0;
}

static int draw(RGB32 *src, RGB32 *dest)
{
	if(!bgIsSet) {
		setBackground(src);
	}

	if(mode == 0) {
		drawDisappearing(src, dest);
	} else {
		drawAppearing(src, dest);
	}

	return 0;
}

static void drawDisappearing(RGB32 *src, RGB32 *dest)
{
	int i;
	unsigned int Y;
	int r, g, b;
	int R, G, B;
	unsigned char *p;
	RGB32 *q;
	unsigned int *s;

	p = timebuffer + plane * video_area;
	q = bgimage;
	s = sum;
	for(i=0; i<video_area; i++) {
		Y = *src++;

		r = (Y>>16) & 0xff;
		g = (Y>>8) & 0xff;
		b = Y & 0xff;

		R = (*q>>16) & 0xff;
		G = (*q>>8) & 0xff;
		B = *q & 0xff;

		Y = (r + g * 2 + b) >> 2;
		*s -= *p;
		*s += Y;
		*p = Y;
		Y = (abs(((int)Y<<PLANES_DEPTH) - (int)(*s)) * 8)>>PLANES_DEPTH;
		if(Y>255) Y = 255;

		R += ((r - R) * Y) >> 8;
		G += ((g - G) * Y) >> 8;
		B += ((b - B) * Y) >> 8;
		*dest++ = (R<<16)|(G<<8)|B;

		p++;
		q++;
		s++;
	}
	plane++;
	plane = plane & (PLANES-1);
}

static void drawAppearing(RGB32 *src, RGB32 *dest)
{
	int i;
	unsigned int Y;
	int r, g, b;
	int R, G, B;
	unsigned char *p;
	RGB32 *q;
	unsigned int *s;

	p = timebuffer + plane * video_area;
	q = bgimage;
	s = sum;
	for(i=0; i<video_area; i++) {
		Y = *src++;

		r = (Y>>16) & 0xff;
		g = (Y>>8) & 0xff;
		b = Y & 0xff;

		R = (*q>>16) & 0xff;
		G = (*q>>8) & 0xff;
		B = *q & 0xff;

		Y = (r + g * 2 + b) >> 2;
		*s -= *p;
		*s += Y;
		*p = Y;
		Y = (abs(((int)Y<<PLANES_DEPTH) - (int)(*s)) * 8)>>PLANES_DEPTH;
		if(Y>255) Y = 255;

		r += ((R - r) * Y) >> 8;
		g += ((G - g) * Y) >> 8;
		b += ((B - b) * Y) >> 8;
		*dest++ = (r<<16)|(g<<8)|b;

		p++;
		q++;
		s++;
	}
	plane++;
	plane = plane & (PLANES-1);
}

static void setBackground(RGB32 *src)
{
	memcpy(bgimage, src, video_area * PIXEL_SIZE);
	bgIsSet = 1;
}

static int event(SDL_Event *event)
{
	if(event->type == SDL_KEYDOWN) {
		switch(event->key.keysym.sym) {
		case SDLK_SPACE:
			bgIsSet = 0;
			break;
		case SDLK_1:
		case SDLK_KP1:
			mode = 0;
			break;
		case SDLK_2:
		case SDLK_KP2:
			mode = 1;
			break;
		default:
			break;
		}
	}
	return 0;
}
