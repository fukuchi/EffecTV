/*
 * v4lutils - utility library for Video4Linux
 * Copyright (C) 2001 FUKUCHI Kentarou
 *
 * v4lutils.h: header file
 *
 */

#ifndef __V4LUTILS_H__
#define __V4LUTILS_H__

#include <sys/types.h>
#include <linux/videodev.h>
#include <pthread.h>

/*
 * Video4Linux Device Structure
 */
struct _v4ldevice
{
	int fd;
	struct video_capability capability;
	struct video_channel channel[10];
	struct video_picture picture;
	struct video_clip clip;
	struct video_window window;
	struct video_capture capture;
	struct video_buffer buffer;
	struct video_mmap mmap;
	struct video_mbuf mbuf;
	struct video_unit unit;
	unsigned char *map;
	pthread_mutex_t mutex;
	int frame;
	int overlay;
};

typedef struct _v4ldevice v4ldevice;

extern int v4lopen(char *, v4ldevice *);
extern int v4lclose(v4ldevice *);
extern int v4lgetcapability(v4ldevice *);
extern int v4lsetdefaultnorm(v4ldevice *, int);
extern int v4lgetsubcapture(v4ldevice *);
extern int v4lsetsubcapture(v4ldevice *, int, int, int, int, int, int);
extern int v4lgetframebuffer(v4ldevice *);
extern int v4lsetframebuffer(v4ldevice *, void *, int, int, int, int);
extern int v4loverlaystart(v4ldevice *);
extern int v4loverlaystop(v4ldevice *);
extern int v4lsetchannel(v4ldevice *, int);
extern int v4lsetchannelnorm(v4ldevice *vd, int, int);
extern int v4lgetpicture(v4ldevice *);
extern int v4lsetpicture(v4ldevice *, int, int, int, int, int);
extern int v4lsetpalette(v4ldevice *, int);
extern int v4lgetmbuf(v4ldevice *);
extern int v4lmmap(v4ldevice *);
extern int v4lmunmap(v4ldevice *);
extern int v4lgrabinit(v4ldevice *, int, int);
extern int v4lgrabstart(v4ldevice *, int);
extern int v4lsync(v4ldevice *, int);
extern int v4llock(v4ldevice *);
extern int v4ltrylock(v4ldevice *);
extern int v4lunlock(v4ldevice *);
extern int v4lsyncf(v4ldevice *);
extern int v4lgrabf(v4ldevice *);
extern unsigned char *v4lgetaddress(v4ldevice *);
extern void v4lprint(v4ldevice *);

#endif /* __V4LUTILS_H__ */
