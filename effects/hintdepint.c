/*
 * EffecTV - Realtime Digital Video Effector
 * Copyright (C) 2001-2002 FUKUCHI Kentarou
 *
 * HintDePintTV - multi-resolutional mozaic.
 * Copyright (C) 2002 Jun IIO
 *
 */

#include <stdlib.h>
#include <string.h>
#include "../EffecTV.h"
#include "utils.h"

int hintdepintStart();
int hintdepintStop();
int hintdepintDraw();
int hintdepintEvent();

#define FRAME_COUNTS 		10
#define MIN_FRAME_COUNTS 	1
#define MAX_FRAME_COUNTS 	100

static void initialize();
static void makeMosaic(RGB32*, RGB32*, int);

static char *effectname = "HintDePintTV";
static int state = 0;

static int cellsizeTable[] = { 1, 5, 10, 20, 40, 80, 0 };
static int cellsizeIndex;
static int counter = 0;
static int frameCounts = FRAME_COUNTS;

effect *hintdepintRegister()
{
	effect *entry;

	entry = (effect *)malloc(sizeof(effect));
	if(entry == NULL) return NULL;
	
	entry->name = effectname;
	entry->start = hintdepintStart;
	entry->stop = hintdepintStop;
	entry->draw = hintdepintDraw;
	entry->event = hintdepintEvent;

	return entry;
}

int hintdepintStart()
{
	if(video_grabstart())
		return -1;
	state = 1;

	initialize();
	return 0;
}

int hintdepintStop()
{
	if(state) {
		video_grabstop();
		state = 0;
	}

	return 0;
}

int hintdepintDraw()
{
	int cellsize;
	RGB32 *src, *dest;

	if(video_syncframe())
		return -1;
	if(screen_mustlock()) {
		if(screen_lock() < 0) {
			return video_grabframe();
		}
	}

	src = (RGB32 *)video_getaddress();
	cellsize = cellsizeTable[cellsizeIndex];
	counter++;
	if(counter == frameCounts) {
		counter = 0;
		cellsizeIndex--;
		if(cellsizeIndex < 0) initialize();
	}
	
	if(stretch) {
		dest = stretching_buffer;
	} else {
		dest = (RGB32 *)screen_getaddress();
	}
	if(cellsize == 1) {
		bcopy(src, dest, video_area * sizeof(RGB32));
	} else {
		makeMosaic(dest, src, cellsize);
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

void initialize()
{
	int size;
	int i = 0;
	
	while ((size = cellsizeTable[i]) != 0) {
		if((size > video_width) || (size > video_height)) {
			 cellsizeTable[i] = 0;
		} else i++;
	}			
	cellsizeIndex = i-1;
	counter = 0;
}

void makeMosaic(RGB32 *dest, RGB32 *src, int cellsize)
{
	int x, y;

	for (y = 0; y < video_height; y += cellsize)
		for (x = 0; x < video_width; x += cellsize) {
			RGB32 v;
			int yy, xx;
			int dx = cellsize;
			int dy = cellsize;
			if (cellsize > video_width-x) { dx = video_width-x; }
			if (cellsize > video_height-y) { dy = video_height-y; }
			v = src[(y+dy/2)*video_width+(x+dx/2)];
			for (yy = y; yy < y + dy; yy++)
				for (xx = x; xx < x + dx; xx++) {
					dest[yy*video_width+xx] = v;
				}
		}
}

int hintdepintEvent(SDL_Event *event)
{
	if(event->type == SDL_KEYDOWN) {
		switch(event->key.keysym.sym) {
		case SDLK_SPACE:
			frameCounts = FRAME_COUNTS;
			initialize();
			break;
		case SDLK_MINUS:
		case SDLK_COMMA:
			counter = 0;
			frameCounts--;
			if (frameCounts < MIN_FRAME_COUNTS) {
				frameCounts = MIN_FRAME_COUNTS;
			}
			break;
		case SDLK_PAGEDOWN:
			counter = 0;
			frameCounts /= 2;
			if (frameCounts < MIN_FRAME_COUNTS) {
				frameCounts = MIN_FRAME_COUNTS;
			}
			break;
		case SDLK_PLUS:
		case SDLK_PERIOD:
			counter = 0;
			frameCounts++;
			if (frameCounts > MAX_FRAME_COUNTS) {
				frameCounts = MAX_FRAME_COUNTS;
			}
			break;
		case SDLK_PAGEUP:
			counter = 0;
			frameCounts *= 2;
			if (frameCounts > MAX_FRAME_COUNTS) {
				frameCounts = MAX_FRAME_COUNTS;
			}
			break;
		default:
			break;
		}
	}
	return 0;
}
