/*
 * EffecTV - Realtime Digital Video Effector
 * Copyright (C) 2001-2002 FUKUCHI Kentarou
 *
 * BrokenTV - simulate broken VTR.
 * Copyright (C) 2002 Jun IIO
 *
 */

#include <stdlib.h>
#include <string.h>
#include "../EffecTV.h"
#include "utils.h"

int scrollStart();
int scrollStop();
int scrollDraw();

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
	entry->start = scrollStart;
	entry->stop = scrollStop;
	entry->draw = scrollDraw;
	entry->event = NULL;

	return entry;
}

int scrollStart()
{
	if(video_grabstart())
		return -1;
	state = 1;

	offset = 0;

	return 0;
}

int scrollStop()
{
	if(state) {
		video_grabstop();
		state = 0;
	}

	return 0;
}

int scrollDraw()
{
  	RGB32 *src, *dest;

	if(video_syncframe())
		return -1;
	if(screen_mustlock()) {
		if(screen_lock() < 0) {
			return video_grabframe();
		}
	}
	src = (RGB32 *)video_getaddress();
	if(stretch) {
	  	dest = stretching_buffer;
	} else {
	  	dest = (RGB32 *)screen_getaddress();
	}
	memcpy (dest, src+(video_height - offset)*video_width, 
		offset * video_width * sizeof (RGB32));
	memcpy (dest+offset*video_width, src,
		(video_height - offset) * video_width * sizeof (RGB32));
	add_noise (dest);

	offset += SCROLL_STEPS;
	if (offset >= video_height) { offset = 0; }

	if (stretch) {
	  	image_stretch_to_screen();
	}
	if(screen_mustlock()) {
		screen_unlock();
	}

	if(video_grabframe())
		return -1;

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
