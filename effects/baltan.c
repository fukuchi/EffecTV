/*
 * EffecTV - Realtime Digital Video Effector
 * Copyright (C) 2001 FUKUCHI Kentarou
 *
 * BaltanTV - like StreakTV, but following for a long time
 * Copyright (C) 2001 FUKUCHI Kentarou
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

static char *effectname = "BaltanTV";
static int state = 0;
static RGB32 *buffer;
static RGB32 *planetable[PLANES];
static int plane;

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
	entry->draw = baltanDraw;
	entry->event = NULL;

	return entry;
}

int baltanStart()
{
	int i;

	buffer = (RGB32 *)malloc(video_area*sizeof(RGB32)*PLANES);
	if(buffer == NULL)
		return -1;
	bzero(buffer, video_area*sizeof(RGB32)*PLANES);
	for(i=0;i<PLANES;i++)
		planetable[i] = &buffer[video_area*i];

	plane = 0;
	if(video_grabstart()) {
		free(buffer);
		buffer = NULL;
		return -1;
	}

	state = 1;
	return 0;
}

int baltanStop()
{
	if(state) {
		video_grabstop();
		if(buffer) {
			free(buffer);
			buffer = NULL;
		}
		state = 0;
	}

	return 0;
}

int baltanDraw()
{
	int i, cf;
	RGB32 *src, *dest;

	if(video_syncframe())
		return -1;
	src = (RGB32 *)video_getaddress();
	for(i=0; i<video_area; i++) {
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
	if(stretch) {
		dest = stretching_buffer;
	} else {
		dest = (RGB32 *)screen_getaddress();
	}
	for(i=0; i<video_area; i++) {
		dest[i] = planetable[cf][i]
		        + planetable[cf+STRIDE][i]
		        + planetable[cf+STRIDE*2][i]
		        + planetable[cf+STRIDE*3][i];
		planetable[plane][i] = (dest[i]&0xfcfcfc)>>2;
	}
	if(stretch) {
		image_stretch_to_screen();
	}
	if(screen_mustlock()) {
		screen_unlock();
	}
	plane++;
	plane = plane & (PLANES-1);

	return 0;
}
