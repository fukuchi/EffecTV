/*
 * EffecTV - Realtime Digital Video Effector
 * Copyright (C) 2001 FUKUCHI Kentarou
 *
 * DizzyTV - Alpha blending with zoomed and rotated images.
 * Copyright (C) 2001 FUKUCHI Kentarou
 *
 */

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "../EffecTV.h"
#include "utils.h"

int dizzyStart();
int dizzyStop();
int dizzyDraw();

static char *effectname = "DizzyTV";
static int state = 0;
static RGB32 *buffer;
static RGB32 *current_buffer, *alt_buffer;
static double dizz = 0;
static int dx, dy;
static int sx, sy;
static double phase = 0.0;
static double zoomrate = 1.05;

static void setParams()
{
	double vx, vy;
	double t;
	double x, y;

	x = video_width / 2;
	y = video_height / 2;
	t = (x*x + y*y) * zoomrate;
	if(video_width > video_height) {
		if(dizz >= 0) {
			if(dizz > x) dizz = x;
			vx = (x*(x-dizz) + y*y) / t;
		} else {
			if(dizz < -x) dizz = -x;
			vx = (x*(x+dizz) + y*y) / t;
		}
		vy = (dizz*y) / t;
	} else {
		if(dizz >= 0) {
			if(dizz > y) dizz = y;
			vx = (x*x + y*(y-dizz)) / t;
		} else {
			if(dizz < -y) dizz = -y;
			vx = (x*x + y*(y+dizz)) / t;
		}
		vy = (dizz*x) / t;
	}
	dx = vx * 65536;
	dy = vy * 65536;
	x = -x;
	y = -y;
	sx = (vx * x - vy * y - x) * 65536;
	sy = (vx * y + vy * x - y) * 65536;
}

static void rotateDizz()
{
	dizz = sin(phase)*10 + sin(phase*1.9+5)*5;
	phase += .1;
}

effect *dizzyRegister()
{
	effect *entry;
	
	sharedbuffer_reset();
	buffer = (RGB32 *)sharedbuffer_alloc(video_area*2);
	if(buffer == NULL) {
		return NULL;
	}

	entry = (effect *)malloc(sizeof(effect));
	if(entry == NULL) {
		return NULL;
	}
	
	entry->name = effectname;
	entry->start = dizzyStart;
	entry->stop = dizzyStop;
	entry->draw = dizzyDraw;
	entry->event = NULL;

	return entry;
}

int dizzyStart()
{
	bzero(buffer, video_area * 2 * sizeof(RGB32));
	current_buffer = buffer;
	alt_buffer = buffer + video_area;
	dizz = 0;
	phase = 0;

	if(video_grabstart())
		return -1;

	state = 1;
	return 0;
}

int dizzyStop()
{
	if(state) {
		video_grabstop();
		state = 0;
	}

	return 0;
}

int dizzyDraw()
{
	RGB32 *src, *dest;
	RGB32 *p;
	RGB32 v;
	int x, y;
	int ox, oy;
	int cx, cy;

	if(video_syncframe())
		return -1;
	src = (RGB32 *)video_getaddress();

	setParams();
	p = alt_buffer;
	for(y=video_height; y>0; y--) {
		ox = sx;
		oy = sy;
		for(x=video_width; x>0; x--) {
			cx = ox>>16;
			cy = oy>>16;
#if 0
			if(cx<0) cx = 0;
			if(cy<0) cy = 0;
			if(cx>=video_width) cx = video_width-1;
			if(cy>=video_height) cy = video_height-1;
#endif
			v = current_buffer[cy*video_width + cx] & 0xfcfcff;
			v = (v * 3) + ((*src++) & 0xfcfcff);
			*p++ = (v>>2);
			ox += dx;
			oy += dy;
		}
		sx -= dy;
		sy += dx;
	}

	if(video_grabframe())
		return -1;
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

	memcpy(dest, alt_buffer, video_area*sizeof(RGB32));

	if(stretch) {
		image_stretch_to_screen();
	}
	if(screen_mustlock()) {
		screen_unlock();
	}
	p = current_buffer;
	current_buffer = alt_buffer;
	alt_buffer = p;

	rotateDizz();

	return 0;
}
