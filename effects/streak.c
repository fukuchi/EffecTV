/*
 * EffecTV - Realtime Digital Video Effector
 * Copyright (C) 2001-2003 FUKUCHI Kentaro
 *
 * StreakTV - afterimage effector.
 * Copyright (C) 2001-2002 FUKUCHI Kentaro
 *
 */

#include <stdlib.h>
#include <string.h>
#include "EffecTV.h"
#include "utils.h"

#define PLANES 32
#define STRIDE 4
#define STRIDE_MASK 0xf8f8f8f8
#define STRIDE_SHIFT 3

static int start(void);
static int stop(void);
static int draw(RGB32 *src, RGB32 *dest);

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
		planetable[plane][i] = (src[i] & STRIDE_MASK)>>STRIDE_SHIFT;
	}

	cf = plane & (STRIDE-1);
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
	plane++;
	plane = plane & (PLANES-1);

	return 0;
}
