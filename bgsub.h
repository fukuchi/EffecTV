/*
 * EffecTV - Realtime Digital Video Effector
 * Copyright (C) 2021 FUKUCHI Kentaro
 *
 * bgsub.h: background subtraction module
 *
 */

#ifndef __BGSUB_H__
#define __BGSUB_H__

typedef struct {
	int w, h;
	int y_threshold;
	RGB32 rgb_threshold;
	RGB32 *background;
	unsigned char *diff;
} BgSubtractor;

BgSubtractor *bgsub_new(int w, int h);
void bgsub_free(BgSubtractor *bgsub);

void bgsub_set_threshold_y(BgSubtractor *bgsub, int threshold);
void bgsub_bgset_y(BgSubtractor *bgsub, RGB32 *src);
unsigned char *bgsub_subtract_y(BgSubtractor *bgsub, RGB32 *src);
unsigned char *bgsub_subtract_update_y(BgSubtractor *bgsub, RGB32 *src);

void bgsub_set_threshold_RGB(BgSubtractor *bgsub, int r, int g, int b);
void bgsub_bgset_RGB(BgSubtractor *bgsub, RGB32 *src);
unsigned char *bgsub_subtract_RGB(BgSubtractor *bgsub, RGB32 *src);
unsigned char *bgsub_subtract_update_RGB(BgSubtractor *bgsub, RGB32 *src);

unsigned char *bgsub_denoise(BgSubtractor *bgsub);

#endif /* __BGSUB_H__ */


