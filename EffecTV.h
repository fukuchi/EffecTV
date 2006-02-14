/*
 * EffecTV - Realtime Digital Video Effector
 * Copyright (C) 2001-2006 FUKUCHI Kentaro
 *
 * EffecTV.h: common header
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#ifndef __EFFECTV_H__
#define __EFFECTV_H__

#include <SDL/SDL.h>
#include "screen.h"
#include "video.h"

#define VERSION_MAJOR 0
#define VERSION_MINOR 3
#define VERSION_PATCH 10
#define VERSION_STRING "0.3.10"

#define DEFAULT_VIDEO_WIDTH 320
#define DEFAULT_VIDEO_HEIGHT 240

#ifndef DEFAULT_VIDEO_DEVICE
#define DEFAULT_VIDEO_DEVICE "/dev/video0"
#endif
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
	int (*draw)(RGB32 *src, RGB32 *dest);
	int (*event)(SDL_Event *event);
} effect;

typedef effect *effectRegisterFunc(void);

extern int debug;

#endif /* __EFFECTV_H__ */
