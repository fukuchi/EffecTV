/*
 * EffecTV - Realtime Digital Video Effector
 * Copyright (C) 2001 FUKUCHI Kentarou
 *
 * mosaic.c: censors incoming objects
 *
 */

#include <stdlib.h>
#include <string.h>
#include "../EffecTV.h"
#include "utils.h"

#define MAGIC_THRESHOLD 50
#define CENSOR_LEVEL 20

int mosaicStart();
int mosaicStop();
int mosaicDraw();
int mosaicDrawDouble();
int mosaicEvent();

static char *effectname = "MosaicTV";
static int stat;
static int format;
static unsigned int *background;
static unsigned char abstable[65536];

static void makeAbstable()
{
	int x, y;

	for(y=0; y<256; y++) {
		for(x=0; x<256; x++) {
			abstable[y<<8|x] = (abs(x-y)>MAGIC_THRESHOLD)?1:0;
		}
	}
}

static int setBackground()
{
	if(video_syncframe())
		return -1;
	bcopy(video_getaddress(), background, SCREEN_WIDTH*SCREEN_HEIGHT*4);
	if(video_grabframe())
		return -1;

	return 0;
}

effect *mosaicRegister()
{
	effect *entry;
	
	sharedbuffer_reset();
	background = (unsigned int *)sharedbuffer_alloc(SCREEN_WIDTH*SCREEN_HEIGHT*4);
	if(background == NULL) {
		return NULL;
	}

	entry = (effect *)malloc(sizeof(effect));
	if(entry == NULL) {
		return NULL;
	}
	
	entry->name = effectname;
	entry->start = mosaicStart;
	entry->stop = mosaicStop;
	if(scale == 2) {
		entry->draw = mosaicDrawDouble;
	} else {
		entry->draw = mosaicDraw;
	}
	entry->event = mosaicEvent;

	makeAbstable();

	return entry;
}

int mosaicStart()
{
	format = video_getformat();
	if(video_setformat(VIDEO_PALETTE_RGB32) < 0)
		return -1;
	if(video_grabstart())
		return -1;
	if(setBackground())
		return -1;

	stat = 1;
	return 0;
}

int mosaicStop()
{
	if(stat) {
		video_grabstop();
		video_setformat(format);
		stat = 0;
	}

	return 0;
}

int mosaicDraw()
{
	int  x, y, xx, yy, v;
	unsigned char count;
	unsigned int *src, *dest;
	unsigned int *p, *q, *r;

	if(video_syncframe())
		return -1;
	if(screen_mustlock()) {
		if(screen_lock() < 0) {
			return 0;
		}
	}
	src = (unsigned int *)video_getaddress();
	dest = (unsigned int *)screen_getaddress();

	for(y=0; y<30; y++) {
		for(x=0; x<40; x++) {
			count = 0;
			p = &src[y*8*SCREEN_WIDTH+x*8];
			q = &dest[y*8*SCREEN_WIDTH+x*8];
			r = &background[y*8*SCREEN_WIDTH+x*8];
			for(yy=0; yy<8; yy++) {
				for(xx=0; xx<8; xx++) {
					count += abstable[(p[yy*SCREEN_WIDTH+xx]&0xff)<<8|(r[yy*SCREEN_WIDTH+xx]&0xff)];
					q[yy*SCREEN_WIDTH+xx] = p[yy*SCREEN_WIDTH+xx];
				}
			}
			if(count > CENSOR_LEVEL) {
				v = p[3*SCREEN_WIDTH+3];
				for(yy=0; yy<8; yy++) {
					for(xx=0; xx<8; xx++){
						q[yy*SCREEN_WIDTH+xx] = v;
					}
				}
			}
		}
	}
	if(screen_mustlock()) {
		screen_unlock();
	}
	screen_update();
	if(video_grabframe())
		return -1;

	return 0;
}

int mosaicDrawDouble()
{
	int  x, y, xx, yy, v;
	unsigned char count;
	unsigned int *src, *dest;
	unsigned int *p, *q, *r;

	if(video_syncframe())
		return -1;
	if(screen_mustlock()) {
		if(screen_lock() < 0) {
			return 0;
		}
	}
	src = (unsigned int *)video_getaddress();
	dest = (unsigned int *)screen_getaddress();

	for(y=0; y<30; y++) {
		for(x=0; x<40; x++) {
			count = 0;
			p = &src[y*8*SCREEN_WIDTH+x*8];
			q = &dest[y*16*SCREEN_WIDTH*2+x*16];
			r = &background[y*8*SCREEN_WIDTH+x*8];
			for(yy=0; yy<8; yy++) {
				for(xx=0; xx<8; xx++) {
					count += abstable[(p[yy*SCREEN_WIDTH+xx]&0xff)<<8|(r[yy*SCREEN_WIDTH+xx]&0xff)];
				}
			}
			if(count > CENSOR_LEVEL) {
				v = p[3*SCREEN_WIDTH+3];
				for(yy=0; yy<16; yy++) {
					for(xx=0; xx<16; xx++){
						q[yy*SCREEN_WIDTH*2+xx] = v;
					}
				}
			} else {
				for(yy=0; yy<8; yy++) {
					for(xx=0; xx<8; xx++){
						v = p[yy*SCREEN_WIDTH+xx];
						q[yy*2*SCREEN_WIDTH*2+xx*2] = v;
						q[yy*2*SCREEN_WIDTH*2+xx*2+1] = v;
						q[(yy*2+1)*SCREEN_WIDTH*2+xx*2] = v;
						q[(yy*2+1)*SCREEN_WIDTH*2+xx*2+1] = v;
					}
				}
			}
		}
	}
	if(screen_mustlock()) {
		screen_unlock();
	}
	screen_update();
	if(video_grabframe())
		return -1;

	return 0;
}

int mosaicEvent(SDL_Event *event)
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
