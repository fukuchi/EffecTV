/*
 * EffecTV - Realtime Digital Video Effector
 * Copyright (C) 2001 FUKUCHI Kentarou
 *
 * vloopback.c: vloopback device manager
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#include "EffecTV.h"

int vloopback = 0;

static int outputfd;
static int frame_length;
static unsigned char *buf;

int vloopback_init(char *name)
{
	struct video_window vid_win;
	struct video_picture vid_pic;

	outputfd = open(name, O_RDWR);
	if(outputfd < 0) {
		fprintf(stderr, "Couldn't open output device file %s\n",name);
		return -1;
	}
	if(ioctl(outputfd, VIDIOCGPICT, &vid_pic) == -1) {
		perror("vloopback_init:VIDIOCGPICT");
		return -1;
	}
	vid_pic.palette = VIDEO_PALETTE_RGB24;
	if(ioctl(outputfd, VIDIOCSPICT, &vid_pic) == -1) {
		perror("vloopback_init:VIDIOCSPICT");
		return -1;
	}
	if(ioctl(outputfd, VIDIOCGWIN, &vid_win) == -1) {
		perror("vloopback_init:VIDIOCGWIN");
		return -1;
	}
	vid_win.width = SCREEN_WIDTH * scale;
	vid_win.height = SCREEN_HEIGHT * scale;
	if(ioctl(outputfd, VIDIOCSWIN, &vid_win) == -1) {
		perror("vloopback_init:VIDIOCSWIN");
		return -1;
	}
	frame_length = SCREEN_AREA * scale * scale * 3;
	buf = (unsigned char *)malloc(frame_length);
	if(buf == NULL) return -1;

	printf("video pipelining is OK.\n");
	atexit(vloopback_quit);
	return 0;
}

/* vloopback_quit() is called automatically when the process terminates.
 * This function is registerd in vloopback_init() by callint atexit(). */
void vloopback_quit()
{
	close(outputfd);
	printf("video pipelining is stopped.\n");
}

int vloopback_push()
{
	int i;
	unsigned char *src;

	src = (unsigned char *)screen_getaddress();
	for(i=0; i < SCREEN_AREA * scale * scale; i++) {
		buf[i*3] = src[i*4];
		buf[i*3+1] = src[i*4+1];
		buf[i*3+2] = src[i*4+2];
	}
	if(write(outputfd, buf, frame_length) != frame_length) {
		perror("vloopback_push: write");
	}
	return 0;
}
