/*
 * EffecTV - Realtime Digital Video Effector
 * Copyright (C) 2001-2005 FUKUCHI Kentaro
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

int video_init(char *file, int channel, int norm, int freq, int w, int h, int palette);
void video_quit(void);
int video_setformat(int palette);
int video_grab_check(int palette);
int video_set_grabformat(int palette);
int video_grabstart(void);
int video_grabstop(void);
int video_changesize(int width, int height);
int video_setfreq(int v);
int video_syncframe(void);
int video_grabframe(void);
unsigned char *video_getaddress(void);
void video_change_brightness(int);
void video_change_hue(int);
void video_change_color(int);
void video_change_contrast(int);
void video_change_whiteness(int);

#define video_getformat() (vd.mmap.format)

int videox_getnorm(const char *name);
int videox_getfreq(const char *name);

#endif /* __VIDEO_H__ */
