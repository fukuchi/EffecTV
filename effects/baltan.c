/*
 * EffecTV - Realtime Digital Video Effector
 * Copyright (C) 2001-2003 FUKUCHI Kentaro
 *
 * BaltanTV - like StreakTV, but following for a long time
 * Copyright (C) 2001-2002 FUKUCHI Kentaro
 *
 */

#include <stdlib.h>
#include <string.h>
#include "EffecTV.h"
#include "utils.h"

#define PLANES 32
#define STRIDE 8

static int start(void);
static int stop(void);
static int draw(RGB32 *src, RGB32 *dest);

static char *effectname = "BaltanTV";
static int state = 0;
static RGB32 *buffer = NULL;
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
	entry->start = start;
	entry->stop = stop;
	entry->draw = draw;
	entry->event = NULL;

	return entry;
}

static int start()
{
	int i;

	buffer = (RGB32 *)malloc(video_area * PIXEL_SIZE * PLANES);
	if(buffer == NULL)
		return -1;

	memset(buffer, 0, video_area * PIXEL_SIZE * PLANES);
	for(i=0;i<PLANES;i++)
		planetable[i] = &buffer[video_area*i];

	plane = 0;

	state = 1;

	return 0;
}

static int stop()
{
	if(state) {
		if(buffer) {
			free(buffer);
			buffer = NULL;
		}
		state = 0;
	}

	return 0;
}

static int draw(RGB32 *src, RGB32 *dest)
{
	int i, cf;

	for(i=0; i<video_area; i++) {
		planetable[plane][i] = (src[i] & 0xfcfcfc)>>2;
	}

	cf = plane & (STRIDE-1);
	for(i=0; i<video_area; i++) {
		dest[i] = planetable[cf][i]
		        + planetable[cf+STRIDE][i]
		        + planetable[cf+STRIDE*2][i]
		        + planetable[cf+STRIDE*3][i];
		planetable[plane][i] = (dest[i]&0xfcfcfc)>>2;
	}
	plane++;
	plane = plane & (PLANES-1);

	return 0;
}
