/*
 * EffecTV - Realtime Digital Video Effector
 * Copyright (C) 2001 FUKUCHI Kentarou
 *
 * streak.c: afterimages following
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

int streakStart();
int streakStop();
int streakDraw();
int streakDrawDouble();

static char *effectname = "StreakTV";
static int state = 0;
static unsigned int *buffer;
static unsigned int *planetable[PLANES];
static int plane;

effect *streakRegister()
{
	effect *entry;

	entry = (effect *)malloc(sizeof(effect));
	if(entry == NULL) {
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

	buffer = (unsigned int *)malloc(SCREEN_AREA*PIXEL_SIZE*PLANES);
	if(buffer == NULL)
		return -1;
	bzero(buffer, SCREEN_AREA*PIXEL_SIZE*PLANES);
	for(i=0;i<PLANES;i++)
		planetable[i] = &buffer[SCREEN_AREA*i];

	plane = 0;
	if(video_grabstart())
		return -1;

	state = 1;
	return 0;
}

int streakStop()
{
	if(state) {
		video_grabstop();
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
	for(i=0; i<SCREEN_AREA; i++) {
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
	for(i=0; i<SCREEN_AREA; i++) {
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
	plane++;
	plane = plane & (PLANES-1);

	return 0;
}

int streakDrawDouble()
{
	int i, x, y, cf;
	unsigned int *src, *dest;

	if(video_syncframe())
		return -1;
	src = (unsigned int *)video_getaddress();
	dest = (unsigned int *)screen_getaddress();
	for(i=0; i<SCREEN_AREA; i++) {
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
	plane++;
	plane = plane & (PLANES-1);

	return 0;
}
