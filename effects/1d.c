/*
 * EffecTV - Realtime Digital Video Effector
 * Copyright (C) 2001 FUKUCHI Kentarou
 *
 * 1d.c: scan line
 *
 */

#include <stdlib.h>
#include <string.h>
#include "../EffecTV.h"
#include "utils.h"

int onedStart();
int onedStop();
int onedDraw();

static char *effectname = "1DTV";
static int format;
static int linelength;
static int lines;
static int line;
static int prevline;
static unsigned char *linebuf;

effect *onedRegister()
{
	effect *entry;

	linelength = SCREEN_WIDTH*4*scale;
	lines = SCREEN_HEIGHT*scale;

	sharedbuffer_reset();
	linebuf = (unsigned char *)sharedbuffer_alloc(linelength);
	if(linebuf == NULL)
		return NULL;

	entry = (effect *)malloc(sizeof(effect));
	if(entry == NULL)
		return NULL;
	
	entry->name = effectname;
	entry->start = onedStart;
	entry->stop = onedStop;
	entry->draw = onedDraw;
	entry->event = NULL;

	return entry;
}

int onedStart()
{
	screen_clear(0);
	bzero(linebuf, linelength);
	line = 0;
	prevline = 0;
	format = video_getformat();
	video_setformat(VIDEO_PALETTE_RGB32);
	if(scale == 2){
		video_changesize(SCREEN_WIDTH*2, SCREEN_HEIGHT*2);
	}
	video_grabstart();

	return 0;
}

int onedStop()
{
	video_grabstop();
	video_setformat(format);
	if(scale == 2){
		video_changesize(0, 0);
	}

	return 0;
}

int onedDraw()
{
	unsigned int *src, *dest;
	int i;

	video_syncframe();

	if(screen_mustlock()) {
		if(screen_lock() < 0) {
			video_grabframe();
			return 0;
		}
	}
	src = (unsigned int *)(video_getaddress()+line*linelength);
	if(doublebuf) {
		dest = (unsigned int *)(screen_getaddress()+prevline*linelength);
		bcopy(linebuf, dest, linelength);
	}
	dest = (unsigned int *)(screen_getaddress()+line*linelength);
	bcopy(src, dest, linelength);
	if(doublebuf) {
		bcopy(src, linebuf, linelength);
		prevline = line;
	}
	line++;
	if(line >= lines)
		line = 0;
	dest = (unsigned int *)(screen_getaddress()+line*linelength);
	for(i=0; i<linelength/4; i++) {
		dest[i] = 0xff00;
	}

	if(screen_mustlock()) {
		screen_unlock();
	}
	video_grabframe();
	screen_update();

	return 0;
}
