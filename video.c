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
#include "palette.h"
#include "utils.h"

/* Currently there is only one v4l device obeject. */
v4ldevice vd;

/* Is TV tuner enabled? */
int hastuner = 0;

/* Flag for horizontal flipping mode */
int horizontal_flip = 0;

/* Width and height of captured image */
int video_width;
int video_height;
int video_area; // = video_width * video_height

static palette_converter_toRGB32 *converter;
static palette_converter_toRGB32 *converter_hflip;

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
static RGB32 *framebuffer;

#define MAXWIDTH (vd.capability.maxwidth)
#define MAXHEIGHT (vd.capability.maxheight)
#define MINWIDTH (vd.capability.minwidth)
#define MINHEIGHT (vd.capability.minheight)

/* Channel and norm is determined at initialization time. */
int video_init(char *file, int channel, int norm, int freq, int w, int h)
{
	if(file == NULL){
		file = DEFAULT_VIDEO_DEVICE;
	}
	if(v4lopen(file, &vd)) return -1;
	v4lsetdefaultnorm(&vd, norm);
	v4lgetcapability(&vd);

	if(!(vd.capability.type & VID_TYPE_CAPTURE)) {
		fprintf(stderr, "video_init: This device seems not to support video capturing.\n");
		return -1;
	}
	if((vd.capability.type & VID_TYPE_TUNER)) {
		hastuner = 1;
		frequency_table = freq;
		TVchannel = 0;
		video_setfreq(0);
	}
	if(w == 0 && h == 0) {
		w = DEFAULT_VIDEO_WIDTH;
		h = DEFAULT_VIDEO_HEIGHT;
	}
	if(w > MAXWIDTH || h > MAXHEIGHT) {
		w = MAXWIDTH;
		h = MAXHEIGHT;
		fprintf(stderr, "capturing size is set to %dx%d.\n", w, h);
	} else if(w < MINWIDTH || h < MINHEIGHT) {
		w = MINWIDTH;
		h = MINHEIGHT;
		fprintf(stderr, "capturing size is set to %dx%d.\n", w, h);
	}

	video_width = w;
	video_height = h;
	video_area = video_width * video_height;

	framebuffer = (RGB32 *)malloc(video_area*sizeof(RGB32));
	if(framebuffer == NULL) return -1;

	if(v4lmaxchannel(&vd)) {
		if(v4lsetchannel(&vd, channel)) return -1;
	}
	if(v4lmmap(&vd)) return -1;
	if(v4lgrabinit(&vd, video_width, video_height)) return -1;
/* quick hack for v4l driver that does not support double buffer capturing */
	if(vd.mbuf.frames < 2) {
		fprintf(stderr, "video_init: double buffer capturing with mmap is not supported.\n");
		return -1;
	}
	if(video_set_grabformat()) return -1;
	if(converter) {
		if(palette_init()) return -1;
	}

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

/* check supported pixel format */
int video_grab_check(int palette)
{
	int ret;

	v4lseterrorlevel(V4L_PERROR_NONE);
	if(video_setformat(palette)) {
		ret = -1;
		goto EXIT;
	}
	if(v4lgrabstart(&vd, 0) <0) {
		ret = -1;
		goto EXIT;
	}
	ret = v4lsync(&vd, 0);
EXIT:
	v4lseterrorlevel(V4L_PERROR_ALL);
	return ret;
}

int video_set_grabformat()
{
	if(video_grab_check(DEFAULT_PALETTE) == 0) {
		converter = NULL;
		return 0;
	}

	palette_get_supported_converter_toRGB32(&converter, &converter_hflip);
	if(converter == NULL)
		return -1;

	return 0;
}

/* Start the continuous grabbing */
int video_grabstart()
{
	vd.frame = 0;
	if(v4lgrabstart(&vd, 0) < 0)
		return -1;
	if(v4lgrabstart(&vd, 1) < 0)
		return -1;
	return 0;
}

/* Stop the continuous grabbing */
int video_grabstop()
{
	if(vd.framestat[0]) {
		if(v4lsync(&vd, 0) < 0)
			return -1;
	}
	if(vd.framestat[1]) {
		if(v4lsync(&vd, 1) < 0)
			return -1;
	}
	return 0;
}

/* Wait on the capturing image */
int video_syncframe()
{
	return v4lsyncf(&vd);
}

/* Start capturing next image */
int video_grabframe(){
	return v4lgrabf(&vd);
}

/* Returns a pointer to captured image */
unsigned char *video_getaddress()
{
	if(converter) {
		if(horizontal_flip) {
			(*converter_hflip)(v4lgetaddress(&vd), framebuffer, video_width, video_height);
		} else {
			(*converter)(v4lgetaddress(&vd), framebuffer, video_width, video_height);
		}
		return (unsigned char *)framebuffer;
	} else if(horizontal_flip) {
		image_hflip((RGB32 *)v4lgetaddress(&vd), framebuffer,
			video_width, video_height);
		return (unsigned char *)framebuffer;
	} else {
		return v4lgetaddress(&vd);
	}
}

/* Change the size of captured image. When both width and height are 0,
 * the size is set to defautl size. */
int video_changesize(int width, int height)
{
	if(width == 0 || height == 0) {
		width = DEFAULT_VIDEO_WIDTH;
		height = DEFAULT_VIDEO_HEIGHT;
	}
	video_width = width;
	video_height = height;

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
