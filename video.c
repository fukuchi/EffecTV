/*
 * EffecTV - Realtime Digital Video Effector
 * Copyright (C) 2001 FUKUCHI Kentarou
 *
 * video.c: video manager
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <v4lutils.h>

#include "EffecTV.h"

v4ldevice vd;

int video_init(char *file, int channel)
{
	if(file == NULL){
		file = DEFAULT_VIDEO_DEVICE;
	}
	if(v4lopen(file, &vd)) return -1;
	v4lsetdefaultnorm(&vd, DEFAULT_VIDEO_NORM);
	v4lgetcapability(&vd);

	if(v4lsetchannel(&vd, channel)) return -1;
	if(v4lsetpalette(&vd, DEFAULT_PALETTE)) return -1;
	if(v4lmmap(&vd)) return -1;
	if(v4lgrabinit(&vd, SCREEN_WIDTH, SCREEN_HEIGHT)) return -1;

	atexit(video_quit);
	return 0;
}

void video_quit()
{
	v4lmunmap(&vd);
	v4lclose(&vd);
}

int video_setformat(int palette)
{
	return v4lsetpalette(&vd, palette);
}

int video_grabstart()
{
	if(v4lgrabstart(&vd, 0) < 0)
		return -1;
	if(v4lgrabstart(&vd, 1) < 0)
		return -1;
	return 0;
}

int video_grabstop()
{
	if(v4lsync(&vd, 0) < 0)
		return -1;
	if(v4lsync(&vd, 1) < 0)
		return -1;
	return 0;
}

int video_changesize(int width, int height)
{
	if(width == 0 || height == 0) {
		width = SCREEN_WIDTH;
		height = SCREEN_HEIGHT;
	}
	return v4lgrabinit(&vd, width, height);
}
