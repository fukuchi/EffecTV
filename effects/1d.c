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

int onedStart();
int onedStop();
int onedDraw();

static char *effectname = "1DTV";
static int format;
static int linelength;
static int lines;
static int line;

effect *onedRegister()
{
	effect *entry;

	entry = (effect *)malloc(sizeof(effect));
	if(entry == NULL) return NULL;
	
	entry->name = effectname;
	entry->start = onedStart;
	entry->stop = onedStop;
	entry->draw = onedDraw;
	entry->event = NULL;
	if(scale == 2) {
		linelength = SCREEN_WIDTH*2*4;
		lines = SCREEN_HEIGHT*2;
	} else {
		linelength = SCREEN_WIDTH*4;
		lines = SCREEN_HEIGHT;
	}

	return entry;
}

int onedStart()
{
	line = 0;
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
	dest = (unsigned int *)(screen_getaddress()+line*linelength);
	bcopy(src, dest, linelength);
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
