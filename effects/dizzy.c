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
int dizzyEvent();

static char *effectname = "DizzyTV";
static int state = 0;
static RGB32 *buffer;
static RGB32 *current_buffer, *alt_buffer;
static int dx, dy;
static int sx, sy;
static double phase = 0.0;
static double phase_increment = 0.02;
static double zoomrate = 1.01;

static void setParams()
{
	double vx, vy;
	double t;
	double x, y;
	double dizz;

	dizz = sin(phase) * 10 + sin(phase*1.9+5) * 5;

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
	sx = (-vx * x + vy * y + x + cos(phase*5) * 2) * 65536;
	sy = (-vx * y - vy * x + y + sin(phase*6) * 2) * 65536;

	phase += phase_increment;
	if(phase > 5700000) phase = 0;
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
	entry->event = dizzyEvent;

	return entry;
}

int dizzyStart()
{
	bzero(buffer, video_area * 2 * sizeof(RGB32));
	current_buffer = buffer;
	alt_buffer = buffer + video_area;
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
	int i;

	if(video_syncframe())
		return -1;
	src = (RGB32 *)video_getaddress();

	setParams();
	p = alt_buffer;
	for(y=video_height; y>0; y--) {
		ox = sx;
		oy = sy;
		for(x=video_width; x>0; x--) {
			i = (oy>>16)*video_width + (ox>>16);
			if(i<0) i = 0;
			if(i>=video_area) i = video_area;
			v = current_buffer[i] & 0xfcfcff;
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

	return 0;
}

int dizzyEvent(SDL_Event *event)
{
	if(event->type == SDL_KEYDOWN) {
		switch(event->key.keysym.sym) {
		case SDLK_SPACE:
			phase = 0.0;
			phase_increment = 0.02;
			zoomrate = 1.01;
			break;

        case SDLK_INSERT:
			phase_increment += 0.01;
            break;
            
        case SDLK_DELETE:
			phase_increment -= 0.01;
			if(phase_increment < 0.01) {
				phase_increment = 0.01;
			}
            break;

        case SDLK_PAGEUP:
			zoomrate += 0.01;
			if(zoomrate > 1.1) {
				zoomrate = 1.1;
			}
            break;
            
        case SDLK_PAGEDOWN:
			zoomrate -= 0.01;
			if(zoomrate < 1.01) {
				zoomrate = 1.01;
			}
            break;
            
		default:
			break;
		}
	}
    
	return 0;
}
