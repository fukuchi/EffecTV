/*
 * EffecTV - Realtime Digital Video Effector
 * Copyright (C) 2001 FUKUCHI Kentarou
 *
 * video.h: header for video manager
 *
 */

#ifndef __VIDEO_H__
#define __VIDEO_H__

#include <v4lutils.h>

typedef struct _normlist
{
	char name[10];
	int type;
} normlist;

extern v4ldevice vd;
extern int hastuner;
extern int horizontal_flip;
extern int video_width;
extern int video_height;
extern int video_area;

int video_init(char *file, int channel, int norm, int freq, int w, int h);
void video_quit();
int video_setformat(int palette);
int video_set_grabformat();
int video_grabstart();
int video_grabstop();
int video_changesize(int width, int height);
int video_setfreq(int v);
int video_syncframe();
int video_grabframe();
unsigned char *video_getaddress();

#define video_getformat() (vd.mmap.format)

int videox_getnorm(char *name);
int videox_getfreq(char *name);

#endif /* __VIDEO_H__ */
