/*
 * EffecTV - Realtime Digital Video Effector
 * Copyright (C) 2001-2006 FUKUCHI Kentaro
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
#include "utils.h"

static RGB32 *GB65table;
static RGB32 *GB55table;
#define CLIP 320
static unsigned char clip[256 + CLIP * 2];
static RGB32 *buffer = NULL;
static int buffer_length = 0;

int palette_init(void)
{
	int i;
	int g, b;

	GB65table = (RGB32 *)malloc(32*64*sizeof(RGB32));
	GB55table = (RGB32 *)malloc(32*32*sizeof(RGB32));
	if(GB65table == NULL || GB55table == NULL) {
		fprintf(stderr, "palette_init: Memory allocation error.\n");
		return -1;
	}

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
	for(i=0; i<CLIP; i++)
		clip[i] = 0;
	for(i=0; i<256; i++)
		clip[CLIP+i] = i;
	for(i=256+CLIP; i<256+CLIP*2; i++)
		clip[i] = 255;

	return 0;
}

void palette_end(void)
{
	free(GB65table);
	free(GB55table);
}

static inline void check_buffer(int width, int height)
{
	int length;

	length = width * height * PIXEL_SIZE;
	if(length > buffer_length) {
		if(buffer != NULL) {
			free(buffer);
		}
		buffer = (RGB32 *)malloc(length);
		if(buffer == NULL) {
			fprintf(stderr, "Memory allocation failed.\n");
			exit(1);
		}
		buffer_length = length;
	}
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

static void convert_YUV422toRGB32
(unsigned char *src, RGB32 *dest, int width, int height)
{
	int i, length;
	unsigned int gray;
	unsigned int u, v;
	unsigned char *p;

	length = width * height / 2;
	p = (unsigned char *)dest;
	for(i=0; i<length; i++) {
		u = src[1];
		v = src[3];
		gray = YtoRGB[src[0]];
		p[0] = clip[CLIP + gray + UtoB[u]];
		p[1] = clip[CLIP + gray + UtoG[u] + VtoG[v]];
		p[2] = clip[CLIP + gray + VtoR[v]];
		gray = YtoRGB[src[2]];
		p[4] = clip[CLIP + gray + UtoB[u]];
		p[5] = clip[CLIP + gray + UtoG[u] + VtoG[v]];
		p[6] = clip[CLIP + gray + VtoR[v]];
		p += 8;
		src += 4;
	}
}

static void convert_YUV422toRGB32_hflip
(unsigned char *src, RGB32 *dest, int width, int height)
{
	int x, y;
	unsigned int gray;
	unsigned int u, v;
	unsigned char *p;

	/* Images will be cluttered when 'width' is odd number. It is not so
	 * difficult to adjust it, but it makes conversion little slow.. */
	width &= 0xfffffffe;
	p = (unsigned char *)(dest + width - 2);
	for(y=0; y<height; y++) {
		for(x=0; x<width; x+=2) {
			u = src[1];
			v = src[3];
			gray = YtoRGB[src[0]];
			p[4] = clip[CLIP + gray + UtoB[u]];
			p[5] = clip[CLIP + gray + UtoG[u] + VtoG[v]];
			p[6] = clip[CLIP + gray + VtoR[v]];
			gray = YtoRGB[src[2]];
			p[0] = clip[CLIP + gray + UtoB[u]];
			p[1] = clip[CLIP + gray + UtoG[u] + VtoG[v]];
			p[2] = clip[CLIP + gray + VtoR[v]];
			p -= 8;
			src += 4;
		}
		p += width * 8;
	}
}

static void convert_YUV422PtoRGB32
(unsigned char *src, RGB32 *dest, int width, int height)
{
	int i;
	int length;
	unsigned int gray;
	unsigned char *cu, *cv;
	unsigned int u, v;
	unsigned char *p;

	length = height * (width / 2);
	cu = src + width * height;
	cv = cu + length;
	p = (unsigned char *)dest;

	for(i=0; i<length; i++) {
		gray = YtoRGB[src[0]];
		u = *cu;
		v = *cv;
		p[0] = clip[CLIP + gray + UtoB[u]];
		p[1] = clip[CLIP + gray + UtoG[u] + VtoG[v]];
		p[2] = clip[CLIP + gray + VtoR[v]];
		gray = YtoRGB[src[1]];
		p[4] = clip[CLIP + gray + UtoB[u]];
		p[5] = clip[CLIP + gray + UtoG[u] + VtoG[v]];
		p[6] = clip[CLIP + gray + VtoR[v]];
		p += 8;
		src += 2;
		cu++;
		cv++;
	}
}

