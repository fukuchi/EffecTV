/*
 * EffecTV - Realtime Digital Video Effector
 * Copyright (C) 2001 FUKUCHI Kentarou
 *
 * palette.c: pixel format converter
 *
 * Most of functions in this code depend on that the size of integer value is
 * 4 bytes, short interger is 2 bytes, char is 1 byte.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "EffecTV.h"
#include "palette.h"

static RGB32 *GB65table;
static RGB32 *GB55table;

int palette_init()
{
	int i;
	int g, b;

	GB65table = (RGB32 *)malloc(32*64*sizeof(RGB32));
	GB55table = (RGB32 *)malloc(32*32*sizeof(RGB32));
	if(GB65table == NULL || GB55table == NULL)
		return -1;

	i = 0;
	for(g=0; g<64; g++) {
		for(b=0; b<32; b++) {
			GB65table[i++] = (g<<10) | (b<<3);
		}
	}
	i = 0;
	for(g=0; g<32; g++) {
		for(b=0; b<32; b++) {
			GB55table[i++] = (g<<11) | (b<<3);
		}
	}

	return 0;
}

static void convert_RGB565toRGB32
(unsigned char *src, RGB32 *dest, int width, int height)
{
	int i, tmplen;
	unsigned int *p;
	unsigned int v;
	RGB32 w1, w2;
	int length;

	p = (unsigned int *)src;
	length = width * height;
	tmplen = length/2;
	for(i=0; i<tmplen; i++) {
		v = p[i];
		w1 = ((v<<8) & 0xf80000) | GB65table[v&0x7ff];
		w2 = ((v>>8) & 0xf80000) | GB65table[(v>>16)&0x7ff];
		dest[i*2] = w1;
		dest[i*2+1] = w2;
	}
	if(length & 1) {
		v = ((unsigned short *)src)[length-1];
		w1 = ((v<<16) & 0xf80000) | GB65table[v&0x7ff];
		dest[length-1] = w1;
	}
}

static void convert_RGB565toRGB32_hflip
(unsigned char *src, RGB32 *dest, int width, int height)
{
	int x, y;
	unsigned short v;
	RGB32 w;

	dest += width - 1;
	for(y=0; y<height; y++) {
		for(x=0; x<width; x++) {
			v = *(unsigned short *)src;
			w = ((v<<8) & 0xf80000) | GB65table[v&0x7ff];
			*dest-- = w;
			src += 2;
		}
		dest += width*2;
	}
}

static void convert_RGB555toRGB32
(unsigned char *src, RGB32 *dest, int width, int height)
{
	int i, length, tmplen;
	unsigned int *p;
	unsigned int v;
	RGB32 w1, w2;

	p = (unsigned int *)src;
	length = width * height;
	tmplen = length/2;
	for(i=0; i<tmplen; i++) {
		v = p[i];
		w1 = ((v<<9) & 0xf80000) | GB55table[v&0x3ff];
		w2 = ((v>>7) & 0xf80000) | GB55table[(v>>16)&0x3ff];
		dest[i*2] = w1;
		dest[i*2+1] = w2;
	}
	if(length & 1) {
		v = ((unsigned short *)src)[length-1];
		w1 = ((v<<17) & 0xf80000) | GB55table[v&0x3ff];
		dest[length-1] = w1;
	}
}

static void convert_RGB555toRGB32_hflip
(unsigned char *src, RGB32 *dest, int width, int height)
{
	int x, y;
	unsigned short v;
	RGB32 w;

	dest += width - 1;
	for(y=0; y<height; y++) {
		for(x=0; x<width; x++) {
			v = *(unsigned short *)src;
			w = ((v<<9) & 0xf80000) | GB55table[v&0x3ff];
			*dest-- = w;
			src += 2;
		}
		dest += width*2;
	}
}

static void convert_GREYtoRGB32
(unsigned char *src, RGB32 *dest, int width, int height)
{
	int i, length;

	length = width * height;
	for(i=0; i<length; i++) {
		dest[i] = src[i]<<16 | src[i]<<8 | src[i];
	}
}

static void convert_GREYtoRGB32_hflip
(unsigned char *src, RGB32 *dest, int width, int height)
{
	int x, y;
	unsigned char v;

	dest += width - 1;
	for(y=0; y<height; y++) {
		for(x=0; x<width; x++) {
			v = *src++;
			*dest-- = v<<16 | v<<8 | v;
		}
		dest += width * 2;
	}
}

static void convert_RGB24toRGB32
(unsigned char *src, RGB32 *dest, int width, int height)
{
	int i, length;

	length = width * height;
	for(i=0; i<length; i++) {
		*dest++ = *(unsigned int *)src & 0xffffff;
		src += 3;
	}
}

static void convert_RGB24toRGB32_hflip
(unsigned char *src, RGB32 *dest, int width, int height)
{
	int x, y;

	dest += width - 1;
	for(y=0; y<height; y++) {
		for(x=0; x<width; x++) {
			*dest-- = *(unsigned int *)src & 0xffffff;
			src += 3;
		}
		dest += width * 2;
	}
}

static const struct palette_converter_toRGB32_map converter_toRGB32_list[] = {
	{VIDEO_PALETTE_RGB24,  convert_RGB24toRGB32,  convert_RGB24toRGB32_hflip},
	{VIDEO_PALETTE_RGB565, convert_RGB565toRGB32, convert_RGB565toRGB32_hflip},
	{VIDEO_PALETTE_RGB555, convert_RGB555toRGB32, convert_RGB555toRGB32_hflip},
	{VIDEO_PALETTE_GREY  , convert_GREYtoRGB32, convert_GREYtoRGB32_hflip},
	{-1, NULL, NULL}
};

void palette_get_supported_converter_toRGB32
(palette_converter_toRGB32 **conv, palette_converter_toRGB32 **conv_hflip)
{
	int i;

	for(i=0; converter_toRGB32_list[i].palette>=0; i++) {
		if(video_setformat(converter_toRGB32_list[i].palette)) continue;
		*conv = converter_toRGB32_list[i].converter;
		*conv_hflip = converter_toRGB32_list[i].converter_hflip;
		if(debug) {
			printf("format: %d\n",converter_toRGB32_list[i].palette);
		}
		break;
	}
}

#define COMMON_CONVERTER_FROMRGB32(_convert_block_) \
(RGB32 *src, int src_width, int src_height,\
 unsigned char *dest, int dest_width, int dest_height)\
{\
	int x, y;\
	int sx, sy;\
	int tx, ty;\
	RGB32 *p, v;\
\
	if(src_width == dest_width && src_height == dest_height) {\
		for(x=0; x<src_width*src_height; x++) {\
			v = *src++;\
			_convert_block_ \
		}\
	} else {\
		tx = src_width * 256 / dest_width;\
		ty = src_height * 256 / dest_height;\
		sy = 0;\
		for(y=0; y<dest_height; y++) {\
			p = src + (sy>>8) * src_width;\
			sx = 0;\
			for(x=0; x<dest_width; x++) {\
				v = p[sx>>8];\
				_convert_block_ \
				sx += tx;\
			}\
			sy += ty;\
		}\
	}\
}

static void convert_RGB32toRGB32
COMMON_CONVERTER_FROMRGB32(*(RGB32 *)dest = v;dest+=4;)

static void convert_RGB32toRGB24
COMMON_CONVERTER_FROMRGB32(dest[0] = v & 0xff; dest[1] = (v>>8) & 0xff; dest[2] = (v>>16) & 0xff; dest += 3;)

static void convert_RGB32toRGB565
COMMON_CONVERTER_FROMRGB32(*(unsigned short *)dest = ((v>>3) & 0x1f) | ((v>>5) & 0x7e0) | ((v>>16) & 0xf800); dest += 2;)

static void convert_RGB32toRGB555
COMMON_CONVERTER_FROMRGB32(*(unsigned short *)dest = ((v>>3) & 0x1f) | ((v>>6) & 0x3e0) | ((v>>17) & 0x7c00); dest += 2;)

static const struct palette_converter_fromRGB32_map converter_fromRGB32_list[] = {
	{VIDEO_PALETTE_RGB32, convert_RGB32toRGB32},
	{VIDEO_PALETTE_RGB24, convert_RGB32toRGB24},
	{VIDEO_PALETTE_RGB565, convert_RGB32toRGB565},
	{VIDEO_PALETTE_RGB555, convert_RGB32toRGB555},
	{-1, NULL}
};

palette_converter_fromRGB32 *palette_get_supported_converter_fromRGB32
(int palette)
{
	int i;

	if(debug) {
		fprintf(stderr, "requested palette: %d\n", palette);
	}
	for(i=0; converter_fromRGB32_list[i].palette>=0; i++) {
		if(converter_fromRGB32_list[i].palette == palette)
			return converter_fromRGB32_list[i].converter;
	}
	return NULL;
}
