/*
 * EffecTV - Realtime Digital Video Effector
 * Copyright (C) 2001-2003 FUKUCHI Kentaro
 *
 * WarholTV - Hommage aux Andy Warhol
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

static char *effectname = "WarholTV";
static int state = 0;
static RGB32 colortable[26] = {
	0x000080, 0x008000, 0x800000,
	0x00e000, 0x808000, 0x800080, 
	0x808080, 0x008080, 0xe0e000, 
};

effect *warholRegister()
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
	state = 1;
	return 0;
}

static int stop()
{
	state = 0;
	return 0;
}

#define DIVIDER		3
static int draw(RGB32 *src, RGB32 *dst)
{
	int p, q, x, y, i;

	for (y = 0; y < video_height; y++)
	  for (x = 0; x < video_width; x++)
	    {
	      p = (x * DIVIDER) % video_width;
	      q = (y * DIVIDER) % video_height;
	      i = ((y * DIVIDER) / video_height) * DIVIDER 
		+ ((x * DIVIDER) / video_width);
	      *dst++ = src[q * video_width + p] ^ colortable[i];
	    }

	return 0;
}
