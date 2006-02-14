/*
 * EffecTV - Realtime Digital Video Effector
 * Copyright (C) 2001-2006 FUKUCHI Kentaro
 *
 * ColourfulStreak - streak effect with color.
 *                   It blends Red, Green and Blue layers independently. The
 *                   number of frames for blending are different to each layers.
 * Copyright (C) 2005 Ryo-ta
 *
 * Ported to EffecTV by Kentaro Fukuchi
 *
 * This is heavy effect because of the 3 divisions per pixel.
 * If you want a light effect, you can declare 'blendnum' as a constant value,
 * and disable the parameter controll (comment out the inside of 'event()').
 *
 */

#include <stdlib.h>
#include <string.h>
#include "EffecTV.h"
#include "utils.h"

#define MAX_PLANES 32

static int start(void);
static int stop(void);
static int draw(RGB32 *src, RGB32 *dest);
static int event(SDL_Event *event);

static char *effectname = "ColourfulStreak";
static int state = 0;
static unsigned char *buffer;
static unsigned char *Rplane;
static unsigned char *Gplane;
static unsigned char *Bplane;
static int plane;
static int blendnum = 4;

effect *colstreakRegister(void)
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
	buffer = (unsigned char *)malloc(video_area * MAX_PLANES * 3);
	if(buffer == NULL)
		return -1;
	memset(buffer, 0, video_area * MAX_PLANES * 3);

	Bplane = buffer;
	Gplane = buffer + video_area * MAX_PLANES;
	Rplane = buffer + video_area * MAX_PLANES * 2;

	plane = 0;

	state = 1;
	return 0;
}

static int stop(void)
{
	if(state) {
		if(buffer) {
			free(buffer);
			buffer = NULL;
		}
		state = 0;
	}
	return 0;
}

static int draw(RGB32 *src, RGB32 *dest)
{
	int i, j, cf, pf;
	RGB32 v;
	unsigned int R, G, B;
	unsigned char *pR, *pG, *pB;

	pf = plane - 1;
	if(pf < 0) pf += MAX_PLANES;

	pB = Bplane;
	pG = Gplane;
	pR = Rplane;

	for(i=0; i<video_area; i++) {
		v = *src++;
		B = (unsigned char)v;
		G = (unsigned char)(v >> 8);
		R = (unsigned char)(v >> 16);
		pB[plane] = B;
		pG[plane] = G;
		pR[plane] = R;

		cf = pf;
		for(j=1; j<blendnum; j++) {
			B += pB[cf];
			cf = (cf + MAX_PLANES - 1) & (MAX_PLANES - 1);
		}
		B /= blendnum;

		cf = pf;
		for(j=1; j<blendnum*2; j++) {
			G += pG[cf];
			cf = (cf + MAX_PLANES - 1) & (MAX_PLANES - 1);
		}
		G /= blendnum * 2;

		cf = pf;
		for(j=1; j<blendnum*3; j++) {
			R += pR[cf];
			cf = (cf + MAX_PLANES - 1) & (MAX_PLANES - 1);
		}
		R /= blendnum * 3;
		*dest++ = (R<<16)|(G<<8)|B;

		pB += MAX_PLANES;
		pG += MAX_PLANES;
		pR += MAX_PLANES;
	}
	plane++;
	plane = plane & (MAX_PLANES-1);

	return 0;
}

static int event(SDL_Event *event)
{
	if(event->type == SDL_KEYDOWN) {
		switch(event->key.keysym.sym) {
		case SDLK_INSERT:
			blendnum++;
			if(blendnum > MAX_PLANES / 3) {
				blendnum = MAX_PLANES / 3;
			}
			break;

		case SDLK_DELETE:
			if(blendnum > 1) blendnum--;
			break;

		default:
			break;
		}
	}

	return 0;
}
