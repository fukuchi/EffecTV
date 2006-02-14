/*
 * EffecTV - Realtime Digital Video Effector
 * Copyright (C) 2001-2006 FUKUCHI Kentaro
 *
 * slofastTV - nonlinear time TV
 * Copyright (C) 2005 SCHUBERT Erich
 * based on slofastTV Copyright (C) 2002 TANNENBAUM Edo
 *
 */

#include <stdlib.h>
#include <string.h>
#include "EffecTV.h"
#include "utils.h"

#define PLANES 8


static int start(void);
static int stop(void);
static int draw(RGB32 *src, RGB32 *dest);
static int event(SDL_Event *event);

static char *effectname = "SloFastTV";
static int state = 0;
static RGB32 *buffer;
static RGB32 *planetable[PLANES];
static int mode = 0;
static int head;
static int tail;
static int count;

#define STATE_STOP  0
#define STATE_FILL  1
#define STATE_FLUSH 2

effect *slofastRegister(void)
{
	effect *entry;

	entry = (effect *)malloc(sizeof(effect));
	if(entry == NULL) {
		free(buffer);
		return NULL;
	}

	entry->name = effectname;
	entry->start = start;
	entry->stop = stop;
	entry->draw = draw;
	entry->event = event;

	return entry;
}

static int start(void)
{
	int i;

	buffer = (RGB32 *)malloc(video_area*PIXEL_SIZE*PLANES);
	if(buffer == NULL)
		return -1;
	memset(buffer, 0, video_area*PIXEL_SIZE*PLANES);
	for(i=0;i<PLANES;i++)
		planetable[i] = &buffer[video_area*i];

	head  = 0;
	tail  = 0;
	count = 0;

	mode = STATE_FILL;
	return 0;
}

static int stop(void)
{
	if(state) {
		if(buffer)
			free(buffer);
		state = 0;
	}

	return 0;
}

static int draw(RGB32 *src, RGB32 *dest)
{
	/* store new frame */
	if ((mode == STATE_FILL) || (count & 0x1) == 1) {
		memcpy(planetable[head], src, video_area * PIXEL_SIZE);
		head++;
		while (head >= PLANES) head=0;

		/* switch mode when head catches tail */
		if (head == tail) mode = STATE_FLUSH;
	}

	/* copy current tail image */
	if ((mode == STATE_FLUSH) || (count & 0x1) == 1) {
		memcpy(dest, planetable[tail], video_area * PIXEL_SIZE);
		tail++;
		while (tail >= PLANES) tail -= PLANES;

		/* switch mode when tail reaches head */
		if (head == tail) mode = STATE_FILL;
	}

	count++;

	return 0;
}

static int event(SDL_Event *event)
{
	return 0;
}