static void convert_YUV422PtoRGB32_hflip
(unsigned char *src, RGB32 *dest, int width, int height)
{
	int x, y;
	int hwidth;
	unsigned int gray;
	unsigned char *cu, *cv;
	unsigned int u, v;
	unsigned char *p;

	hwidth = width / 2;
	cu = src + width * height;
	cv = cu + width * height / 2;
	p = (unsigned char *)(dest + hwidth * 2 - 2);

	for(y=0; y<height; y++) {
		for(x=0; x<hwidth; x++) {
			gray = YtoRGB[src[0]];
			u = *cu;
			v = *cv;
			p[4] = clip[CLIP + gray + UtoB[u]];
			p[5] = clip[CLIP + gray + UtoG[u] + VtoG[v]];
			p[6] = clip[CLIP + gray + VtoR[v]];
			gray = YtoRGB[src[1]];
			p[0] = clip[CLIP + gray + UtoB[u]];
			p[1] = clip[CLIP + gray + UtoG[u] + VtoG[v]];
			p[2] = clip[CLIP + gray + VtoR[v]];
			p -= 8;
			src += 2;
			cu++;
			cv++;
		}
		p += (width + hwidth * 2) * PIXEL_SIZE;
	}
}

static void convert_YUV411PtoRGB32
(unsigned char *src, RGB32 *dest, int width, int height)
{
	int i;
	int length;
	unsigned int gray;
	unsigned char *cu, *cv;
	unsigned int u, v;
	unsigned char *p;

	length = height * (width / 4);
	cu = src + width * height;
	cv = cu + length;
	p = (unsigned char *)dest;

	for(i=0; i<length; i++) {
		u = *cu;
		v = *cv;
		gray = YtoRGB[src[0]];
		p[0] = clip[CLIP + gray + UtoB[u]];
		p[1] = clip[CLIP + gray + UtoG[u] + VtoG[v]];
		p[2] = clip[CLIP + gray + VtoR[v]];
		gray = YtoRGB[src[1]];
		p[4] = clip[CLIP + gray + UtoB[u]];
		p[5] = clip[CLIP + gray + UtoG[u] + VtoG[v]];
		p[6] = clip[CLIP + gray + VtoR[v]];
		gray = YtoRGB[src[2]];
		p[8] = clip[CLIP + gray + UtoB[u]];
		p[9] = clip[CLIP + gray + UtoG[u] + VtoG[v]];
		p[10] = clip[CLIP + gray + VtoR[v]];
		gray = YtoRGB[src[3]];
		p[12] = clip[CLIP + gray + UtoB[u]];
		p[13] = clip[CLIP + gray + UtoG[u] + VtoG[v]];
		p[14] = clip[CLIP + gray + VtoR[v]];
		p += 16;
		src += 4;
		cu++;
		cv++;
	}
}

static void convert_YUV411PtoRGB32_hflip
(unsigned char *src, RGB32 *dest, int width, int height)
{
	int x, y;
	int qwidth;
	unsigned int gray;
	unsigned char *cu, *cv;
	unsigned int u, v;
	unsigned char *p;

	qwidth = width / 4;
	cu = src + width * height;
	cv = cu + width * height / 4;
	p = (unsigned char *)(dest + qwidth * 4 - 4);

	for(y=0; y<height; y++) {
		for(x=0; x<qwidth; x++) {
			u = *cu;
			v = *cv;
			gray = YtoRGB[src[0]];
			p[12] = clip[CLIP + gray + UtoB[u]];
			p[13] = clip[CLIP + gray + UtoG[u] + VtoG[v]];
			p[14] = clip[CLIP + gray + VtoR[v]];
			gray = YtoRGB[src[1]];
			p[8] = clip[CLIP + gray + UtoB[u]];
			p[9] = clip[CLIP + gray + UtoG[u] + VtoG[v]];
			p[10] = clip[CLIP + gray + VtoR[v]];
			gray = YtoRGB[src[2]];
			p[4] = clip[CLIP + gray + UtoB[u]];
			p[5] = clip[CLIP + gray + UtoG[u] + VtoG[v]];
			p[6] = clip[CLIP + gray + VtoR[v]];
			gray = YtoRGB[src[3]];
			p[0] = clip[CLIP + gray + UtoB[u]];
			p[1] = clip[CLIP + gray + UtoG[u] + VtoG[v]];
			p[2] = clip[CLIP + gray + VtoR[v]];
			p -= 16;
			src += 4;
			cu++;
			cv++;
		}
		p += (width + qwidth * 4) * PIXEL_SIZE;
	}
}

