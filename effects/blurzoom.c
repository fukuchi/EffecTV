/*
 * EffecTV - Realtime Digital Video Effector
 * Copyright (C) 2001 FUKUCHI Kentarou
 *
 * RadioacTV - motion-enlightment effect.
 * Copyright (C) 2001 FUKUCHI Kentarou
 *
 * I referred to "DUNE!" by QuoVadis for this effect.
 */

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "../EffecTV.h"
#include "utils.h"

#define COLORS 32
#define MAGIC_THRESHOLD 40
#define RATIO 0.95

int blurzoomStart();
int blurzoomStop();
int blurzoomDraw();

extern void blurzoomcore();

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
static RGB32 palette[COLORS];

#define VIDEO_HWIDTH (buf_width/2)
#define VIDEO_HHEIGHT (buf_height/2)

/* this table assumes that video_width is times of 32 */
static void setTable()
{
	int bits, x, y, tx, ty, xx;
	int ptr, prevptr;

	prevptr = (int)(0.5+RATIO*(-VIDEO_HWIDTH)+VIDEO_HWIDTH);
	for(xx=0; xx<(buf_width_blocks); xx++){
		bits = 0;
		for(x=0; x<32; x++){
			ptr= (int)(0.5+RATIO*(xx*32+x-VIDEO_HWIDTH)+VIDEO_HWIDTH);
			bits = bits<<1;
			if(ptr != prevptr)
				bits |= 1;
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

static void makePalette()
{
	int i;

#define DELTA (255/(COLORS/2-1))

	for(i=0; i<COLORS/2; i++) {
		palette[i] = i*DELTA;
	}
	for(i=0; i<COLORS/2; i++) {
		palette[i+COLORS/2] = 255 | (i*DELTA)<<16 | (i*DELTA)<<8;
	}
	for(i=0; i<COLORS; i++) {
		palette[i] = palette[i] & 0xfefeff;
	}
}

effect *blurzoomRegister()
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
	blurzoombuf = (unsigned char *)sharedbuffer_alloc(buf_area*2);
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
	entry->start = blurzoomStart;
	entry->stop = blurzoomStop;
	entry->draw = blurzoomDraw;
	entry->event = NULL;

	setTable();
	makePalette();

	return entry;
}

int blurzoomStart()
{
	bzero(blurzoombuf, buf_area*2);
	image_set_threshold_y(MAGIC_THRESHOLD);
	if(video_grabstart())
		return -1;
	stat = 1;
	return 0;
}

int blurzoomStop()
{
	if(stat) {
		video_grabstop();
		stat = 0;
	}

	return 0;
}

int blurzoomDraw()
{
	int x, y;
	RGB32 a, b;
	RGB32 *src, *dest;
	unsigned char *diff, *p;

	if(video_syncframe())
		return -1;
	src = (RGB32 *)video_getaddress();

	diff = image_bgsubtract_update_y(src);
	diff += buf_margin_left;
	p = blurzoombuf;
	for(y=0; y<buf_height; y++) {
		for(x=0; x<buf_width; x++) {
			p[x] |= diff[x] >> 3;
		}
		diff += video_width;
		p += buf_width;
	}
	blurzoomcore();

	if(screen_mustlock()) {
		if(screen_lock() < 0) {
			return 0;
		}
	}
	if(stretch) {
		dest = stretching_buffer;
	} else {
		dest = (RGB32 *)screen_getaddress();
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
	if(stretch) {
		image_stretch_to_screen();
	}
	if(screen_mustlock()) {
		screen_unlock();
	}
	if(video_grabframe())
		return -1;


	return 0;
}
