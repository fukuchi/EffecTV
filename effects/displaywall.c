/*
 * EffecTV - Realtime Digital Video Effector
 * Copyright (C) 2001-2006 FUKUCHI Kentaro
 *
 * DisplayWall
 * Copyright (C) 2005-2006 FUKUCHI Kentaro
 *
 */

#include <stdlib.h>
#include <string.h>
#include "EffecTV.h"
#include "utils.h"

static int start(void);
static int stop(void);
static int draw(RGB32 *src, RGB32 *dest);
static int event(SDL_Event *event);

static char *effectname = "DisplayWall";
static int state = 0;

static int *vecx;
static int *vecy;
static int scale;
static int dx, dy;
static int bx, by;
static int speed;
static int speedi;
static int cx, cy;

static void initVec(void);

effect *displayWallRegister(void)
{
	effect *entry;

	vecx = (int *)malloc(sizeof(int) * video_area);
	vecy = (int *)malloc(sizeof(int) * video_area);
	if(vecx == NULL || vecy == NULL) {
		return NULL;
	}

	entry = (effect *)malloc(sizeof(effect));
	if(entry == NULL) {
		free(vecx);
		free(vecy);
		return NULL;
	}

	entry->name = effectname;
	entry->start = start;
	entry->stop = stop;
	entry->draw = draw;
	entry->event = event;

	scale = 3;
	dx = 0;
	dy = 0;
	speed = 10;
	cx = video_width / 2;
	cy = video_height / 2;

	initVec();

	return entry;
}

static void initVec(void)
{
	int x, y, i;
	double vx, vy;

	i = 0;
	for(y=0; y<video_height; y++) {
		for(x=0; x<video_width; x++) {
			vx = (double)(x - cx) / video_width;
			vy = (double)(y - cy) / video_width;

			vx *= 1.0 - vx * vx * 0.4;
			vy *= 1.0 - vx * vx * 0.8;
			vx *= 1.0 - (double)y / video_height * 0.15;
			vecx[i] = vx * video_width;
			vecy[i] = vy * video_width;

			i++;
		}
	}
}

static int start(void)
{
	state = 1;

	return 0;
}

static int stop(void)
{
	state = 0;

	return 0;
}

static int draw(RGB32 *src, RGB32 *dest)
{
	int x, y, i;
	int px, py;

	speed += speedi;
	if(speed < 0) speed = 0;

	bx += dx * speed;
	by += dy * speed;
	while(bx < 0) bx += video_width;
	while(bx >= video_width) bx -= video_width;
	while(by < 0) by += video_height;
	while(by >= video_height) by -= video_height;

	if(scale == 1) {
		bx = cx;
		by = cy;
	}

	i = 0;
	for(y=0; y<video_height; y++) {
		for(x=0; x<video_width; x++) {
			px = bx + vecx[i] * scale;
			py = by + vecy[i] * scale;
			while(px < 0) px += video_width;
			while(px >= video_width) px -= video_width;
			while(py < 0) py += video_height;
			while(py >= video_height) py -= video_height;

			dest[i++] = src[py * video_width + px];
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
		case SDLK_5:
		case SDLK_6:
		case SDLK_7:
		case SDLK_8:
		case SDLK_9:
			scale = event->key.keysym.sym - SDLK_0;
			break;
		case SDLK_h:
			dy = 0;
			dx = 1;
			break;
		case SDLK_k:
			dy = 1;
			dx = 0;
			break;
		case SDLK_j:
			dy = -1;
			dx = 0;
			break;
		case SDLK_l:
			dy = 0;
			dx = -1;
			break;
		case SDLK_PAGEUP:
			speedi = 1;
			break;
		case SDLK_PAGEDOWN:
			speedi = -1;
			break;
		case SDLK_SPACE:
			speed = 0;
			break;
		default:
			break;
		}
	} else if(event->type == SDL_KEYUP) {
		switch(event->key.keysym.sym) {
		case SDLK_PAGEUP:
		case SDLK_PAGEDOWN:
			speedi = 0;
			break;
		default:
			break;
		}
	}
	return 0;
}
