/*
 * EffecTV - Realtime Digital Video Effector
 * Copyright (C) 2001-2005 FUKUCHI Kentaro
 *
 * TimeDistortionTV - scratch the surface and playback old images.
 * Copyright (C) 2005 Ryo-ta
 *
 * Ported to EffecTV by Kentaro Fukuchi
 */

#include <stdlib.h>
#include <string.h>
#include "EffecTV.h"
#include "utils.h"

#define PLANES 32
#define MAGIC_THRESHOLD 40

static int start(void);
static int stop(void);
static int draw(RGB32 *src, RGB32 *dest);

static char *effectname = "TimeDistortion";
static int state = 0;
static RGB32 *buffer;
static RGB32 *planetable[PLANES];
static int plane;
static int *warptime;

effect *timeDistortionRegister(void)
{
	effect *entry;

	entry = (effect *)malloc(sizeof(effect));
	if(entry == NULL) {
		return NULL;
	}

	sharedbuffer_reset();
	warptime = (unsigned int *)sharedbuffer_alloc(video_area * sizeof(int));
	if(warptime == NULL) {
		free(entry);
		return NULL;
	}

	
	entry->name = effectname;
	entry->start = start;
	entry->stop = stop;
	entry->draw = draw;
	entry->event = NULL;

	return entry;
}

static int start(void)
{
	int i;

	buffer = (RGB32 *)malloc(video_area * PIXEL_SIZE * PLANES);
	if(buffer == NULL)
		return -1;
	memset(buffer, 0, video_area * PIXEL_SIZE * PLANES);
	for(i=0;i<PLANES;i++)
		planetable[i] = &buffer[video_area*i];

	memset(warptime, 0, video_area * sizeof(int));

	plane = 0;
	image_set_threshold_y(MAGIC_THRESHOLD);

	state = 1;
	return 0;
}

static int stop(void)
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
	int i, x, y;
	unsigned char *diff;
	int *p;

	memcpy(planetable[plane], src, PIXEL_SIZE * video_area);
	diff = image_bgsubtract_update_y(src);

	p = warptime + video_width + 1;
	for(y=video_height - 2; y>0; y--) {
		for(x=video_width - 2; x>0; x--) {
			i = *(p - video_width) + *(p - 1) + *(p + 1) + *(p + video_width);
			if(i > 3) i -= 3;
			*p++ = i >> 2;
		}
		p += 2;
	}

	for(i=0; i<video_area; i++) {
		if(*diff++) {
			warptime[i] = PLANES;
		}
		dest[i] = planetable[(plane - warptime[i] + PLANES) & (PLANES - 1)][i];
	}

	plane++;
	plane = plane & (PLANES-1);

	return 0;
}
