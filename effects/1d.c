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
int onedDrawDouble();

static char *effectname = "1DTV";
static int state = 0;
static int linelength;
static int lines;
static int line;
static int prevline;
static unsigned int *linebuf;

effect *onedRegister()
{
	effect *entry;

	if(hireso) {
		linelength = SCREEN_WIDTH*PIXEL_SIZE*scale;
		lines = SCREEN_HEIGHT*scale;
	} else {
		linelength = SCREEN_WIDTH*PIXEL_SIZE;
		lines = SCREEN_HEIGHT;
	}

	sharedbuffer_reset();
	linebuf = (unsigned int *)sharedbuffer_alloc(linelength);
	if(linebuf == NULL)
		return NULL;

	entry = (effect *)malloc(sizeof(effect));
	if(entry == NULL)
		return NULL;
	
	entry->name = effectname;
	entry->start = onedStart;
	entry->stop = onedStop;
	if(stretch) {
		entry->draw = onedDrawDouble;
	} else {
		entry->draw = onedDraw;
	}
	entry->event = NULL;

	return entry;
}

int onedStart()
{
	screen_clear(0);
	bzero(linebuf, linelength);
	line = 0;
	prevline = 0;
	if(hireso) {
		if(video_changesize(SCREEN_WIDTH*2, SCREEN_HEIGHT*2))
			return -1;
	}
	if(video_grabstart())
		return -1;
	state = 1;
	return 0;
}

int onedStop()
{
	if(state) {
		video_grabstop();
		if(hireso){
			video_changesize(0, 0);
		}
		state = 0;
	}

	return 0;
}

int onedDraw()
{
	unsigned int *src, *dest;
	int i;

	if(video_syncframe())
		return -1;

	if(screen_mustlock()) {
		if(screen_lock() < 0) {
			return video_grabframe();
		}
	}
	if(doublebuf) {
		dest = (unsigned int *)(screen_getaddress()+prevline*linelength);
		bcopy(linebuf, dest, linelength);
	}
	src = (unsigned int *)(video_getaddress()+line*linelength);
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
	if(video_grabframe())
		return -1;

	return 0;
}

int onedDrawDouble()
{
	unsigned int *src, *dest;
	int x;

	if(video_syncframe())
		return -1;

	if(screen_mustlock()) {
		if(screen_lock() < 0) {
			return video_grabframe();
		}
	}

	if(doublebuf) {
		dest = (unsigned int *)(screen_getaddress()+prevline*2*SCREEN_WIDTH*2*PIXEL_SIZE);
		for(x=0; x<SCREEN_WIDTH; x++) {
			dest[0] = linebuf[x];
			dest[1] = linebuf[x];
			dest[SCREEN_WIDTH*2] = linebuf[x];
			dest[SCREEN_WIDTH*2+1] = linebuf[x];
			dest += 2;
		}
	}
	src = (unsigned int *)(video_getaddress()+line*SCREEN_WIDTH*PIXEL_SIZE);
	dest = (unsigned int *)(screen_getaddress()+line*2*SCREEN_WIDTH*2*PIXEL_SIZE);
	for(x=0; x<SCREEN_WIDTH; x++) {
		dest[0] = src[x];
		dest[1] = src[x];
		dest[SCREEN_WIDTH*2] = src[x];
		dest[SCREEN_WIDTH*2+1] = src[x];
		dest += 2;
	}
	if(doublebuf) {
		bcopy(src, linebuf, linelength);
		prevline = line;
	}
	line++;
	if(line >= lines)
		line = 0;
	dest = (unsigned int *)(screen_getaddress()+line*2*SCREEN_WIDTH*2*PIXEL_SIZE);
	for(x=0; x<SCREEN_WIDTH*4; x++) {
		dest[x] = 0xff00;
	}
	if(screen_mustlock()) {
		screen_unlock();
	}
	if(video_grabframe())
		return -1;

	return 0;
}
