/*
 * EffecTV - Realtime Digital Video Effector
 * Copyright (C) 2001 FUKUCHI Kentarou
 *
 * burn.c: Burn it!
 *
 * Fire routine is taken from Frank Jan Sorensen's demo program.
 */

#include <stdlib.h>
#include <string.h>
#include "../EffecTV.h"
#include "utils.h"

int burnStart();
int burnStop();
int burnDraw();
int burnEvent(SDL_Event *);

#define MaxColor 120
#define Decay 20
#define MAGIC_THRESHOLD 50

static char *effectname = "BurningTV";
static int state = 0;
static int format;
static unsigned char *background;
static unsigned char *buffer;
static unsigned int palette[256];
static unsigned char abstable[65536];

static void makeAbstable()
{
	int x, y;

	for(y=0; y<256; y++) {
		for(x=0; x<256; x++) {
			abstable[y<<8|x] = (abs(x-y)>MAGIC_THRESHOLD)?0xff:0;
		}
	}
}

static void makePalette()
{
	int i, r, g, b;

	for(i=0; i<MaxColor; i++) {
		HSI2RGB(4.6-1.5*i/MaxColor, (double)i/MaxColor, (double)i/MaxColor, &r, &g, &b);
		palette[i] = (r<<16)|(g<<8)|b;
	}
	for(i=MaxColor; i<256; i++) {
		if(r<255)r++;if(r<255)r++;if(r<255)r++;
		if(g<255)g++;
		if(g<255)g++;
		if(b<255)b++;
		if(b<255)b++;
		palette[i] = (r<<16)|(g<<8)|b;
	}
}

static int setBackground()
{
	int i;
	unsigned char *src;
	if(video_syncframe())
		return -1;
	src = video_getaddress();
	for(i=0; i<SCREEN_AREA; i++) {
		background[i] = src[i*4];
	}
	if(video_grabframe())
		return -1;

	return 0;
}

effect *burnRegister()
{
	effect *entry;
	
	sharedbuffer_reset();
	background = (unsigned char *)sharedbuffer_alloc(SCREEN_AREA);
	buffer = (unsigned char *)sharedbuffer_alloc(SCREEN_AREA);
	if(background == NULL || buffer == NULL) {
		return NULL;
	}
	bzero(buffer, SCREEN_AREA);

	entry = (effect *)malloc(sizeof(effect));
	if(entry == NULL) {
		return NULL;
	}
	
	entry->name = effectname;
	entry->start = burnStart;
	entry->stop = burnStop;
	entry->draw = burnDraw;
	entry->event = burnEvent;

	makePalette();
	makeAbstable();

	return entry;
}

int burnStart()
{
	bzero(buffer, SCREEN_AREA);
	format = video_getformat();
	if(video_setformat(VIDEO_PALETTE_RGB32))
		return -1;
	if(video_grabstart())
		return -1;
	if(setBackground())
		return -1;

	state = 1;
	return 0;
}

int burnStop()
{
	if(state) {
		video_grabstop();
		video_setformat(format);
		state = 0;
	}
	return 0;
}

int burnDraw()
{
	int x, y;
	unsigned char v, w;
	unsigned int a, b;
	unsigned char *src;
	unsigned int *dest;

	if(video_syncframe())
		return -1;
	src = video_getaddress();
	dest = (unsigned int *)screen_getaddress();

	for(x=1; x<SCREEN_WIDTH-1; x++) {
		v = 0;
		for(y=0; y<SCREEN_HEIGHT-1; y++) {
			w = abstable[src[(y*SCREEN_WIDTH+x)*4]<<8|background[y*SCREEN_WIDTH+x]];
			buffer[y*SCREEN_WIDTH+x] |= v ^ w;
			v = w;
		}
	}
	if(video_grabframe())
		return -1;

	for(x=1; x<SCREEN_WIDTH-1; x++) {
		for(y=1; y<SCREEN_HEIGHT; y++) {
			v = buffer[y*SCREEN_WIDTH+x];
			if(v<Decay)
				buffer[y*SCREEN_WIDTH-SCREEN_WIDTH+x] = 0;
			else
				buffer[y*SCREEN_WIDTH-SCREEN_WIDTH+x+fastrand()%3-1] = v-fastrand()%Decay;
		}
	}

	if(screen_mustlock()) {
		if(screen_lock() < 0) {
			return 0;
		}
	}
	if(scale==2) {
		for(y=0; y<SCREEN_HEIGHT; y++) {
			for(x=1; x<SCREEN_WIDTH-1; x++) {
				a = ((unsigned int *)src)[y*SCREEN_WIDTH+x] & 0xfefeff;
				b = palette[buffer[y*SCREEN_WIDTH+x]] & 0xfefeff;
				a += b;
				b = a & 0x1010100;
				a = a | (b - (b >> 8));
				dest[y*4*SCREEN_WIDTH+x*2] = a;
				dest[y*4*SCREEN_WIDTH+x*2+1] = a;
				dest[y*4*SCREEN_WIDTH+SCREEN_WIDTH*2+x*2] = a;
				dest[y*4*SCREEN_WIDTH+SCREEN_WIDTH*2+x*2+1] = a;
			}
		}
	} else {
		for(y=0; y<SCREEN_HEIGHT; y++) {
			for(x=1; x<SCREEN_WIDTH-1; x++) {
				a = ((unsigned int *)src)[y*SCREEN_WIDTH+x] & 0xfefeff;
				b = palette[buffer[y*SCREEN_WIDTH+x]] & 0xfefeff;
				a += b;
				b = a & 0x1010100;
				dest[y*SCREEN_WIDTH+x] = a | (b - (b >> 8));
			}
		}
	}
	if(screen_mustlock()) {
		screen_unlock();
	}
	screen_update();

	return 0;
}

int burnEvent(SDL_Event *event)
{
	if(event->type == SDL_KEYDOWN) {
		switch(event->key.keysym.sym) {
		case SDLK_SPACE:
			setBackground();
			break;
		default:
			break;
		}
	}
	return 0;
}
