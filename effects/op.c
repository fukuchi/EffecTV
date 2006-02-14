/*
 * EffecTV - Realtime Digital Video Effector
 * Copyright (C) 2001-2006 FUKUCHI Kentaro
 *
 * OpTV - Optical art meets real-time video effect.
 * Copyright (C) 2004-2005 FUKUCHI Kentaro
 *
 */

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "EffecTV.h"
#include "utils.h"

static int start(void);
static int stop(void);
static int draw(RGB32 *src, RGB32 *dest);
static int event(SDL_Event *event);

static char *effectname = "OpTV";
static int stat;
static unsigned char phase;
static int mode = 0;
static int speed = 16;
static int speedInc = 0;
#define OPMAP_MAX 4
static char *opmap[OPMAP_MAX];
#define OP_SPIRAL1  0
#define OP_SPIRAL2  1
#define OP_PARABOLA 2
#define OP_HSTRIPE  3

static RGB32 palette[256];

static void initPalette(void)
{
	int i;
	unsigned char v;

	for(i=0; i<112; i++) {
		palette[i] = 0;
		palette[i+128] = 0xffffff;
	}
	for(i=0; i<16; i++) {
		v = 16 * (i + 1) - 1;
		palette[i+112] = (v<<16) | (v<<8) | v;
		v = 255 - v;
		palette[i+240] = (v<<16) | (v<<8) | v;
	}
}

static void setOpmap(void)
{
	int i, j, x, y;
#ifndef PS2
	double xx, yy, r, at, rr;
#else
	float xx, yy, r, at, rr;
#endif
	int sci;

	sci = 640 / video_width;
	i = 0;
	for(y=0; y<video_height; y++) {
		yy = (double)(y - video_height/2) / video_width;
		for(x=0; x<video_width; x++) {
			xx = (double)x / video_width - 0.5;
#ifndef PS2
			r = sqrt(xx * xx + yy * yy);
			at = atan2(xx, yy);
#else
			r = sqrtf(xx * xx + yy * yy);
			at = atan2f(xx, yy);
#endif

			opmap[OP_SPIRAL1][i] = ((unsigned int)
				((at / M_PI * 256) + (r * 4000))) & 255;

			j = r * 300 / 32;
			rr = r * 300 - j * 32;
			j *= 64;
			j += (rr > 28) ? (rr - 28) * 16 : 0;
			opmap[OP_SPIRAL2][i] = ((unsigned int)
				((at / M_PI * 4096) + (r * 1600) - j )) & 255;

			opmap[OP_PARABOLA][i] = ((unsigned int)(yy/(xx*xx*0.3+0.1)*400))&255;
			opmap[OP_HSTRIPE][i] = x*8*sci;
			i++;
		}
	}
}

effect *opRegister(void)
{
	effect *entry;
	int i;
	
	for(i=0; i<OPMAP_MAX; i++) {
		opmap[i] = (char *)malloc(video_area);
		if(opmap[i] == NULL) {
			return NULL;
		}
	}

	initPalette();
	setOpmap();

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
	phase = 0;
	image_set_threshold_y(50);

	stat = 1;
	return 0;
}

static int stop(void)
{
	stat = 0;

	return 0;
}

static int draw(RGB32 *src, RGB32 *dest)
{
	int x, y;
	char *p;
	unsigned char *diff;

	switch(mode) {
		default:
		case 0:
			p = opmap[OP_SPIRAL1];
			break;
		case 1:
			p = opmap[OP_SPIRAL2];
			break;
		case 2:
			p = opmap[OP_PARABOLA];
			break;
		case 3:
			p = opmap[OP_HSTRIPE];
			break;
	}

	speed += speedInc;
	phase -= speed;

	diff = image_y_over(src);
	for(y=0; y<video_height; y++) {
		for(x=0; x<video_width; x++) {
			*dest++ = palette[(((char)(*p+phase))^*diff++)&255];
			p++;
		}
	}

	return 0;
}

static int event(SDL_Event *event)
{
	if(event->type == SDL_KEYDOWN) {
		switch(event->key.keysym.sym) {
		case SDLK_1:
			mode = 0;
			break;
		case SDLK_2:
			mode = 1;
			break;
		case SDLK_3:
			mode = 2;
			break;
		case SDLK_4:
			mode = 3;
			break;
		case SDLK_INSERT:
			if(speed >= 0) {
				speedInc = 1;
			} else {
				speedInc = -1;
			}
			break;
		case SDLK_DELETE:
			if(speed >= 0) {
				speedInc = -1;
			} else {
				speedInc = 1;
			}
			break;
		case SDLK_SPACE:
			speed = -speed;
			break;
		case SDLK_q:
			speed = 4;
			break;
		case SDLK_w:
			speed = 8;
			break;
		case SDLK_e:
			speed = 16;
			break;
		case SDLK_r:
			speed = 32;
			break;
		case SDLK_t:
			speed = 64;
			break;
		case SDLK_y:
			speed = -64;
			break;
		case SDLK_u:
			speed = -32;
			break;
		case SDLK_i:
			speed = -16;
			break;
		case SDLK_o:
			speed = -8;
			break;
		case SDLK_p:
			speed = -4;
			break;
		default:
			break;
		}
	} else if(event->type == SDL_KEYUP) {
		switch(event->key.keysym.sym) {
		case SDLK_INSERT:
			speedInc = 0;
			break;
		case SDLK_DELETE:
			speedInc = 0;
			break;
		default:
			break;
		}
	}
	return 0;
}
