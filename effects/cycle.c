/*
 * EffecTV - Realtime Digital Video Effector
 * Copyright (C) 2001-2002 FUKUCHI Kentaro
 *
 * cycleTV - no effect.
 * Written by clifford smith <nullset@dookie.net>
 *
 */

#include <stdlib.h>
#include <string.h>
#include "../EffecTV.h"
#include "utils.h"

int cycleStart();
int cycleStop();
int cycleDraw();

static char *effectname = "cycleTV";
static int state = 0;
int roff,goff,boff; /* Offset values */
effect *cycleRegister()
{
	effect *entry;

	entry = (effect *)malloc(sizeof(effect));
	if(entry == NULL) return NULL;
	
	entry->name = effectname;
	entry->start = cycleStart;
	entry->stop = cycleStop;
	entry->draw = cycleDraw;
	entry->event = NULL;

	return entry;
}

int cycleStart()
{
  roff = goff = boff = 0;
	if(video_grabstart())
		return -1;
	state = 1;
	return 0;
}

int cycleStop()
{
	if(state) {
		video_grabstop();
		state = 0;
	}

	return 0;
}

#define NEWCOLOR(c,o) ((c+o)%230)
int cycleDraw()
{
  int i;
  RGB32 *src,*dst;
  
  src = (RGB32 *)video_getaddress();
  if (stretch) {
    dst = stretching_buffer;
  } else {
    dst = (RGB32 *)screen_getaddress();
  }
  
  roff += 1;
  goff += 3;        
  if (stretch)
    image_stretch_to_screen();
  
  boff += 7;
  if(video_syncframe())
    return -1;
  if(screen_mustlock()) {
    if(screen_lock() < 0) {
      return video_grabframe();
    }
  }
  for (i=0 ; i < video_area ; i++) {
    RGB32 t;
    t = *src++;
    *dst++ = RGB(NEWCOLOR(RED(t),roff),
	       NEWCOLOR(GREEN(t),goff),
	       NEWCOLOR(BLUE(t),boff));
  }

  
  if(screen_mustlock()) {
    screen_unlock();
  }
  
  if(video_grabframe())
    return -1;
  
  return 0;
}

