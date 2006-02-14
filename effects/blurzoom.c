/*
 * EffecTV - Realtime Digital Video Effector
 * Copyright (C) 2001-2006 FUKUCHI Kentaro
 *
 * RadioacTV - motion-enlightment effect.
 * Copyright (C) 2001-2002 FUKUCHI Kentaro
 *
 * I referred to "DUNE!" by QuoVadis for this effect.
 */

#include <stdlib.h>
#include <string.h>
#include "EffecTV.h"
#include "utils.h"

#define COLORS 32
#define PATTERN 4
#define MAGIC_THRESHOLD 40
#define RATIO 0.95

static int start(void);
static int stop(void);
static int draw(RGB32 *src, RGB32 *dest);
static int event(SDL_Event *event);

extern void blurzoomcore(void);

unsigned char *blurzoombuf;
int *blurzoomx;
int *blurzoomy;
int buf_width_blocks;
int buf_width;
int buf_height;
int buf_area;
int buf_margin_right;
int buf_margin_left;

static char *effectname = "RadioacTV";
static int stat;
static RGB32 *palette;
static RGB32 palettes[COLORS*PATTERN];
static int mode = 0; /* 0=normal/1=strobe/2=strobe2/3=trigger */
static int snapTime = 0;
static int snapInterval = 3;
static RGB32 *snapframe;

#define VIDEO_HWIDTH (buf_width/2)
#define VIDEO_HHEIGHT (buf_height/2)

/* this table assumes that video_width is times of 32 */
static void setTable(void)
{
	unsigned int bits;
	int x, y, tx, ty, xx;
	int ptr, prevptr;

	prevptr = (int)(0.5+RATIO*(-VIDEO_HWIDTH)+VIDEO_HWIDTH);
	for(xx=0; xx<(buf_width_blocks); xx++){
		bits = 0;
		for(x=0; x<32; x++){
			ptr= (int)(0.5+RATIO*(xx*32+x-VIDEO_HWIDTH)+VIDEO_HWIDTH);
#ifdef USE_NASM
			bits = bits<<1;
			if(ptr != prevptr)
				bits |= 1;
#else
			bits = bits>>1;
			if(ptr != prevptr)
				bits |= 0x80000000;
#endif /* USE_NASM */
			prevptr = ptr;
		}
		blurzoomx[xx] = bits;
	}

	ty = (int)(0.5+RATIO*(-VIDEO_HHEIGHT)+VIDEO_HHEIGHT);
	tx = (int)(0.5+RATIO*(-VIDEO_HWIDTH)+VIDEO_HWIDTH);
	xx=(int)(0.5+RATIO*(buf_width-1-VIDEO_HWIDTH)+VIDEO_HWIDTH);
	blurzoomy[0] = ty * buf_width + tx;
	prevptr = ty * buf_width + xx;
	for(y=1; y<buf_height; y++){
		ty = (int)(0.5+RATIO*(y-VIDEO_HHEIGHT)+VIDEO_HHEIGHT);
		blurzoomy[y] = ty * buf_width + tx - prevptr;
		prevptr = ty * buf_width + xx;
	}
}		

#ifndef USE_NASM
/* following code is a replacement of blurzoomcore.nas. */
static void blur(void)
{
	int x, y;
	int width;
	unsigned char *p, *q;
	unsigned char v;
	
	width = buf_width;
	p = blurzoombuf + width + 1;
	q = p + buf_area;

	for(y=buf_height-2; y>0; y--) {
		for(x=width-2; x>0; x--) {
			v = (*(p-width) + *(p-1) + *(p+1) + *(p+width))/4 - 1;
			if(v == 255) v = 0;
			*q = v;
			p++;
			q++;
		}
		p += 2;
		q += 2;
	}
}

static void zoom(void)
{
	int b, x, y;
	unsigned char *p, *q;
	int blocks, height;
	int dx;

	p = blurzoombuf + buf_area;
	q = blurzoombuf;
	height = buf_height;
	blocks = buf_width_blocks;

	for(y=0; y<height; y++) {
		p += blurzoomy[y];
		for(b=0; b<blocks; b++) {
			dx = blurzoomx[b];
			for(x=0; x<32; x++) {
				p += (dx & 1);
				*q++ = *p;
				dx = dx>>1;
			}
		}
	}
}

void blurzoomcore(void)
{
	blur();
	zoom();
}
#endif /* USE_NASM */

static void makePalette(void)
{
	int i;

#define DELTA (255/(COLORS/2-1))

	for(i=0; i<COLORS/2; i++) {
		palettes[           i] = i*DELTA;
		palettes[COLORS   + i] = (i*DELTA)<<8;
		palettes[COLORS*2 + i] = (i*DELTA)<<16;
	}
	for(i=0; i<COLORS/2; i++) {
		palettes[         + i + COLORS/2] = 255 | (i*DELTA)<<16 | (i*DELTA)<<8;
		palettes[COLORS   + i + COLORS/2] = (255<<8) | (i*DELTA)<<16 | i*DELTA;
		palettes[COLORS*2 + i + COLORS/2] = (255<<16) | (i*DELTA)<<8 | i*DELTA;
	}
	for(i=0; i<COLORS; i++) {
		palettes[COLORS*3 + i] = (255*i/COLORS) * 0x10101;
	}
	for(i=0; i<COLORS*PATTERN; i++) {
		palettes[i] = palettes[i] & 0xfefeff;
	}
}

