/*
 * palettecheck: checks all supported pixel formats by the v4l driver.
 * Copyright (C) 2001 FUKUCHI Kentaro
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "v4lutils.h"

char *palette_name[16] = {
	"VIDEO_PALETTE_GREY",
	"VIDEO_PALETTE_HI240",
	"VIDEO_PALETTE_RGB565",
	"VIDEO_PALETTE_RGB24",
	"VIDEO_PALETTE_RGB32",
	"VIDEO_PALETTE_RGB555",
	"VIDEO_PALETTE_YUV422",
	"VIDEO_PALETTE_YUYV",
	"VIDEO_PALETTE_UYVY",
	"VIDEO_PALETTE_YUV420",
	"VIDEO_PALETTE_YUV411",
	"VIDEO_PALETTE_RAW",
	"VIDEO_PALETTE_YUV422P",
	"VIDEO_PALETTE_YUV411P",
	"VIDEO_PALETTE_YUV420P",
	"VIDEO_PALETTE_YUV410P"
};

v4ldevice vd;

int video_grab_check(int palette)
{
	int ret;

	v4lseterrorlevel(V4L_PERROR_NONE);
	if(v4lsetpalette(&vd, palette)) {
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

int video_init(char *file)
{
	int i;

	if(file == NULL){
		file = DEFAULT_VIDEO_DEVICE;
	}
	if(v4lopen(file, &vd)) return -1;
	v4lgetcapability(&vd);

	if(!(vd.capability.type & VID_TYPE_CAPTURE)) {
		fprintf(stderr, "This device seems not to support video capturing.\n");
		v4lclose(&vd);
		return -1;
	}
	if(v4lmmap(&vd)) {
		fprintf(stderr, "This device seems not to support mmap interface.\n");
		v4lclose(&vd);
		return -1;
	}
	if(v4lgrabinit(&vd, vd.capability.minwidth, vd.capability.minheight)) {
		fprintf(stderr, "Failed to initialize the v4l device.\n");
		v4lmunmap(&vd);
		v4lclose(&vd);
		return -1;
	}

	for(i=0; i<16; i++) {
		if(video_grab_check(i+1) == 0) {
			printf("%s\n",palette_name[i]);
		}
	}

	v4lmunmap(&vd);
	v4lclose(&vd);

	return 0;
}

static void usage()
{
	printf("palettecheck: checks all supported pixel formats by the v4l driver.\n");
	printf("Usage: effectv [options...]\n");
	printf("Options:\n");
	printf("  -device FILE     use device FILE for video4linux\n");
}

int main(int argc, char **argv)
{
	char *devfile = NULL;

	if(argc > 1) {
		if(strncmp(argv[1], "-device", 7) == 0) {
			if(argc > 2) {
				devfile = argv[2];
			} else {
				fprintf(stderr, "missing device file.\n");
				exit(1);
			}
		} else {
			usage();
			exit(1);
		}
	}
	if(video_init(devfile)) {
		fprintf(stderr, "Video initialization failed.\n");
		exit(1);
	}
	return 0;
}
