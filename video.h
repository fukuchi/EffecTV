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

extern v4ldevice vd;

int video_init(char *file, int channel);
void video_quit();
int video_setformat(int palette);
int video_grabstart();
int video_grabstop();
int video_changesize(int width, int height);

#define video_getformat() (vd.mmap.format)
#define video_getaddress() (v4lgetaddress(&vd))
#define video_syncframe() (v4lsyncf(&vd))
#define video_grabframe() (v4lgrabf(&vd))

#endif /* __VIDEO_H__ */