effect *blurzoomRegister(void)
{
	effect *entry;
	
	buf_width_blocks = (video_width / 32);
	if(buf_width_blocks > 255) {
		return NULL;
	}
	buf_width = buf_width_blocks * 32;
	buf_height = video_height;
	buf_area = buf_width * buf_height;
	buf_margin_left = (video_width - buf_width)/2;
	buf_margin_right = video_width - buf_width - buf_margin_left;

	sharedbuffer_reset();
	blurzoombuf = (unsigned char *)sharedbuffer_alloc(buf_area * 2);
	if(blurzoombuf == NULL) {
		return NULL;
	}

	blurzoomx = (int *)malloc(buf_width*sizeof(int));
	blurzoomy = (int *)malloc(buf_height*sizeof(int));
	if(blurzoomx == NULL || blurzoomy == NULL) {
		return NULL;
	}

	entry = (effect *)malloc(sizeof(effect));
	if(entry == NULL) {
		return NULL;
	}
	
	entry->name = effectname;
	entry->start = start;
	entry->stop = stop;
	entry->draw = draw;
	entry->event = event;

	setTable();
	makePalette();
	palette = palettes;

	return entry;
}

static int start(void)
{
	memset(blurzoombuf, 0, buf_area * 2);
	image_set_threshold_y(MAGIC_THRESHOLD);
	snapframe = (RGB32 *)malloc(video_area*PIXEL_SIZE);
	if(snapframe == NULL)
		return -1;

	stat = 1;
	return 0;
}

static int stop(void)
{
	if(stat) {
		free(snapframe);
		stat = 0;
	}

	return 0;
}

static int draw(RGB32 *src, RGB32 *dest)
{
	int x, y;
	RGB32 a, b;
	unsigned char *diff, *p;

	if(mode != 2 || snapTime <= 0) {
		diff = image_bgsubtract_update_y(src);
		if(mode == 0 || snapTime <= 0) {
			diff += buf_margin_left;
			p = blurzoombuf;
			for(y=0; y<buf_height; y++) {
				for(x=0; x<buf_width; x++) {
					p[x] |= diff[x] >> 3;
				}
				diff += video_width;
				p += buf_width;
			}
			if(mode == 1 || mode == 2) {
				memcpy(snapframe, src, video_area * PIXEL_SIZE);
			}
		}
	}
	blurzoomcore();

	if(mode == 1 || mode == 2) {
		src = snapframe;
	}
	p = blurzoombuf;
	for(y=0; y<video_height; y++) {
		for(x=0; x<buf_margin_left; x++) {
			*dest++ = *src++;
		}
		for(x=0; x<buf_width; x++) {
			a = *src++ & 0xfefeff;
			b = palette[*p++];
			a += b;
			b = a & 0x1010100;
			*dest++ = a | (b - (b >> 8));
		}
		for(x=0; x<buf_margin_right; x++) {
			*dest++ = *src++;
		}
	}

	if(mode == 1 || mode == 2) {
		snapTime--;
		if(snapTime < 0) {
			snapTime = snapInterval;
		}
	}

	return 0;
}

static int event(SDL_Event *event)
{
	if(event->type == SDL_KEYDOWN) {
		switch(event->key.keysym.sym) {
		case SDLK_1:
		case SDLK_2:
		case SDLK_3:
		case SDLK_4:
			mode = event->key.keysym.sym - SDLK_1;
			if(mode == 3)
				snapTime = 1;
			else
				snapTime = 0;
			break;
		case SDLK_KP1:
		case SDLK_KP2:
		case SDLK_KP3:
		case SDLK_KP4:
			mode = event->key.keysym.sym - SDLK_KP1;
			if(mode == 3)
				snapTime = 1;
			else
				snapTime = 0;
			break;
		case SDLK_r:
			palette = &palettes[COLORS*2];
			break;
		case SDLK_g:
			palette = &palettes[COLORS];
			break;
		case SDLK_b:
			palette = &palettes[0];
			break;
		case SDLK_w:
			palette = &palettes[COLORS*3];
			break;
		case SDLK_SPACE:
			if(mode == 3)
				snapTime = 0;
			break;

		case SDLK_INSERT:
			snapInterval++;
			break;

		case SDLK_DELETE:
			snapInterval--;
			if(snapInterval < 1) {
				snapInterval = 1;
			}
			break;
		default:
			break;
		}
	} else if(event->type == SDL_KEYUP) {
		switch(event->key.keysym.sym) {
		case SDLK_SPACE:
			if(mode == 3)
				snapTime = 1;
			break;
		default:
			break;
		}
	}

	return 0;
}