static void convert_YUV420PtoRGB32
(unsigned char *src, RGB32 *dest, int width, int height)
{
	int x, y;
	int hwidth;
	unsigned int gray;
	unsigned char *cu, *cv;
	unsigned int u, v;
	unsigned char *p;

	hwidth = width / 2;
	cu = src + width * height;
	cv = cu + width * height / 4;
	p = (unsigned char *)dest;

	for(y=0; y<height/2; y++) {
		for(x=0; x<hwidth; x++) {
			gray = YtoRGB[src[0]];
			u = cu[x];
			v = cv[x];
			p[0] = clip[CLIP + gray + UtoB[u]];
			p[1] = clip[CLIP + gray + UtoG[u] + VtoG[v]];
			p[2] = clip[CLIP + gray + VtoR[v]];
			gray = YtoRGB[src[1]];
			p[4] = clip[CLIP + gray + UtoB[u]];
			p[5] = clip[CLIP + gray + UtoG[u] + VtoG[v]];
			p[6] = clip[CLIP + gray + VtoR[v]];
			p += 8;
			src += 2;
		}
		for(x=0; x<hwidth; x++) {
			gray = YtoRGB[src[0]];
			u = cu[x];
			v = cv[x];
			p[0] = clip[CLIP + gray + UtoB[u]];
			p[1] = clip[CLIP + gray + UtoG[u] + VtoG[v]];
			p[2] = clip[CLIP + gray + VtoR[v]];
			gray = YtoRGB[src[1]];
			p[4] = clip[CLIP + gray + UtoB[u]];
			p[5] = clip[CLIP + gray + UtoG[u] + VtoG[v]];
			p[6] = clip[CLIP + gray + VtoR[v]];
			p += 8;
			src += 2;
		}
		cu += hwidth;
		cv += hwidth;
	}
}

static void convert_YUV420PtoRGB32_hflip
(unsigned char *src, RGB32 *dest, int width, int height)
{
	int x, y;
	int hwidth;
	unsigned int gray;
	unsigned char *cu, *cv;
	unsigned int u, v;
	unsigned char *p;

	hwidth = width / 2;
	cu = src + width * height;
	cv = cu + width * height / 4;
	p = (unsigned char *)(dest + hwidth * 2 - 2);

	for(y=0; y<height/2; y++) {
		for(x=0; x<hwidth; x++) {
			gray = YtoRGB[src[0]];
			u = cu[x];
			v = cv[x];
			p[4] = clip[CLIP + gray + UtoB[u]];
			p[5] = clip[CLIP + gray + UtoG[u] + VtoG[v]];
			p[6] = clip[CLIP + gray + VtoR[v]];
			gray = YtoRGB[src[1]];
			p[0] = clip[CLIP + gray + UtoB[u]];
			p[1] = clip[CLIP + gray + UtoG[u] + VtoG[v]];
			p[2] = clip[CLIP + gray + VtoR[v]];
			p -= 8;
			src += 2;
		}
		p += (width + hwidth * 2) * PIXEL_SIZE;
		for(x=0; x<hwidth; x++) {
			gray = YtoRGB[src[0]];
			u = cu[x];
			v = cv[x];
			p[4] = clip[CLIP + gray + UtoB[u]];
			p[5] = clip[CLIP + gray + UtoG[u] + VtoG[v]];
			p[6] = clip[CLIP + gray + VtoR[v]];
			gray = YtoRGB[src[1]];
			p[0] = clip[CLIP + gray + UtoB[u]];
			p[1] = clip[CLIP + gray + UtoG[u] + VtoG[v]];
			p[2] = clip[CLIP + gray + VtoR[v]];
			p -= 8;
			src += 2;
		}
		p += (width + hwidth * 2) * PIXEL_SIZE;
		cu += hwidth;
		cv += hwidth;
	}
}

