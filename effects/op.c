/*
 * EffecTV - Realtime Digital Video Effector
 * Copyright (C) 2001-2004 FUKUCHI Kentaro
 *
 * OpTV - Optical art meets real-time video effect.
 * Copyright (C) 2004 FUKUCHI Kentaro
 *
 * Inspired by Adrian Likin's script for the GIMP.
 */

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "EffecTV.h"
#include "utils.h"

static int start(void);
static int stop(void);
static int draw(RGB32 *src, RGB32 *dest);
static int event();

static char *effectname = "OpTV";
static int stat;
static unsigned char phase;
static int mode = 0;
static int speed = 8;
#define OPMAP_MAX 4
static char *opmap[OPMAP_MAX];
#define OP_SPIRAL1  0
#define OP_SPIRAL2  1
#define OP_PARABOLA 2
#define OP_HSTRIPE  3

static void setOpmap()
{
	int i, j, k, x, y;
	double xx, yy, r, at;
	int sc; /* scale factor */

	sc = video_width / 5; /* sc = 64 normally */
	i = 0;
	for(y=0; y<video_height; y++) {
		yy = y - video_height/2;
		for(x=0; x<video_width; x++) {
			xx = x - video_width/2;
			r = sqrt(xx * xx + yy * yy);
			at = atan2(xx, yy);

			opmap[OP_SPIRAL1][i] = ((unsigned int)
				((at/M_PI*256) + (r*1024/sc)))&255;

			j = r;
			k = j %32;
			j = (j/32) * 48;
			j += (k>28)?(k-28)*12:0;
			opmap[OP_SPIRAL2][i] = ((unsigned int)
				((at/M_PI*4096) + (r*8) + j ))&255;

			//opmap[OP_PARABOLA][i] = ((unsigned int)(y*8+exp(-xx*xx*0.00006)*yy*8))&255;
			opmap[OP_PARABOLA][i] = ((unsigned int)(exp(-xx*xx*0.0008/sc)*yy*1024/sc))&255;
			//opmap[OP_RIPPLE][i] = ((unsigned int)-(sqrt(xx*xx+yy*yy)*16+sin(x*0.7+yy*0.3)*17)&255);
			i++;
		}
	}
	i = 0;
	for(y=0; y<video_height; y++) {
		for(x=0; x<video_width; x++) {
			j = x%16;
			opmap[OP_HSTRIPE][i++] = j*16;
		}
	}
}

effect *opRegister()
{
	effect *entry;
	int i;
	
	for(i=0; i<OPMAP_MAX; i++) {
		opmap[i] = (char *)malloc(video_area);
		if(opmap[i] == NULL) {
			return NULL;
		}
	}

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

static int start()
{
	phase = 0;
	image_set_threshold_y(50);

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
	RGB32 v;
	char *pg;
	unsigned char *diff, g;

	switch(mode) {
		default:
		case 0:
			pg = opmap[OP_SPIRAL1];
			break;
		case 1:
			pg = opmap[OP_SPIRAL2];
			break;
		case 2:
			pg = opmap[OP_PARABOLA];
			break;
		case 3:
			pg = opmap[OP_HSTRIPE];
			break;
	}

	diff = image_y_over(src);
	for(y=0; y<video_height; y++) {
		for(x=0; x<video_width; x++) {
			if(*diff++ == 0xff) {
				v = 0xffffff;
			} else {
				v = 0;
			}
			g = (char)(*pg+phase*3)>>7;
			v ^= (g<<16) | (g<<8) | g;
			*dest++ = v;
			pg++;
		}
	}

	phase -= speed;

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
		case SDLK_q:
			speed = 1;
			break;
		case SDLK_w:
			speed = 2;
			break;
		case SDLK_e:
			speed = 4;
			break;
		case SDLK_r:
			speed = 8;
			break;
		case SDLK_t:
			speed = 16;
			break;
		case SDLK_y:
			speed = 32;
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
	}
	return 0;
}
