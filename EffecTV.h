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
#ifdef VLOOPBACK
#include "vloopback.h"
#endif

#define VERSION_MAJOR 0
#define VERSION_MINOR 3
#define VERSION_PATCH 1
#define VERSION_STRING "0.3.1"

#define DEFAULT_VIDEO_WIDTH 320
#define DEFAULT_VIDEO_HEIGHT 240

#define DEFAULT_VIDEO_DEVICE "/dev/video"
#define DEFAULT_DEPTH 32
#define DEFAULT_PALETTE VIDEO_PALETTE_RGB32
#define DEFAULT_VIDEO_NORM VIDEO_MODE_NTSC

#ifndef SDL_DISABLE
#define SDL_DISABLE 0
#endif
#ifndef SDL_ENABLE
#define SDL_ENABLE 1
#endif

typedef unsigned int RGB32;
#define PIXEL_SIZE (sizeof(RGB32))

typedef struct _effect
{
	char *name;
	int (*start)(void);
	int (*stop)(void);
	int (*draw)(void);
	int (*event)(SDL_Event *);
} effect;

typedef effect *effectRegistFunc(void);

extern int debug;

#endif /* __EFFECTV_H__ */
