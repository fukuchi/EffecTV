/*
 * EffecTV - Realtime Digital Video Effector
 * Copyright (C) 2001-2002 FUKUCHI Kentaro
 *
 * effect.h: common header for effects
 *
 */

#ifndef __EFFECT_H__
#define __EFFECT_H__

#include <SDL/SDL.h>
#include "screen.h"
#include "video.h"

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

#endif /* __EFFECT_H__ */
