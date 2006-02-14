/*
 * EffecTV - Realtime Digital Video Effector
 * Copyright (C) 2001-2006 FUKUCHI Kentaro
 *
 * DumbTV - no effect.
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

static char *effectname = "DumbTV";
static int state = 0;

effect *dumbRegister(void)
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

static int start(void)
{
	state = 1;

	return 0;
}

static int stop(void)
{
	state = 0;

	return 0;
}

static int draw(RGB32 *src, RGB32 *dest)
{
	memcpy(dest, src, video_area * sizeof(RGB32));

	return 0;
}
