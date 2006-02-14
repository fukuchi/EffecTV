/*
 * EffecTV - Realtime Digital Video Effector
 * Copyright (C) 2001-2006 FUKUCHI Kentaro
 *
 * EdgeBlurTV - Get difference, and make blur.
 * Copyright (C) 2005-2006 FUKUCHI Kentaro
 *
 */

#include <stdlib.h>
#include <string.h>
#include "EffecTV.h"
#include "utils.h"

#define MAX_BLUR 31
#define COLORS 4
#define MAGIC_THRESHOLD 16

static int start(void);
static int stop(void);
static int draw(RGB32 *src, RGB32 *dest);
static int event(SDL_Event *event);

static char *effectname = "EdgeBlurTV";
static int state = 0;
static int *blur[2];
static int blurFrame;
static RGB32 *palette;
static RGB32 palettes[(MAX_BLUR + 1) * COLORS];

static void makePalette(void)
{
	int i, v;

	for(i=0; i<=MAX_BLUR; i++) {
		v = 255 * i / MAX_BLUR;
		palettes[i] = v;
		palettes[(MAX_BLUR + 1) + i] = v<<8;
		palettes[(MAX_BLUR + 1) * 2 + i] = v<<16;
		palettes[(MAX_BLUR + 1) * 3 + i] = v * 0x10101;
	}
}

effect *edgeBlurRegister(void)
{
	effect *entry;

	entry = (effect *)malloc(sizeof(effect));
	if(entry == NULL) {
		return NULL;
	}

	sharedbuffer_reset();
	blur[0] = (unsigned int *)sharedbuffer_alloc(video_area * sizeof(int));
	blur[1] = (unsigned int *)sharedbuffer_alloc(video_area * sizeof(int));
	if(blur[0] == NULL || blur[1] == NULL) {
		free(entry);
		return NULL;
	}
	
	entry->name = effectname;
	entry->start = start;
	entry->stop = stop;
	entry->draw = draw;
	entry->event = event;

	makePalette();
	palette = palettes;

	return entry;
}

static int start(void)
{
	memset(blur[0], 0, video_area * sizeof(int));
	memset(blur[1], 0, video_area * sizeof(int));
	blurFrame = 0;

	image_set_threshold_y(MAGIC_THRESHOLD);

	state = 1;
	return 0;
}

static int stop(void)
{
	if(state) {
		state = 0;
	}
	return 0;
}

static int draw(RGB32 *src, RGB32 *dest)
{
	int x, y;
	unsigned char *diff;
	int v, *p, *q;

	diff = image_edge(src);

	p = blur[blurFrame    ] + video_width + 1;
	q = blur[blurFrame ^ 1] + video_width + 1;
	diff += video_width + 1;
	dest += video_width + 1;
	for(y=video_height - 2; y>0; y--) {
		for(x=video_width - 2; x>0; x--) {
			if(*diff++ > 64) {
				v = MAX_BLUR;
			} else {
				v = *(p - video_width) + *(p - 1) + *(p + 1) + *(p + video_width);
				if(v > 3) v -= 3;
				v = v >> 2;
			}
			*dest++ = palette[v];
			*q++ = v;
			p++;

		}
		p += 2;
		q += 2;
		diff += 2;
		dest += 2;
	}

	blurFrame ^= 1;

	return 0;
}

static int event(SDL_Event *event)
{
	if(event->type == SDL_KEYDOWN) {
		switch(event->key.keysym.sym) {
		case SDLK_r:
			palette = &palettes[(MAX_BLUR + 1)*2];
			break;
		case SDLK_g:
			palette = &palettes[(MAX_BLUR + 1)];
			break;
		case SDLK_b:
			palette = &palettes[0];
			break;
		case SDLK_w:
			palette = &palettes[(MAX_BLUR + 1)*3];
			break;
		default:
			break;
		}
	}

	return 0;
}
