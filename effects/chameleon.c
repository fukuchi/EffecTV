/*
 * EffecTV - Realtime Digital Video Effector
 * Copyright (C) 2001-2003 FUKUCHI Kentaro
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
static RGB32 *bgimage = NULL;
static int bgIsSet;
static unsigned int *sum = NULL;
static unsigned char *timebuffer = NULL;
static int plane;
static void setBackground(RGB32 *src);

effect *chameleonRegister()
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

static int start()
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

static int stop()
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
	int i;
	unsigned int Y;
	unsigned int r, g, b;
	unsigned int R, G, B;
	unsigned char *p;
	RGB32 *q;
	unsigned int *s;

	if(!bgIsSet) {
		setBackground(src);
	}

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
		Y = abs((int)Y - (int)(*s>>PLANES_DEPTH)) * 8;
		if(Y>255) Y = 255;
#if 1
		r = (r * Y) >> 8;
		g = (g * Y) >> 8;
		b = (b * Y) >> 8;
		Y = 255 - Y;
		R = (R * Y) >> 8;
		G = (G * Y) >> 8;
		B = (B * Y) >> 8;
		*dest++ = ((r+R)<<16)|((g+G)<<8)|(b+B);
#else
		*dest++ = Y*0x10101;
#endif

		p++;
		q++;
		s++;
	}
	plane++;
	plane = plane & (PLANES-1);

	return 0;
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
		default:
			break;
		}
	}
	return 0;
}
