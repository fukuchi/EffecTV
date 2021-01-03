/*
 * EffecTV - Realtime Digital Video Effector
 * Copyright (C) 2001-2006 FUKUCHI Kentaro
 *
 * image.c: utilities for image processing.
 *
 */

#include <string.h>
#include <stdlib.h>
#include "EffecTV.h"
#include "utils.h"

RGB32 *stretching_buffer;

static unsigned char *buf;
static unsigned char *buf2;

/* Initializer is called from utils_init(). */
int image_init(void)
{
	stretching_buffer = (RGB32 *)malloc(video_area * sizeof(RGB32));
	buf = (unsigned char *)malloc(video_area * sizeof(unsigned char));
	buf2 = (unsigned char *)malloc(video_area * sizeof(unsigned char));
	if(stretching_buffer == NULL || buf == NULL || buf2 == NULL) {
		return -1;
	}
	memset(buf, 0, video_area * sizeof(unsigned char));
	memset(buf2, 0, video_area * sizeof(unsigned char));

	return 0;
}

void image_end(void)
{
	free(stretching_buffer);
	free(buf);
}

void image_stretching_buffer_clear(RGB32 color)
{
	int i;
	RGB32 *p;

	p = stretching_buffer;
	for(i=0; i<video_area; i++) {
		*p++ = color;
	}
}

void image_stretch(RGB32 *src, int src_width, int src_height,
                    RGB32 *dest, int dest_width, int dest_height)
{
	int x, y;
	int sx, sy;
	int tx, ty;
	RGB32 *p;

	tx = src_width * 65536 / dest_width;
	ty = src_height * 65536 / dest_height;
	sy = 0;
	for(y=0; y<dest_height; y++) {
		p = src + (sy>>16) * src_width;
		sx = 0;
		for(x=0; x<dest_width; x++) {
			*dest++ = p[(sx>>16)];
			sx += tx;
		}
		sy += ty;
	}
}

static void image_stretch_to_screen_double(void)
{
	int x, y;
	RGB32 *src, *dest1, *dest2;
	int width = video_width;
	int height = video_height;
	int swidth = screen_width;

	src = stretching_buffer;
	dest1 = (RGB32 *)screen->pixels;
	dest2 = dest1 + swidth;

	for(y=0; y<height; y++) {
		for(x=0; x<width; x++) {
			dest1[x*2] = *src;
			dest1[x*2+1] = *src;
			dest2[x*2] = *src;
			dest2[x*2+1] = *src;
			src++;
		}
		dest1 += swidth*2;
		dest2 += swidth*2;
	}
}

void image_stretch_to_screen(void)
{
	if(screen_scale == 2) {
		image_stretch_to_screen_double();
	} else {
		image_stretch(stretching_buffer, video_width, video_height,
		  (RGB32 *)screen->pixels, screen_width, screen_height);
	}
}

/* Y value filters */
unsigned char *image_y_over(RGB32 *src, int y_threshold)
{
	int i;
	int R, G, B, v;
	unsigned char *p = buf;

	for(i = video_area; i>0; i--) {
		R = ((*src)&0xff0000)>>(16-1);
		G = ((*src)&0xff00)>>(8-2);
		B = (*src)&0xff;
		v = y_threshold - (R + G + B);
		*p = (unsigned char)(v>>24);
		src++;
		p++;
	}

	return buf;
}

unsigned char *image_y_under(RGB32 *src, int y_threshold)
{
	int i;
	int R, G, B, v;
	unsigned char *p = buf;

	for(i = video_area; i>0; i--) {
		R = ((*src)&0xff0000)>>(16-1);
		G = ((*src)&0xff00)>>(8-2);
		B = (*src)&0xff;
		v = (R + G + B) - y_threshold;
		*p = (unsigned char)(v>>24);
		src++;
		p++;
	}

	return buf;
}

/* tiny edge detection */
unsigned char *image_edge(RGB32 *src, int y_threshold)
{
	int x, y;
	unsigned char *p, *q;
	int r, g, b;
	int ar, ag, ab;
	int w;

	p = (unsigned char *)src;
	q = buf;
	w = video_width * sizeof(RGB32);

	for(y=0; y<video_height - 1; y++) {
		for(x=0; x<video_width - 1; x++) {
			b = p[0];
			g = p[1];
			r = p[2];
			ab = abs(b - p[4]);
			ag = abs(g - p[5]);
			ar = abs(r - p[6]);
			ab += abs(b - p[w]);
			ag += abs(g - p[w+1]);
			ar += abs(r - p[w+2]);
			b = ab+ag+ar;
			if(b > y_threshold) {
				*q = 255;
			} else {
				*q = 0;
			}
			q++;
			p += 4;
		}
		p += 4;
		*q++ = 0;
	}
	memset(q, 0, video_width);

	return buf;
}

unsigned char *image_diff_denoise(unsigned char *diff)
{
	int x, y;
	unsigned char *src, *dest;
	unsigned int count;
	unsigned int sum1, sum2, sum3;
	const int width = video_width;

	src = diff;
	dest = buf2 + width + 1;
	for(y=1; y<video_height-1; y++) {
		sum1 = src[0] + src[width] + src[width*2];
		sum2 = src[1] + src[width+1] + src[width*2+1];
		src += 2;
		for(x=1; x<width-1; x++) {
			sum3 = src[0] + src[width] + src[width*2];
			count = sum1 + sum2 + sum3;
			sum1 = sum2;
			sum2 = sum3;
			*dest++ = (0xff*3 - count)>>24;
			src++;
		}
		dest += 2;
	}

	return buf2;
}

/* horizontal flipping */
void image_hflip(RGB32 *src, RGB32 *dest, int width, int height)
{
	int x, y;

	src += width - 1;
	for(y=0; y<height; y++) {
		for(x=0; x<width; x++) {
			*dest++ = *src--;
		}
		src += width * 2;
	}
}

