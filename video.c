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
#include "frequencies.h"

/* Currently there is only one v4l device obeject. */
v4ldevice vd;

/* Is TV tuner enabled? */
int hastuner = 0;

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

static int frequency_table = 0;
static int TVchannel = 0;

/* Channel and norm is determined at initialization time. */
int video_init(char *file, int channel, int norm, int freq)
{
	if(file == NULL){
		file = DEFAULT_VIDEO_DEVICE;
	}
	if(v4lopen(file, &vd)) return -1;
	v4lsetdefaultnorm(&vd, norm);
	v4lgetcapability(&vd);

	if(!(vd.capability.type & VID_TYPE_CAPTURE)) return -1;
	if((vd.capability.type & VID_TYPE_TUNER)) {
		hastuner = 1;
		frequency_table = freq;
		TVchannel = 0;
		video_setfreq(0);
	}

	if(v4lsetchannel(&vd, channel)) return -1;
	if(v4lsetpalette(&vd, DEFAULT_PALETTE)) return -1;
	if(v4lmmap(&vd)) return -1;
	if(v4lgrabinit(&vd, SCREEN_WIDTH, SCREEN_HEIGHT)) return -1;

	atexit(video_quit);
	return 0;
}

/* video_quit() is called automatically when the process terminates.
 * This function is registerd in video_init() by callint atexit(). */
void video_quit()
{
	v4lmunmap(&vd);
	v4lclose(&vd);
}

/* Set the format of captured data. */
int video_setformat(int palette)
{
	return v4lsetpalette(&vd, palette);
}

/* Start the continuous grabbing */
int video_grabstart()
{
	if(v4lgrabstart(&vd, 0) < 0)
		return -1;
	if(v4lgrabstart(&vd, 1) < 0)
		return -1;
	return 0;
}

/* Stop the continuous grabbing */
int video_grabstop()
{
	if(v4lsync(&vd, 0) < 0)
		return -1;
	if(v4lsync(&vd, 1) < 0)
		return -1;
	return 0;
}

/* Change the size of captured image. When both width and height are 0,
 * the size is set to defautl size. */
int video_changesize(int width, int height)
{
	if(width == 0 || height == 0) {
		width = SCREEN_WIDTH;
		height = SCREEN_HEIGHT;
	}
	return v4lgrabinit(&vd, width, height);
}

/* change TVchannel to TVchannel+v */
int video_setfreq(int v)
{
	if(hastuner && (frequency_table >= 0)) {
		TVchannel += v;
		while(TVchannel<0) {
			TVchannel += chanlists[frequency_table].count;
		}
		TVchannel %= chanlists[frequency_table].count;

		return v4lsetfreq(&vd, chanlists[frequency_table].list[TVchannel].freq);
	} else {
		return 0;
	}
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

/* returns the frequency table number. */
int videox_getfreq(char *name)
{
	int i;

	for(i=0; chanlists[i].name; i++) {
		if(strcmp(name, chanlists[i].name) == 0) {
			return i;
		}
	}

	return -1;
}