static void convert_YUV410PtoRGB32
(unsigned char *src, RGB32 *dest, int width, int height)
{
	int x, y, i;
	int qwidth;
	unsigned int gray;
	unsigned char *cu, *cv;
	unsigned int u, v;
	unsigned char *p;

	qwidth = width / 4;
	cu = src + width * height;
	cv = cu + width * height / 16;
	p = (unsigned char *)dest;

	for(y=0; y<height/4; y++) {
		for(i=0; i<4; i++) {
			for(x=0; x<qwidth; x++) {
				u = cu[x];
				v = cv[x];
				gray = YtoRGB[src[0]];
				p[0] = clip[CLIP + gray + UtoB[u]];
				p[1] = clip[CLIP + gray + UtoG[u] + VtoG[v]];
				p[2] = clip[CLIP + gray + VtoR[v]];
				gray = YtoRGB[src[1]];
				p[4] = clip[CLIP + gray + UtoB[u]];
				p[5] = clip[CLIP + gray + UtoG[u] + VtoG[v]];
				p[6] = clip[CLIP + gray + VtoR[v]];
				gray = YtoRGB[src[2]];
				p[8] = clip[CLIP + gray + UtoB[u]];
				p[9] = clip[CLIP + gray + UtoG[u] + VtoG[v]];
				p[10] = clip[CLIP + gray + VtoR[v]];
				gray = YtoRGB[src[3]];
				p[12] = clip[CLIP + gray + UtoB[u]];
				p[13] = clip[CLIP + gray + UtoG[u] + VtoG[v]];
				p[14] = clip[CLIP + gray + VtoR[v]];
				p += 16;
				src += 4;
			}
		}
		cu += qwidth;
		cv += qwidth;
	}
}

static void convert_YUV410PtoRGB32_hflip
(unsigned char *src, RGB32 *dest, int width, int height)
{
	int x, y, i;
	int qwidth;
	unsigned int gray;
	unsigned char *cu, *cv;
	unsigned int u, v;
	unsigned char *p;

	qwidth = width / 4;
	cu = src + width * height;
	cv = cu + width * height / 16;
	p = (unsigned char *)(dest + qwidth * 4 - 4);

	for(y=0; y<height/4; y++) {
		for(i=0; i<4; i++) {
			for(x=0; x<qwidth; x++) {
				u = cu[x];
				v = cv[x];
				gray = YtoRGB[src[0]];
				p[12] = clip[CLIP + gray + UtoB[u]];
				p[13] = clip[CLIP + gray + UtoG[u] + VtoG[v]];
				p[14] = clip[CLIP + gray + VtoR[v]];
				gray = YtoRGB[src[1]];
				p[8] = clip[CLIP + gray + UtoB[u]];
				p[9] = clip[CLIP + gray + UtoG[u] + VtoG[v]];
				p[10] = clip[CLIP + gray + VtoR[v]];
				gray = YtoRGB[src[2]];
				p[4] = clip[CLIP + gray + UtoB[u]];
				p[5] = clip[CLIP + gray + UtoG[u] + VtoG[v]];
				p[6] = clip[CLIP + gray + VtoR[v]];
				gray = YtoRGB[src[3]];
				p[0] = clip[CLIP + gray + UtoB[u]];
				p[1] = clip[CLIP + gray + UtoG[u] + VtoG[v]];
				p[2] = clip[CLIP + gray + VtoR[v]];
				p -= 16;
				src += 4;
			}
			p += (width + qwidth * 4) * PIXEL_SIZE;
		}
		cu += qwidth;
		cv += qwidth;
	}
}

static const struct palette_converter_toRGB32_map converter_toRGB32_list[] = {
	{VIDEO_PALETTE_RGB24,   convert_RGB24toRGB32,   convert_RGB24toRGB32_hflip},
	{VIDEO_PALETTE_RGB565,  convert_RGB565toRGB32,  convert_RGB565toRGB32_hflip},
	{VIDEO_PALETTE_RGB555,  convert_RGB555toRGB32,  convert_RGB555toRGB32_hflip},
	{VIDEO_PALETTE_YUV422,  convert_YUV422toRGB32,  convert_YUV422toRGB32_hflip},
	{VIDEO_PALETTE_YUV422P, convert_YUV422PtoRGB32, convert_YUV422PtoRGB32_hflip},
	{VIDEO_PALETTE_YUV420P, convert_YUV420PtoRGB32, convert_YUV420PtoRGB32_hflip},
	{VIDEO_PALETTE_YUV411P, convert_YUV411PtoRGB32, convert_YUV411PtoRGB32_hflip},
	{VIDEO_PALETTE_YUV410P, convert_YUV410PtoRGB32, convert_YUV410PtoRGB32_hflip},
	{VIDEO_PALETTE_GREY  , convert_GREYtoRGB32, convert_GREYtoRGB32_hflip},
	{-1, NULL, NULL}
};

