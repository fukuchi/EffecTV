/*
 * EffecTV - Realtime Digital Video Effector
 * Copyright (C) 2001-2002 FUKUCHI Kentaro
 *
 * 1DTV - scans line by line and generates amazing still image.
 * Copyright (C) 2001-2002 FUKUCHI Kentaro
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
static int state = 0;
static int line;
static int prevline;
static int sline;
static int sheight;
static int prevsline;
static int prevsheight;
static RGB32 *linebuf;

static void setparams()
{
	int snext;

	sline = line * screen_width / video_width;
	snext = (line + 1) * screen_width / video_width;
	sheight = snext - sline;
}

effect *onedRegister()
{
	effect *entry;

	sharedbuffer_reset();
	linebuf = (RGB32 *)sharedbuffer_alloc(screen_width * sizeof(RGB32));
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
	bzero(linebuf, screen_width*sizeof(RGB32));
	line = 0;
	setparams();
	prevline = 0;
	prevsline = 0;
	prevsheight = 0;
	if(video_grabstart())
		return -1;
	state = 1;
	return 0;
}

int onedStop()
{
	if(state) {
		video_grabstop();
		state = 0;
	}

	return 0;
}

static void blitline()
{
	int i, x, t;
	RGB32 *src, *dest, *p;

	src = (RGB32 *)video_getaddress() + video_width * line;
	dest = (RGB32 *)screen_getaddress() + screen_width * sline;
	for(i=0; i<sheight; i++) {
		p = src;
		t = screen_width;
		for(x=0; x<screen_width; x++) {
			*dest++ = *p;
			t -= video_width;
			if(t<=0) {
				t += screen_width;
				p++;
			}
		}
	}
}

static void blitline_buf()
{
	int i, x, t;
	RGB32 *src, *dest, *p;

	src = (RGB32 *)video_getaddress() + video_width * line;
	dest = (RGB32 *)screen_getaddress() + screen_width * sline;
	p = src;
	t = screen_width;
	for(x=0; x<screen_width; x++) {
		*dest++ = *p;
		linebuf[x] = *p;
		t -= video_width;
		if(t<=0) {
			t += screen_width;
			p++;
		}
	}
	for(i=1; i<sheight; i++) {
		p = src;
		t = screen_width;
		for(x=0; x<screen_width; x++) {
			*dest++ = *p;
			t -= video_width;
			if(t<=0) {
				t += screen_width;
				p++;
			}
		}
	}
}

int onedDraw()
{
	RGB32 *dest;
	int i;


	if(video_syncframe())
		return -1;
	if(screen_mustlock()) {
		if(screen_lock() < 0) {
			return video_grabframe();
		}
	}

	if(doublebuf) {
		dest = (RGB32 *)screen_getaddress() + screen_width * prevsline;
		for(i=0; i<prevsheight; i++) {
			memcpy(dest, linebuf, screen_width*sizeof(RGB32));
			dest += screen_width;
		}
	}
	if(doublebuf) {
		blitline_buf();
		prevline = line;
		prevsline = sline;
		prevsheight = sheight;
	} else {
		blitline();
	}
	line++;
	if(line >= video_height)
		line = 0;
	setparams();
	dest = (RGB32 *)screen_getaddress() + screen_width * sline;
	for(i=0; i<screen_width; i++) {
		dest[i] = 0xff00;
	}
	if(screen_mustlock()) {
		screen_unlock();
	}
	if(video_grabframe())
		return -1;

	return 0;
}
