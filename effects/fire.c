/*
 * EffecTV - Realtime Digital Video Effector
 * Copyright (C) 2001 FUKUCHI Kentarou
 *
 * fire.c: fire dance
 *
 * Fire routine is taken from Frank Jan Sorensen's demo program.
 */

#include <stdlib.h>
#include <string.h>
#include "../EffecTV.h"
#include "utils.h"

int fireStart();
int fireStop();
int fireDraw();
int fireEvent(SDL_Event *);

#define MaxColor 120
#define Decay 20
#define MAGIC_THRESHOLD 50

static char *effectname = "FireTV";
static int state = 0;
static unsigned char *background;
static unsigned char *buffer;
static unsigned int palette[256];
static unsigned char abstable[65536];
#ifdef ENABLE_PALETTECHANGE
static int format;
#endif

#ifndef ENABLE_PALETTECHANGE
inline static unsigned char inline_RGBtoY(int rgb)
{
	int i;

	i = RtoY[(rgb>>16)&0xff];
	i += GtoY[(rgb>>8)&0xff];
	i += BtoY[rgb&0xff];
	return i;
}
#endif

static void makeAbstable()
{
	int x, y;

	for(y=0; y<256; y++) {
		for(x=0; x<256; x++) {
			abstable[y<<8|x] = (abs(x-y)>MAGIC_THRESHOLD)?0xff:0;
		}
	}
	/* compiled code of 'abs(x-y)' includes a conditional branch.
	   Conditional branch costs very expensive on recent CPU than
	   using look up table. */
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
#ifndef ENABLE_PALETTECHANGE
	int i;
	unsigned int *src;
#endif

	if(video_syncframe())
		return -1;
#ifdef ENABLE_PALETTECHANGE
	bcopy(video_getaddress(), background, SCREEN_AREA);
#else
	src = (unsigned int *)video_getaddress();
	for(i=0; i<SCREEN_AREA; i++) {
		background[i] = inline_RGBtoY(src[i]);
	}
#endif
	if(video_grabframe())
		return -1;

	return 0;
}

effect *fireRegister()
{
	effect *entry;
	
	sharedbuffer_reset();
	background = (unsigned char *)sharedbuffer_alloc(SCREEN_AREA);
	buffer = (unsigned char *)sharedbuffer_alloc(SCREEN_AREA);
	if(background == NULL || buffer == NULL) {
		return NULL;
	}
	bzero(buffer, SCREEN_AREA);

	yuvTableInit();

	entry = (effect *)malloc(sizeof(effect));
	if(entry == NULL) {
		return NULL;
	}
	
	entry->name = effectname;
	entry->start = fireStart;
	entry->stop = fireStop;
	entry->draw = fireDraw;
	entry->event = fireEvent;

	makePalette();
	makeAbstable();

	return entry;
}

int fireStart()
{
	bzero(buffer, SCREEN_AREA);
	screen_clear(0);
#ifdef ENABLE_PALETTECHANGE
	format = video_getformat();
	if(video_setformat(VIDEO_PALETTE_GREY))
		return -1;
#endif
	if(video_grabstart())
		return -1;
	if(setBackground())
		return -1;

	state = 1;
	return 0;
}

int fireStop()
{
	if(state) {
		video_grabstop();
#ifdef ENABLE_PALETTECHANGE
		video_setformat(format);
#endif
		state = 0;
	}

	return 0;
}

int fireDraw()
{
	int i, x;
	unsigned char v;
	unsigned char *src;
	unsigned int *dest;

	if(video_syncframe())
		return -1;
	src = video_getaddress();
	dest = (unsigned int *)screen_getaddress();

	for(i=0; i<SCREEN_WIDTH*(SCREEN_HEIGHT-1); i++) {
#ifdef ENABLE_PALETTECHANGE
		buffer[i] |= abstable[src[i]<<8|background[i]];
#else
		v = inline_RGBtoY(((unsigned int *)src)[i]);
		buffer[i] |= abstable[v<<8|background[i]];
#endif
	}
	if(video_grabframe())
		return -1;

	for(x=1; x<SCREEN_WIDTH-1; x++) {
		for(i=1; i<SCREEN_HEIGHT; i++) {
			v = buffer[i*SCREEN_WIDTH+x];
			if(v<Decay)
				buffer[i*SCREEN_WIDTH-SCREEN_WIDTH+x] = 0;
			else
				buffer[i*SCREEN_WIDTH-SCREEN_WIDTH+x+fastrand()%3-1] = v-fastrand()%Decay;
		}
	}

	if(screen_mustlock()) {
		if(screen_lock() < 0) {
			return 0;
		}
	}
	if(scale == 2) {
		src = buffer+1;
		for(i=0; i<SCREEN_HEIGHT; i++) {
			for(x=1; x<SCREEN_WIDTH-1; x++) {
				dest[x*2] = palette[*src];
				dest[x*2+1] = palette[*src];
				dest[x*2+SCREEN_WIDTH*2] = palette[*src];
				dest[x*2+SCREEN_WIDTH*2+1] = palette[*src++];
			}
			src+=2;
			dest += SCREEN_WIDTH*4;
		}
	} else {
		for(i=0; i<SCREEN_HEIGHT; i++) {
			for(x=1; x<SCREEN_WIDTH-1; x++) {
				dest[i*SCREEN_WIDTH+x] = palette[buffer[i*SCREEN_WIDTH+x]];
			}
		}
	}
	if(screen_mustlock()) {
		screen_unlock();
	}

	return 0;
}

int fireEvent(SDL_Event *event)
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
