/*
 * EffecTV - Realtime Digital Video Effector
 * Copyright (C) 2001 FUKUCHI Kentarou
 *
 * streak.c: like BaltanTV, but following for a long time
 *
 */

#include <stdlib.h>
#include <string.h>
#include "../EffecTV.h"
#include "utils.h"

#define PLANES 32
#define STRIDE 8

int streakStart();
int streakStop();
int streakDraw();
int streakDrawDouble();

static char *effectname = "StreakTV";
static int state = 0;
static unsigned int *buffer;
static unsigned int *planetable[PLANES];
static int plane;
static int format;

effect *streakRegister()
{
	effect *entry;
	
	entry = (effect *)malloc(sizeof(effect));
	if(entry == NULL) {
		free(buffer);
		return NULL;
	}
	
	entry->name = effectname;
	entry->start = streakStart;
	entry->stop = streakStop;
	if(scale == 2)
		entry->draw = streakDrawDouble;
	else
		entry->draw = streakDraw;
	entry->event = NULL;

	return entry;
}

int streakStart()
{
	int i;

	buffer = (unsigned int *)malloc(SCREEN_WIDTH*SCREEN_HEIGHT*4*PLANES);
	if(buffer == NULL)
		return -1;
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

int streakStop()
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

int streakDraw()
{
	int i, cf;
	unsigned int *src, *dest;

	if(video_syncframe())
		return -1;
	src = (unsigned int *)video_getaddress();
	dest = (unsigned int *)screen_getaddress();
	for(i=0; i<SCREEN_WIDTH*SCREEN_HEIGHT; i++) {
		planetable[plane][i] = (src[i] & 0xfcfcfc)>>2;
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
		        + planetable[cf+STRIDE*3][i];
		planetable[plane][i] = (dest[i]&0xfcfcfc)>>2;
	}
	if(screen_mustlock()) {
		screen_unlock();
	}
	screen_update();
	plane++;
	plane = plane & (PLANES-1);

	return 0;
}

int streakDrawDouble()
{
	int i, x, y, cf, v;
	unsigned int *src, *dest;

	if(video_syncframe())
		return -1;
	src = (unsigned int *)video_getaddress();
	dest = (unsigned int *)screen_getaddress();
	for(i=0; i<SCREEN_WIDTH*SCREEN_HEIGHT; i++) {
		planetable[plane][i] = (src[i] & 0xfcfcfc)>>2;
	}
	if(video_grabframe())
		return -1;
	cf = plane & (STRIDE-1);
	if(screen_mustlock()) {
		if(screen_lock() < 0) {
			return 0;
		}
	}
	i = 0;
	for(y=0; y<SCREEN_HEIGHT; y++) {
		for(x=0; x<SCREEN_WIDTH; x++) {
			v = planetable[cf][i]
			  + planetable[cf+STRIDE][i]
			  + planetable[cf+STRIDE*2][i]
			  + planetable[cf+STRIDE*3][i];
			planetable[plane][i] = (v&0xfcfcfcfc)>>2;
			dest[y*2*SCREEN_WIDTH*2+x*2] = v;
			dest[y*2*SCREEN_WIDTH*2+x*2+1] = v;
			dest[(y*2+1)*SCREEN_WIDTH*2+x*2] = v;
			dest[(y*2+1)*SCREEN_WIDTH*2+x*2+1] = v;
			i++;
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
