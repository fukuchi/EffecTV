/*
 * EffecTV - Realtime Digital Video Effector
 * Copyright (C) 2001 FUKUCHI Kentarou
 *
 * image.c: utilities for image processing.
 *
 */

#include "../EffecTV.h"
#include "utils.h"

void image_stretch(unsigned int *src, unsigned int *dest)
{
	int x, y;

	for(y=0; y<SCREEN_HEIGHT; y++) {
		for(x=0; x<SCREEN_WIDTH; x++) {
			dest[0] = *src;
			dest[1] = *src;
			dest[SCREEN_WIDTH*2] = *src;
			dest[SCREEN_WIDTH*2+1] = *src;
			src++;
			dest += 2;
		}
		dest += SCREEN_WIDTH*2;
	}
}
