/*
 * EffecTV - Realtime Digital Video Effector
 * Copyright (C) 2001 FUKUCHI Kentarou
 *
 * baltan.c: afterimages following
 *
 */

#include <stdlib.h>
#include <string.h>
#include "../EffecTV.h"
#include "utils.h"

#define PLANES 32
#define STRIDE 4
#define STRIDE_MASK 0xf8f8f8f8
#define STRIDE_SHIFT 3

int baltanStart();
int baltanStop();
int baltanDraw();
int baltanDrawDouble();

static char *effectname = "BaltanTV";
static int state = 0;
static unsigned int *buffer;
static unsigned int *planetable[PLANES];
static int plane;
static int format;

effect *baltanRegister()
{
	effect *entry;

	entry = (effect *)malloc(sizeof(effect));
	if(entry == NULL) {
		return NULL;
	}
	
	entry->name = effectname;
	entry->start = baltanStart;
	entry->stop = baltanStop;
	if(scale == 2)
		entry->draw = baltanDrawDouble;
	else
		entry->draw = baltanDraw;
	entry->event = NULL;

	return entry;
}

int baltanStart()
{
	int i;

	buffer = (unsigned int *)malloc(SCREEN_WIDTH*SCREEN_HEIGHT*4*PLANES);
	if(buffer == NULL)
		return -1;
	bzero(buffer, SCREEN_WIDTH*SCREEN_HEIGHT*4*PLANES);
	for(i=0;i<PLANES;i++)
		planetable[i] = &buffer[SCREEN_WIDTH*SCREEN_HEIGHT*i];

	plane = 0;
	format = video_getformat();
	if(video_setformat(VIDEO_PALETTE_RGB32))
		return -1;
	if(video_grabstart())
		return -1;

	state = 1;
	return 0;
}

int baltanStop()
{
	if(state) {
		video_grabstop();
		video_setformat(format);
		if(buffer)
			free(buffer);
		state = 0;
	}
	return 0;
}

int baltanDraw()
{
	int i, cf;
	unsigned int *src, *dest;

	if(video_syncframe())
		return -1;
	src = (unsigned int *)video_getaddress();
	dest = (unsigned int *)screen_getaddress();
	for(i=0; i<SCREEN_WIDTH*SCREEN_HEIGHT; i++) {
		planetable[plane][i] = (src[i] & STRIDE_MASK)>>STRIDE_SHIFT;
	}
	if(video_grabframe())
		return -1;

	cf = plane & (STRIDE-1);
	if(screen_mustlock()) {
		if(screen_lock() < 0) {
			return 0;
		}
	}
	for(i=0; i<SCREEN_WIDTH*SCREEN_HEIGHT; i++) {
		dest[i] = planetable[cf][i]
		        + planetable[cf+STRIDE][i]
		        + planetable[cf+STRIDE*2][i]
		        + planetable[cf+STRIDE*3][i]
		        + planetable[cf+STRIDE*4][i]
		        + planetable[cf+STRIDE*5][i]
		        + planetable[cf+STRIDE*6][i]
		        + planetable[cf+STRIDE*7][i];
	}
	if(screen_mustlock()) {
		screen_unlock();
	}
	screen_update();
	plane++;
	plane = plane & (PLANES-1);

	return 0;
}

int baltanDrawDouble()
{
	int i, x, y, cf;
	unsigned int *src, *dest;

	if(video_syncframe())
		return -1;
	src = (unsigned int *)video_getaddress();
	dest = (unsigned int *)screen_getaddress();
	for(i=0; i<SCREEN_WIDTH*SCREEN_HEIGHT; i++) {
		planetable[plane][i] = (src[i] & STRIDE_MASK)>>STRIDE_SHIFT;
	}
	if(video_grabframe())
		return -1;
	cf = plane & (STRIDE-1);
	if(screen_mustlock()) {
		if(screen_lock() < 0) {
			return 0;
		}
	}
	for(y=0; y<SCREEN_HEIGHT; y++) {
		for(x=0; x<SCREEN_WIDTH; x++) {
			i = planetable[cf][y*SCREEN_WIDTH+x]
			  + planetable[cf+STRIDE][y*SCREEN_WIDTH+x]
			  + planetable[cf+STRIDE*2][y*SCREEN_WIDTH+x]
			  + planetable[cf+STRIDE*3][y*SCREEN_WIDTH+x]
			  + planetable[cf+STRIDE*4][y*SCREEN_WIDTH+x]
			  + planetable[cf+STRIDE*5][y*SCREEN_WIDTH+x]
			  + planetable[cf+STRIDE*6][y*SCREEN_WIDTH+x]
			  + planetable[cf+STRIDE*7][y*SCREEN_WIDTH+x];
			dest[y*2*SCREEN_WIDTH*2+x*2] = i;
			dest[y*2*SCREEN_WIDTH*2+x*2+1] = i;
			dest[(y*2+1)*SCREEN_WIDTH*2+x*2] = i;
			dest[(y*2+1)*SCREEN_WIDTH*2+x*2+1] = i;
		}
	}
	if(screen_mustlock()) {
		screen_unlock();
	}
	screen_update();
	plane++;
	plane = plane & (PLANES-1);

	return 0;
}
