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

static normlist normlists[] =
{
	{"ntsc"   , VIDEO_MODE_NTSC},
	{"pal"    , VIDEO_MODE_PAL},
	{"secam"  , VIDEO_MODE_SECAM},
	{"auto"   , VIDEO_MODE_AUTO},
/* following values are supproted by bttv driver. */
	{"pal-nc" , 3},
	{"pal-m"  , 4},
	{"pal-n"  , 5},
	{"ntsc-jp", 6},
	{"", -1}
};

int video_init(char *file, int channel, int norm)
{
	if(file == NULL){
		file = DEFAULT_VIDEO_DEVICE;
	}
	if(v4lopen(file, &vd)) return -1;
	v4lsetdefaultnorm(&vd, norm);
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

/* start the continuous grabbing */
int video_grabstart()
{
	if(v4lgrabstart(&vd, 0) < 0)
		return -1;
	if(v4lgrabstart(&vd, 1) < 0)
		return -1;
	return 0;
}

/* stop the continuous grabbing */
int video_grabstop()
{
	if(v4lsync(&vd, 0) < 0)
		return -1;
	if(v4lsync(&vd, 1) < 0)
		return -1;
	return 0;
}

/* change the size of captured image. When both width and height are 0,
 * the size is set to defautl size. */
int video_changesize(int width, int height)
{
	if(width == 0 || height == 0) {
		width = SCREEN_WIDTH;
		height = SCREEN_HEIGHT;
	}
	return v4lgrabinit(&vd, width, height);
}

/*
 * videox_ series are the utility for using video capturing layer.
 * They don't touch a v4ldevice.
 */

/* returns the norm number described in video4linux drivers. */
int videox_getnorm(char *name)
{
	int i;

	for(i=0; normlists[i].type != -1; i++) {
		if(strcasecmp(name, normlists[i].name) == 0) {
			return normlists[i].type;
		}
	}

	return -1;
}
