/*
 * EffecTV - Realtime Digital Video Effector
 * Copyright (C) 2021 FUKUCHI Kentaro
 *
 * bgsub.c: background subtraction module
 *
 */

#include <string.h>
#include <stdlib.h>
#include "EffecTV.h"
#include "bgsub.h"
#include "utils.h"

BgSubtractor *bgsub_new(int w, int h)
{
	BgSubtractor *entry;
	const int area = w * h;

	entry = (BgSubtractor *)malloc(sizeof(BgSubtractor));
	if(entry == NULL) return NULL;

	entry->background = NULL;
	entry->diff = NULL;
	entry->w = w;
	entry->h = h;
	entry->background = (RGB32 *)calloc(area, sizeof(RGB32));
	entry->diff = (unsigned char *)calloc(area, sizeof(unsigned char));
	if(entry->background == NULL || entry->diff == NULL) goto EXIT;

	return entry;

EXIT:
	if(entry->background != NULL) free(entry->background);
	if(entry->diff != NULL) free(entry->diff);
	free(entry);

	return NULL;
}

void bgsub_free(BgSubtractor *bgsub)
{
	free(bgsub->background);
	free(bgsub->diff);
	free(bgsub);
}

void bgsub_set_threshold_y(BgSubtractor *bgsub, int threshold)
{
	bgsub->y_threshold = threshold * 7; /* fake-Y value is timed by 7 */
}

void bgsub_bgset_y(BgSubtractor *bgsub, RGB32 *src)
{
	int i;
	int R, G, B;
	RGB32 *p;
	short *q;
	const int area = bgsub->w * bgsub->h;

	p = src;
	q = (short *)bgsub->background;
	for(i=0; i<area; i++) {
		R = ((*p)&0xff0000)>>(16-1);
		G = ((*p)&0xff00)>>(8-2);
		B = (*p)&0xff;
		*q = (short)(R + G + B);
		p++;
		q++;
	}
}

unsigned char *bgsub_subtract_y(BgSubtractor *bgsub, RGB32 *src)
{
	int i;
	int R, G, B;
	RGB32 *p;
	short *q;
	unsigned char *r;
	int v;
	const int area = bgsub->w * bgsub->h;

	p = src;
	q = (short *)bgsub->background;
	r = bgsub->diff;
	for(i=0; i<area; i++) {
		R = ((*p)&0xff0000)>>(16-1);
		G = ((*p)&0xff00)>>(8-2);
		B = (*p)&0xff;
		v = (R + G + B) - (int)(*q);
		*r = ((v + bgsub->y_threshold)>>24) | ((bgsub->y_threshold - v)>>24);

		p++;
		q++;
		r++;
	}

	return bgsub->diff;
/* The origin of subtraction function is;
 * diff(src, dest) = (abs(src - dest) > threshold) ? 0xff : 0;
 *
 * This functions is transformed to;
 * (threshold > (src - dest) > -threshold) ? 0 : 0xff;
 *
 * (v + threshold)>>24 is 0xff when v is less than -threshold.
 * (v - threshold)>>24 is 0xff when v is less than threshold.
 * So, ((v + threshold)>>24) | ((threshold - v)>>24) will become 0xff when
 * abs(src - dest) > threshold.
 */
}

/* Background image is refreshed every frame */
unsigned char *bgsub_subtract_update_y(BgSubtractor *bgsub, RGB32 *src)
{
	int i;
	int R, G, B;
	RGB32 *p;
	short *q;
	unsigned char *r;
	int v;
	const int area = bgsub->w * bgsub->h;

	p = src;
	q = (short *)bgsub->background;
	r = bgsub->diff;
	for(i=0; i<area; i++) {
		R = ((*p)&0xff0000)>>(16-1);
		G = ((*p)&0xff00)>>(8-2);
		B = (*p)&0xff;
		v = (R + G + B) - (int)(*q);
		*q = (short)(R + G + B);
		*r = ((v + bgsub->y_threshold)>>24) | ((bgsub->y_threshold - v)>>24);

		p++;
		q++;
		r++;
	}

	return bgsub->diff;
}

void bgsub_set_threshold_RGB(BgSubtractor *bgsub, int r, int g, int b)
{
	unsigned char R, G, B;

	R = G = B = 0xff;
	R = R<<r;
	G = G<<g;
	B = B<<b;
	bgsub->rgb_threshold = (RGB32)(R<<16 | G<<8 | B);
}

void bgsub_bgset_RGB(BgSubtractor *bgsub, RGB32 *src)
{
    int i;
    RGB32 *p;
	const int area = bgsub->w * bgsub->h;

    p = bgsub->background;
    for(i=0; i<area; i++) {
        *p++ = (*src++) & 0xfefefe;
    }
}

unsigned char *bgsub_subtract_RGB(BgSubtractor *bgsub, RGB32 *src)
{
	int i;
	RGB32 *p, *q;
	unsigned a, b;
	unsigned char *r;
	const int area = bgsub->w * bgsub->h;

	p = src;
	q = bgsub->background;
	r = bgsub->diff;
	for(i=0; i<area; i++) {
		a = (*p++)|0x1010100;
		b = *q++;
		a = a - b;
		b = a & 0x1010100;
		b = b - (b>>8);
		b = b ^ 0xffffff;
		a = a ^ b;
		a = a & bgsub->rgb_threshold;
		*r++ = (0 - a)>>24;
	}

	return bgsub->diff;
}

unsigned char *bgsub_subtract_update_RGB(BgSubtractor *bgsub, RGB32 *src)
{
	int i;
	RGB32 *p, *q;
	unsigned a, b;
	unsigned char *r;
	const int area = bgsub->w * bgsub->h;

	p = src;
	q = bgsub->background;
	r = bgsub->diff;
	for(i=0; i<area; i++) {
		a = *p|0x1010100;
		b = *q&0xfefefe;
		*q++ = *p++;
		a = a - b;
		b = a & 0x1010100;
		b = b - (b>>8);
		b = b ^ 0xffffff;
		a = a ^ b;
		a = a & bgsub->rgb_threshold;
		*r++ = (0 - a)>>24;
	}

	return bgsub->diff;
}
