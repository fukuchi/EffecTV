/*
 * EffecTV - Realtime Digital Video Effector
 * Copyright (C) 2001 FUKUCHI Kentarou
 *
 * blurzoom.c: blur and zoom (I need a good name...)
 *
 * I referred to "DUNE!" by QuoVadis for this effect.
 */

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "../EffecTV.h"
#include "utils.h"

#define COLORS 32
#define MAGIC_THRESHOLD 40
#define RATIO 0.95

int blurzoomStart();
int blurzoomStop();
int blurzoomDraw();
int blurzoomDrawDouble();

extern void blurzoomcore();

unsigned char *blurzoombuf;
int blurzoomx[SCREEN_WIDTH];
int blurzoomy[SCREEN_HEIGHT];

static char *effectname = "RadioacTV";
static int format;
static unsigned int *background;
static unsigned int palette[COLORS];
static unsigned char abstable[65536];

static void makeAbstable()
{
	int x, y;

	for(y=0; y<256; y++) {
		for(x=0; x<256; x++) {
			abstable[y<<8|x] = (abs(x-y)>MAGIC_THRESHOLD)?0x1f:0;
		}
	}
}

static void setTable()
{
	int b,x,y,tx,ty,ttx;
	int esi,mesi;

	mesi=0;
	ty=(int)(0.5+RATIO*(-SCREEN_HEIGHT/2)+SCREEN_HEIGHT/2);
	tx=(int)(0.5+RATIO*(-SCREEN_WIDTH/2)+SCREEN_WIDTH/2);
	esi=ty*320+tx;
	blurzoomy[0]=esi-mesi;
	mesi=esi;
	for(ttx=0;ttx<10;ttx++){
		b=0;
		for(x=0;x<32;x++){
			tx=(int)(0.5+RATIO*(ttx*32+x-160)+160);
			esi=ty*320+tx;
			b=b<<1;
			if(esi!=mesi)
				b|=1;
			mesi=esi;
		}
		blurzoomx[ttx]=b;
	}
	tx=(int)(0.5+RATIO*(-160)+160);
	ttx=(int)(0.5+RATIO*(319-160)+160);
	for(y=1;y<240;y++){
		ty=(int)(0.5+RATIO*(y-120)+120);
		esi=ty*320+tx;
		blurzoomy[y]=esi-mesi;
		mesi=ty*320+ttx;
	}
}		

static void makePalette()
{
	int i;

#define DELTA (255/(COLORS/2-1))

	for(i=0; i<COLORS/2; i++) {
		palette[i] = i*DELTA;
	}
	for(i=0; i<COLORS/2; i++) {
		palette[i+COLORS/2] = 255 | (i*DELTA)<<16 | (i*DELTA)<<8;
	}
	for(i=0; i<COLORS; i++) {
		palette[i] = palette[i] & 0xfefeff;
	}
}

effect *blurzoomRegister()
{
	effect *entry;
	
	sharedbuffer_reset();
	background = (unsigned int *)sharedbuffer_alloc(SCREEN_WIDTH*SCREEN_HEIGHT*4);
	blurzoombuf = (unsigned char *)sharedbuffer_alloc(SCREEN_WIDTH*SCREEN_HEIGHT*2);
	if(background == NULL || blurzoombuf == NULL) {
		return NULL;
	}

	entry = (effect *)malloc(sizeof(effect));
	if(entry == NULL) {
		return NULL;
	}
	
	entry->name = effectname;
	entry->start = blurzoomStart;
	entry->stop = blurzoomStop;
	entry->draw = blurzoomDraw;
	entry->event = NULL;

	setTable();
	makePalette();
	makeAbstable();

	return entry;
}

int blurzoomStart()
{
	bzero(blurzoombuf, SCREEN_WIDTH*SCREEN_HEIGHT*2);
	format = video_getformat();
	if(video_setformat(VIDEO_PALETTE_RGB32) < 0)
		return -1;
	video_grabstart();
	video_syncframe();
	bcopy(video_getaddress(), background, SCREEN_WIDTH*SCREEN_HEIGHT*4);
	video_grabframe();

	return 0;
}

int blurzoomStop()
{
	video_grabstop();
	video_setformat(format);

	return 0;
}

int blurzoomDraw()
{
	int i, x, y, v;
	unsigned int a, b;
	unsigned int *src, *dest;

	video_syncframe();
	src = (unsigned int *)video_getaddress();
	dest = (unsigned int *)screen_getaddress();

	for(i=0; i<SCREEN_WIDTH*SCREEN_HEIGHT; i++) {
		blurzoombuf[i] |= abstable[(src[i]&0xff)<<8|(background[i]&0xff)];
		background[i] = src[i];
	}
	video_grabframe();

	blurzoomcore();

	if(screen_mustlock()) {
		if(screen_lock() < 0) {
			return 0;
		}
	}
	if(scale == 2) {
		i=0;
		for(y=0; y<SCREEN_HEIGHT; y++) {
			for(x=0; x<SCREEN_WIDTH; x++) {
				a = background[i] & 0xfefeff;
				b = palette[blurzoombuf[i]];
				a += b;
				b = a & 0x1010100;
				v = a | (b - (b >> 8));
				i++;
				dest[x*2] = v;
				dest[x*2+1] = v;
				dest[x*2+SCREEN_WIDTH*2] = v;
				dest[x*2+SCREEN_WIDTH*2+1] = v;
			}
			dest += SCREEN_WIDTH*4;
		}
	} else {
		for(i=0; i<SCREEN_WIDTH*SCREEN_HEIGHT; i++) {
			a = background[i] & 0xfefeff;
			b = palette[blurzoombuf[i]];
			a += b;
			b = a & 0x1010100;
			dest[i] = a | (b - (b >> 8));
		}
	}
	if(screen_mustlock()) {
		screen_unlock();
	}
	screen_update();

	return 0;
}
