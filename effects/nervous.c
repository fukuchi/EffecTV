/*
 * EffecTV - Realtime Digital Video Effector
 * Copyright (C) 2001 FUKUCHI Kentarou
 *
 * nervousTV - The name says it all...
 * Copyright (C) 2002 TANNENBAUM Edo
 *
 * 2002/2/9 
 *   Original code copied same frame twice, and did not use memcpy().
 *   I modifed those point.
 *   -Kentarou Fukuchi
 */

#include <stdlib.h>
#include <string.h>
#include "../EffecTV.h"
#include "utils.h"

#define PLANES 32


int nervousStart();
int nervousStop();
int nervousDraw();
static int firstflag=1;

static char *effectname = "NervousTV";
static int state = 0;
static RGB32 *buffer;
static RGB32 *planetable[PLANES];
static int plane;

effect *nervousRegister()
{
	effect *entry;

	entry = (effect *)malloc(sizeof(effect));
	if(entry == NULL) {
		free(buffer);
		return NULL;
	}

	entry->name = effectname;
	entry->start = nervousStart;
	entry->stop = nervousStop;
	entry->draw = nervousDraw;
	entry->event = NULL;

	return entry;
}

int nervousStart()
{
	int i;

	buffer = (RGB32 *)malloc(video_area*sizeof(RGB32)*PLANES);
	if(buffer == NULL)
		return -1;
	bzero(buffer, video_area*sizeof(RGB32)*PLANES);
	for(i=0;i<PLANES;i++)
		planetable[i] = &buffer[video_area*i];

	plane = 0;
	firstflag=1;
	if(video_grabstart())
		return -1;

	state = 1;
	return 0;
}

int nervousStop()
{
	if(state) {
		video_grabstop();
		if(buffer)
			free(buffer);
		state = 0;
	}

	return 0;
}

int nervousDraw()
{
	int i, readplane;
	RGB32 *src, *dest;

	if(video_syncframe())
		return -1;
	src = (RGB32 *)video_getaddress();
	memcpy(planetable[plane], src, video_area * PIXEL_SIZE);
	if(video_grabframe())
		return -1;

	if (firstflag==1){
		for(i=0; i<PLANES ; i++){
			if(i != plane) {
				memcpy(planetable[i], planetable[plane], video_area * PIXEL_SIZE);
			}
		}
		firstflag=0;
	}

	readplane = inline_fastrand()%PLANES;
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
	memcpy(dest, planetable[readplane], video_area * PIXEL_SIZE);
	if(stretch) {
		image_stretch_to_screen();
	}
	if(screen_mustlock()) {
		screen_unlock();
	}
	plane++;
	if (plane == PLANES) plane=0;

	return 0;
}
