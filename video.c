/*
 * EffecTV - Realtime Digital Video Effector
 * Copyright (C) 2001-2006 FUKUCHI Kentaro
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
static v4ldevice vd;

/* Flag for horizontal flipping mode */
int video_horizontalFlip = 0;

/* Width and height of captured image */
int video_width;
int video_height;
int video_area; // = video_width * video_height

/* Video input parameters */
static int video_channel;
static int video_norm;
static int video_palette;

/* Tuner parameters */
int video_hasTuner = 0;
static int video_frequencyTable = 0;
static int video_TVchannel = 0;


/* Initial parameters */
static char *video_file;

/* Picture parameters */
static int picture_brightness;
static int picture_hue;
static int picture_colour;
static int picture_contrast;

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

static RGB32 *framebuffer;

#define MAXWIDTH (vd.capability.maxwidth)
#define MAXHEIGHT (vd.capability.maxheight)
#define MINWIDTH (vd.capability.minwidth)
#define MINHEIGHT (vd.capability.minheight)

/* Channel and norm is determined at initialization time. */
int video_init(char *file, int channel, int norm, int freq, int w, int h, int palette)
{
	if(file == NULL){
		file = DEFAULT_VIDEO_DEVICE;
	}
	if(v4lopen(file, &vd)) return -1;

	video_file    = strdup(file);
	video_channel = channel;
	video_norm    = norm;
	video_palette = palette;

	v4lsetdefaultnorm(&vd, norm);
	v4lgetcapability(&vd);

	if(!(vd.capability.type & VID_TYPE_CAPTURE)) {
		fprintf(stderr, "video_init: This device seems not to support video capturing.\n");
		return -1;
	}
	if((vd.capability.type & VID_TYPE_TUNER)) {
		video_hasTuner = 1;
		video_frequencyTable = freq;
		video_TVchannel = 0;
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
	if(framebuffer == NULL) {
		fprintf(stderr, "video_init: Memory allocation error.\n");
		return -1;
	}

	if(v4lmaxchannel(&vd)) {
		if(v4lsetchannel(&vd, channel)) return -1;
	}
	if(v4lmmap(&vd)) {
		fprintf(stderr, "video_init: mmap interface is not supported by this driver.\n");
		return -1;
	}
	if(v4lgrabinit(&vd, video_width, video_height)) return -1;

/* quick hack for v4l driver that does not support double buffer capturing */
	if(vd.mbuf.frames < 2) {
		fprintf(stderr, "video_init: double buffer capturing with mmap is not supported.\n");
		return -1;
	}
	/* detecting a pixel format supported by the v4l driver.
	 * video_set_grabformat() overwrites both 'converter' and
	 * 'converter_hflip'. */
	if(video_set_grabformat(palette)) {
		fprintf(stderr, "video_init: Can't find a supported pixel format.\n");
		return -1;
	}

	v4lgetpicture(&vd);
	picture_brightness = vd.picture.brightness;
	picture_hue = vd.picture.hue;
	picture_colour = vd.picture.colour;
	picture_contrast = vd.picture.contrast;

	return 0;
}

/* close v4l device. */
void video_quit(void)
{
	v4lmunmap(&vd);
	v4lclose(&vd);
	free(framebuffer);
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

int video_set_grabformat(int palette)
{
	if(palette == 0) {
		if(video_grab_check(DEFAULT_PALETTE) == 0) {
			converter = NULL;
			return 0;
		}
		palette_get_supported_converter_toRGB32(&converter, &converter_hflip);
		if(converter == NULL)
			return -1;
	} else {
		if(palette_check_supported_converter_toRGB32(palette, &converter, &converter_hflip)) {
			return 0;
		} else {
			return -1;
		}
	}

	return 0;
}

/* Start the continuous grabbing */
int video_grabstart(void)
{
	vd.frame = 0;
	if(v4lgrabstart(&vd, 0) < 0)
		return -1;
	if(v4lgrabstart(&vd, 1) < 0)
		return -1;
	return 0;
}

/* Stop the continuous grabbing */
int video_grabstop(void)
{
	if(vd.framestat[vd.frame]) {
		if(v4lsync(&vd, vd.frame) < 0)
			return -1;
	}
	if(vd.framestat[vd.frame ^ 1]) {
		if(v4lsync(&vd, vd.frame ^ 1) < 0)
			return -1;
	}
	return 0;
}

/* Wait on the capturing image */
int video_syncframe(void)
{
	return v4lsyncf(&vd);
}

/* Start capturing next image */
int video_grabframe(void){
	return v4lgrabf(&vd);
}

/* Returns a pointer to captured image */
unsigned char *video_getaddress(void)
{
	if(converter) {
		if(video_horizontalFlip) {
			(*converter_hflip)(v4lgetaddress(&vd), framebuffer, video_width, video_height);
		} else {
			(*converter)(v4lgetaddress(&vd), framebuffer, video_width, video_height);
		}
		return (unsigned char *)framebuffer;
	} else if(video_horizontalFlip) {
		image_hflip((RGB32 *)v4lgetaddress(&vd), framebuffer,
			video_width, video_height);
		return (unsigned char *)framebuffer;
	} else {
		return v4lgetaddress(&vd);
	}
}

/* Change the size of captured image. When both width and height are 0,
 * the size is set to default size. */
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

/* change video_TVchannel to video_TVchannel+v */
int video_setfreq(int v)
{
	if(video_hasTuner && (video_frequencyTable >= 0)) {
		video_TVchannel += v;
		while(video_TVchannel<0) {
			video_TVchannel += chanlists[video_frequencyTable].count;
		}
		video_TVchannel %= chanlists[video_frequencyTable].count;

		return v4lsetfreq(&vd, chanlists[video_frequencyTable].list[video_TVchannel].freq);
	} else {
		return 0;
	}
}

/* increase brightness value with v */
void video_change_brightness(int v)
{
	picture_brightness += v;
	if(picture_brightness < 0) picture_brightness = 0;
	if(picture_brightness > 65535) picture_brightness = 65535;
	v4lsetpicture(&vd, picture_brightness, -1, -1, -1, -1);
}

/* increase hue value with v */
void video_change_hue(int v)
{
	picture_hue += v;
	if(picture_hue < 0) picture_hue = 0;
	if(picture_hue > 65535) picture_hue = 65535;
	v4lsetpicture(&vd, -1, picture_hue, -1, -1, -1);
}

/* increase color value with v */
void video_change_color(int v)
{
	picture_colour += v;
	if(picture_colour < 0) picture_colour = 0;
	if(picture_colour > 65535) picture_colour = 65535;
	v4lsetpicture(&vd, -1, -1, picture_colour, -1, -1);
}

/* increase contrast value with v */
void video_change_contrast(int v)
{
	picture_contrast += v;
	if(picture_contrast < 0) picture_contrast = 0;
	if(picture_contrast > 65535) picture_contrast = 65535;
	v4lsetpicture(&vd, -1, -1, -1, picture_contrast, -1);
}

/* change channel: stops video grabbing at first, then recreate v4ldevice
 * object. */
int video_change_channel(int channel)
{
	int ret = 0;
	int maxch;

	maxch = v4lmaxchannel(&vd);
	if(channel < 0) channel = 0;
	if(channel > maxch) channel = maxch;

	video_grabstop();
	if(v4lsetchannel(&vd, channel)) ret = -1;
	video_channel = channel;
	video_grabstart();

	return ret;
}

/* retry grabbing */
int video_retry(void)
{
	video_quit();
	return video_init(
			video_file,
			video_channel,
			video_norm,
			video_frequencyTable,
			video_width,
			video_height,
			video_palette);
}

/*
 * videox_ series are the utility for using video capturing layer.
 * They don't touch a v4ldevice.
 */

/* returns the norm number described in video4linux drivers. */
int videox_getnorm(const char *name)
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
int videox_getfreq(const char *name)
{
	int i;

	for(i=0; chanlists[i].name; i++) {
		if(strcmp(name, chanlists[i].name) == 0) {
			return i;
		}
	}

	return -1;
}
