/*
 * EffecTV - Realtime Digital Video Effector
 * Copyright (C) 2001 FUKUCHI Kentarou
 *
 * StreakTV - afterimage effector.
 * Copyright (C) 2001 FUKUCHI Kentarou
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

static char *effectname = "StreakTV";
static int state = 0;
static RGB32 *buffer;
static RGB32 *planetable[PLANES];
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
	entry->draw = streakDraw;
	entry->event = NULL;

	return entry;
}

int streakStart()
{
	int i;

	buffer = (RGB32 *)malloc(video_area * sizeof(RGB32) * PLANES);
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

int streakStop()
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

int streakDraw()
{
	int i, cf;
	RGB32 *src, *dest;

	if(video_syncframe())
		return -1;
	src = (RGB32 *)video_getaddress();
	for(i=0; i<video_area; i++) {
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
	if(stretch) {
		dest = stretching_buffer;
	} else {
		dest = (RGB32 *)screen_getaddress();
	}
	for(i=0; i<video_area; i++) {
		dest[i] = planetable[cf][i]
		        + planetable[cf+STRIDE][i]
		        + planetable[cf+STRIDE*2][i]
		        + planetable[cf+STRIDE*3][i]
		        + planetable[cf+STRIDE*4][i]
		        + planetable[cf+STRIDE*5][i]
		        + planetable[cf+STRIDE*6][i]
		        + planetable[cf+STRIDE*7][i];
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