int palette_check_supported_converter_toRGB32
(int palette,
 palette_converter_toRGB32 **conv, palette_converter_toRGB32 **conv_hflip)
{
	int i;

	for(i=0; converter_toRGB32_list[i].palette>=0; i++) {
		if(palette != converter_toRGB32_list[i].palette) continue;

		if(video_grab_check(converter_toRGB32_list[i].palette)) {
			*conv = NULL;
			*conv_hflip = NULL;
			return 0;
		} else {
			*conv = converter_toRGB32_list[i].converter;
			*conv_hflip = converter_toRGB32_list[i].converter_hflip;
			if(debug) {
				printf("format: %d\n",converter_toRGB32_list[i].palette);
			}
			return 1;
		}
	}

	return 0;
}

void palette_get_supported_converter_toRGB32
(palette_converter_toRGB32 **conv, palette_converter_toRGB32 **conv_hflip)
{
	int i;

	for(i=0; converter_toRGB32_list[i].palette>=0; i++) {
		if(video_grab_check(converter_toRGB32_list[i].palette)) continue;
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
COMMON_CONVERTER_FROMRGB32(*(unsigned short *)dest = ((v>>3) & 0x1f) | ((v>>5) & 0x7e0) | ((v>>8) & 0xf800); dest += 2;)

static void convert_RGB32toRGB555
COMMON_CONVERTER_FROMRGB32(*(unsigned short *)dest = ((v>>3) & 0x1f) | ((v>>6) & 0x3e0) | ((v>>9) & 0x7c00); dest += 2;)

static void convert_RGB32toYUV422
(RGB32 *src, int src_width, int src_height,
 unsigned char *dest, int dest_width, int dest_height)
{
	int length;
	int x, y;
	int sx, sy;
	int tx, ty;
	RGB32 *p;
	unsigned char *q;

	if(src_width == dest_width && src_height == dest_height) {
		length = src_width * src_height / 2;
		q = (unsigned char *)src;
		for(x=0; x<length; x++) {
			dest[0] = BtoY[q[0]] + GtoY[q[1]] + RtoY[q[2]] + 16;
			dest[1] = RtoV[q[0]] + GtoU[q[1]] + RtoU[q[2]] + 128;
			dest[3] = BtoV[q[0]] + GtoV[q[1]] + RtoV[q[2]] + 128;
			dest[2] = BtoY[q[4]] + GtoY[q[5]] + RtoY[q[6]] + 16;
			dest += 4;
			q += 8;
		}
	} else {
	/* Images will be cluttered when 'dest_width' is odd number. It is not so
	 * difficult to adjust it, but it makes conversion little slow.. */
		tx = src_width * 256 / dest_width;
		ty = src_height * 256 / dest_height;
		sy = 0;
		dest_width /= 4;
		dest_width *= 2;
		for(y=0; y<dest_height; y++) {
			p = src + (sy>>8) * src_width;
			sx = 0;
			for(x=0; x<dest_width; x++) {
				q = (unsigned char *)&p[sx>>8];
				dest[0] = BtoY[q[0]] + GtoY[q[1]] + RtoY[q[2]] + 16;
				dest[1] = RtoV[q[0]] + GtoU[q[1]] + RtoU[q[2]] + 128;
				dest[3] = BtoV[q[0]] + GtoV[q[1]] + RtoV[q[2]] + 128;
				sx += tx;
				q = (unsigned char *)&p[sx>>8];
				dest[2] = BtoY[q[0]] + GtoY[q[1]] + RtoY[q[2]] + 16;
				dest += 4;
				sx += tx;
			}
			sy += ty;
		}
	}
}

static void convert_RGB32toYUV422P
(RGB32 *src, int src_width, int src_height,
 unsigned char *dest, int dest_width, int dest_height)
{
	int length;
	int i;
	unsigned char *p, *Y, *U, *V;

	if(src_width == dest_width && src_height == dest_height) {
		p = (unsigned char *)src;
	} else {
		check_buffer(dest_width, dest_height);
		image_stretch(src, src_width, src_height, buffer, dest_width, dest_height);
		p = (unsigned char *)buffer;
	}

	length = dest_width * dest_height / 2;
	Y = dest;
	U = dest + dest_width * dest_height;
	V = U + length;

	for(i=length; i>0; i--) {
		Y[0] = BtoY[p[0]] + GtoY[p[1]] + RtoY[p[2]] + 16;
		Y[1] = BtoY[p[4]] + GtoY[p[5]] + RtoY[p[6]] + 16;
		*U = RtoV[p[0]] + GtoU[p[1]] + RtoU[p[2]] + 128;
		*V = BtoV[p[0]] + GtoV[p[1]] + RtoV[p[2]] + 128;
		Y += 2;
		U++;
		V++;
		p += 8;
	}
}

static void convert_RGB32toYUV420P
(RGB32 *src, int src_width, int src_height,
 unsigned char *dest, int dest_width, int dest_height)
{
	int x, y;
	unsigned char *p, *Y, *U, *V;

	if(src_width == dest_width && src_height == dest_height) {
		p = (unsigned char *)src;
	} else {
		check_buffer(dest_width, dest_height);
		image_stretch(src, src_width, src_height, buffer, dest_width, dest_height);
		p = (unsigned char *)buffer;
	}

	Y = dest;
	U = dest + dest_width * dest_height;
	V = U + dest_width * dest_height / 4;

	for(y=0; y<dest_height; y+=2) {
		for(x=0; x<dest_width; x+=2) {
			Y[0] = BtoY[p[0]] + GtoY[p[1]] + RtoY[p[2]] + 16;
			Y[1] = BtoY[p[4]] + GtoY[p[5]] + RtoY[p[6]] + 16;
			*U = RtoV[p[0]] + GtoU[p[1]] + RtoU[p[2]] + 128;
			*V = BtoV[p[0]] + GtoV[p[1]] + RtoV[p[2]] + 128;
			Y += 2;
			U++;
			V++;
			p += 8;
		}
		for(x=0; x<dest_width; x++) {
			*Y++ = BtoY[p[0]] + GtoY[p[1]] + RtoY[p[2]] + 16;
			p += 4;
		}
	}
}

static const struct palette_converter_fromRGB32_map converter_fromRGB32_list[] = {
	{VIDEO_PALETTE_RGB32, convert_RGB32toRGB32},
	{VIDEO_PALETTE_RGB24, convert_RGB32toRGB24},
	{VIDEO_PALETTE_RGB565, convert_RGB32toRGB565},
	{VIDEO_PALETTE_RGB555, convert_RGB32toRGB555},
	{VIDEO_PALETTE_YUV422, convert_RGB32toYUV422},
	{VIDEO_PALETTE_YUV422P, convert_RGB32toYUV422P},
	{VIDEO_PALETTE_YUV420P, convert_RGB32toYUV420P},
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

static struct palettelist palettelists[] =
{
	{"rgb24", VIDEO_PALETTE_RGB24, "VIDEO_PALETTE_RGB24"},
	{"rgb565", VIDEO_PALETTE_RGB565, "VIDEO_PALETTE_RGB565"},
	{"rgb555", VIDEO_PALETTE_RGB555, "VIDEO_PALETTE_RGB555"},
	{"yuv422", VIDEO_PALETTE_YUV422, "VIDEO_PALETTE_YUV422"},
	{"yuv422p", VIDEO_PALETTE_YUV422P, "VIDEO_PALETTE_YUV422P"},
	{"yuv420p", VIDEO_PALETTE_YUV420P, "VIDEO_PALETTE_YUV420P"},
	{"yuv411p", VIDEO_PALETTE_YUV411P, "VIDEO_PALETTE_YUV411P"},
	{"yuv410p", VIDEO_PALETTE_YUV410P, "VIDEO_PALETTE_YUV410P"},
	{"grey", VIDEO_PALETTE_GREY, "VIDEO_PALETTE_GREY"},
	{"", -1, ""}
};

int palettex_getpalette(const char *name)
{
	int i;

	for(i=0; palettelists[i].palette != -1; i++) {
		if(strcasecmp(name, palettelists[i].tag) == 0) {
			return palettelists[i].palette;
		}
	}

	return -1;
}
