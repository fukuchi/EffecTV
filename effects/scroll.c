/*
 * EffecTV - Realtime Digital Video Effector
 * Copyright (C) 2001-2003 FUKUCHI Kentarou
 *
 * BrokenTV - simulate broken VTR.
 * Copyright (C) 2002 Jun IIO
 *
 */

#include <stdlib.h>
#include <string.h>
#include "EffecTV.h"
#include "utils.h"

static int start(void);
static int stop(void);
static int draw(RGB32 *src, RGB32 *dest);

static char *effectname = "BrokenTV";
static int state = 0;

static void add_noise (RGB32 *dest);

#define SCROLL_STEPS	30
static int offset = 0;

effect *scrollRegister()
{
	effect *entry;

	entry = (effect *)malloc(sizeof(effect));
	if(entry == NULL) return NULL;
	
	entry->name = effectname;
	entry->start = start;
	entry->stop = stop;
	entry->draw = draw;
	entry->event = NULL;

	return entry;
}

static int start()
{
	offset = 0;
	state = 1;

	return 0;
}

static int stop()
{
	state = 0;
	return 0;
}

static int draw(RGB32 *src, RGB32 *dest)
{
	memcpy (dest, src+(video_height - offset)*video_width, 
		offset * video_width * sizeof (RGB32));
	memcpy (dest+offset*video_width, src,
		(video_height - offset) * video_width * sizeof (RGB32));
	add_noise (dest);

	offset += SCROLL_STEPS;
	if (offset >= video_height) { offset = 0; }

	return 0;
}

void add_noise (RGB32 *dest)
{
	int i, x, y, dy;

	for (y = offset, dy = 0; ((dy < 3) && (y < video_height)); y++, dy++) {
		i = y * video_width;
		for (x = 0; x < video_width; x++, i++) {
			if ((dy == 2) && (inline_fastrand()>>31)) {
				continue;
			}
			dest[i] = (inline_fastrand()>>31) ? 0xffffff : 0;
		}
	}
}
