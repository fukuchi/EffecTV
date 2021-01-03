/*
 * EffecTV - Realtime Digital Video Effector
 * Copyright (C) 2001-2006 FUKUCHI Kentaro
 *
 * 1DTV - scans line by line and generates amazing still image.
 * Copyright (C) 2001-2002 FUKUCHI Kentaro
 *
 */

#include <stdlib.h>
#include <string.h>
#include "EffecTV.h"
#include "utils.h"

static int start(void);
static int stop(void);
static int draw(RGB32 *src, RGB32 *dest);

static char *effectname = "1DTV";
static int state = 0;
static int line;
static int prevline;
static RGB32 *linebuf;

effect *onedRegister(void)
{
	effect *entry;

	linebuf = (RGB32 *)malloc(video_width * PIXEL_SIZE);
	if(linebuf == NULL)
		return NULL;

	entry = (effect *)malloc(sizeof(effect));
	if(entry == NULL)
		return NULL;
	
	entry->name = effectname;
	entry->start = start;
	entry->stop = stop;
	entry->draw = draw;
	entry->event = NULL;

	return entry;
}

static int start(void)
{
	memset(linebuf, 0, video_width * PIXEL_SIZE);
	line = 0;
	prevline = 0;

	state = 1;

	return 0;
}

static int stop(void)
{
	state = 0;

	return 0;
}

static void blitline(RGB32 *src, RGB32 *dest)
{
	src += video_width * line;
	dest += video_width * line;
	memcpy(dest, src, PIXEL_SIZE * video_width);
}

static int draw(RGB32 *src, RGB32 *dest)
{
	int i;

	blitline(src, dest);

	line++;
	if(line >= video_height)
		line = 0;

	dest += video_width * line;
	for(i=0; i<video_width; i++) {
		dest[i] = 0xff00;
	}

	return 0;
}
