/*
 * EffecTV - Realtime Digital Video Effector
 * Copyright (C) 2001 FUKUCHI Kentarou
 *
 * baltan.c: like StreakTV, but following for a long time
 *
 */

#include <stdlib.h>
#include <string.h>
#include "../EffecTV.h"
#include "utils.h"

#define PLANES 32
#define STRIDE 8

int baltanStart();
int baltanStop();
int baltanDraw();
int baltanDrawDouble();

static char *effectname = "BaltanTV";
static int state = 0;
static unsigned int *buffer;
static unsigned int *planetable[PLANES];
static int plane;

effect *baltanRegister()
{
	effect *entry;
	
	entry = (effect *)malloc(sizeof(effect));
	if(entry == NULL) {
		free(buffer);
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

	buffer = (unsigned int *)malloc(SCREEN_AREA*PIXEL_SIZE*PLANES);
	if(buffer == NULL)
		return -1;
	for(i=0;i<PLANES;i++)
		planetable[i] = &buffer[SCREEN_AREA*i];

	plane = 0;
	if(video_grabstart())
		return -1;

	state = 1;
	return 0;
}

int baltanStop()
{
	if(state) {
		video_grabstop();
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
	for(i=0; i<SCREEN_AREA; i++) {
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
	for(i=0; i<SCREEN_AREA; i++) {
		dest[i] = planetable[cf][i]
		        + planetable[cf+STRIDE][i]
		        + planetable[cf+STRIDE*2][i]
		        + planetable[cf+STRIDE*3][i];
		planetable[plane][i] = (dest[i]&0xfcfcfc)>>2;
	}
	if(screen_mustlock()) {
		screen_unlock();
	}
	plane++;
	plane = plane & (PLANES-1);

	return 0;
}

int baltanDrawDouble()
{
	int i, x, y, cf, v;
	unsigned int *src, *dest;

	if(video_syncframe())
		return -1;
	src = (unsigned int *)video_getaddress();
	dest = (unsigned int *)screen_getaddress();
	for(i=0; i<SCREEN_AREA; i++) {
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
	plane++;
	plane = plane & (PLANES-1);

	return 0;
}
