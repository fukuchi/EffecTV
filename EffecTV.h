/*
 * EffecTV - Realtime Digital Video Effector
 * Copyright (C) 2001 FUKUCHI Kentarou
 *
 * EffecTV.h: common header
 *
 */

#ifndef __EFFECTV_H__
#define __EFFECTV_H__

#include <SDL/SDL.h>
#include "screen.h"
#include "video.h"

#define VERSION_MAJOR 0
#define VERSION_MINOR 1
#define VERSION_PATCH 4
#define VERSION_STRING "0.1.4"

#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 240

#define DEFAULT_VIDEO_DEVICE "/dev/video"
#define DEFAULT_DEPTH 32
#define DEFAULT_PALETTE VIDEO_PALETTE_RGB32
#define DEFAULT_VIDEO_NORM VIDEO_MODE_NTSC

typedef struct _effect
{
	char *name;
	int (*start)(void);
	int (*stop)(void);
	int (*draw)(void);
	int (*event)(SDL_Event *);
} effect;

typedef effect *effectRegistFunc(void);

extern int scale;		/* screen scale */
extern int doublebuf;	/* flag for doublebuffering */
extern int fullscreen;	/* flag for fullscreen mode */
extern int hwsurface;	/* flag for hardware surface */

#endif /* __EFFECTV_H__ */
