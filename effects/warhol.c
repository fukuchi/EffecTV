/*
 * EffecTV - Realtime Digital Video Effector
 * Copyright (C) 2001 FUKUCHI Kentarou
 *
 * WarholTV - Hommage aux Andy Warhol
 * Copyright (C) 2002 Jun IIO
 *
 */

#include <stdlib.h>
#include <string.h>
#include "../EffecTV.h"
#include "utils.h"

int warholStart();
int warholStop();
int warholDraw();

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
	entry->start = warholStart;
	entry->stop = warholStop;
	entry->draw = warholDraw;
	entry->event = NULL;

	return entry;
}

int warholStart()
{
	if(video_grabstart())
		return -1;
	state = 1;
	return 0;
}

int warholStop()
{
	if(state) {
		video_grabstop();
		state = 0;
	}

	return 0;
}

#define DIVIDER		3
int warholDraw()
{
	int p, q, x, y, i;
  	RGB32 *src, *dst;

	if(video_syncframe())
	  return -1;
	if(screen_mustlock()) {
	  if(screen_lock() < 0) {
	    return video_grabframe();
	  }
	}
	src = (RGB32 *)video_getaddress();
	dst = (stretch) ? stretching_buffer : (RGB32 *)screen_getaddress();
	for (y = 0; y < video_height; y++)
	  for (x = 0; x < video_width; x++)
	    {
	      p = (x * DIVIDER) % video_width;
	      q = (y * DIVIDER) % video_height;
	      i = ((y * DIVIDER) / video_height) * DIVIDER 
		+ ((x * DIVIDER) / video_width);
	      *dst++ = src[q * video_width + p] ^ colortable[i];
	    }

	if(stretch) {
		image_stretch_to_screen();
	}
	if(screen_mustlock()) {
	  screen_unlock();
	}
	
	if(video_grabframe())
	  return -1;
	
	return 0;
}
