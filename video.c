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


/* Initial parameters */
static char *video_file;

/* Picture parameters */
static int picture_brightness;
static int picture_hue;
static int picture_colour;
static int picture_contrast;

static RGB32 *framebuffer;

static void convert_BGR24toBGR32
(unsigned char *src, RGB32 *dest, int width, int height)
{
	int i, length;

	length = width * height;
	for(i=0; i<length; i++) {
		*dest++ = *(unsigned int *)src & 0xffffff;
		src += 3;
	}
}

static void convert_BGR24toBGR32_hflip
(unsigned char *src, RGB32 *dest, int width, int height)
{
	int x, y;

	dest += width - 1;
	for(y=0; y<height; y++) {
		for(x=0; x<width; x++) {
			*dest-- = *(unsigned int *)src & 0xffffff;
			src += 3;
		}
		dest += width * 2;
	}
}

/* Channel and norm is determined at initialization time. */
int video_init(char *file, int channel, int w, int h)
{
	if(file == NULL){
		file = DEFAULT_VIDEO_DEVICE;
	}
	if(v4lopen(file, &vd)) return -1;

	video_file    = strdup(file);
	video_channel = channel;

	v4lgetcapability(&vd);

	if(w == 0 && h == 0) {
		w = DEFAULT_VIDEO_WIDTH;
		h = DEFAULT_VIDEO_HEIGHT;
	}

	video_width = w;
	video_height = h;
	video_area = video_width * video_height;

	framebuffer = (RGB32 *)malloc(video_area*sizeof(RGB32));
	if(framebuffer == NULL) {
		fprintf(stderr, "video_init: Memory allocation error.\n");
		return -1;
	}

	if(v4lgrabinit(&vd, video_width, video_height)) return -1;

	video_width = vd.fmt.fmt.pix.width;
	video_height = vd.fmt.fmt.pix.height;

	/*
	v4lgetpicture(&vd);
	picture_brightness = vd.picture.brightness;
	picture_hue = vd.picture.hue;
	picture_colour = vd.picture.colour;
	picture_contrast = vd.picture.contrast;
	*/

	return 0;
}

/* close v4l device. */
void video_quit(void)
{
	v4lgrabstop(&vd);
	v4lclose(&vd);
	free(framebuffer);
}

/* Start the continuous grabbing */
int video_grabstart(void)
{
	return v4lgrabstart(&vd);
}

/* Stop the continuous grabbing */
int video_grabstop(void)
{
	return v4lgrabstop(&vd);
}

/* Wait on the capturing image */
int video_syncframe(void)
{
	return v4lsync(&vd);
}

/* Start capturing next image */
int video_grabframe(void){
	return v4lnext(&vd);
}

/* Returns a pointer to captured image */
unsigned char *video_getaddress(void)
{
	if(video_horizontalFlip) {
		convert_BGR24toBGR32_hflip(v4lgetaddress(&vd), framebuffer, video_width, video_height);
	} else {
		convert_BGR24toBGR32(v4lgetaddress(&vd), framebuffer, video_width, video_height);
	}
	return (unsigned char *)framebuffer;
}

#if 0
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
#endif

/* retry grabbing */
int video_retry(void)
{
	video_quit();
	return video_init(
			video_file,
			video_channel,
			video_width,
			video_height);
}
