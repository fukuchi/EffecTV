/*
 * EffecTV - Realtime Digital Video Effector
 * Copyright (C) 2001 FUKUCHI Kentarou
 *
 * predator.c: become invisible.
 *
 */

#include <stdlib.h>
#include <string.h>
#include "../EffecTV.h"
#include "utils.h"

int predatorStart();
int predatorStop();
int predatorDraw();
int predatorEvent(SDL_Event *);

#define MAGIC_THRESHOLD 40

static char *effectname = "PredatorTV";
static int state = 0;
static int format;
static unsigned int *bgimage;
static unsigned char *bgvalue;
static unsigned char *buffer;

static unsigned int trunc(int v)
{
	if(v<0) return 0;
	if(v>255) return 255;
	return v;
}

static int setBackground()
{
	int i, v, x, y;
	unsigned int *src;
	unsigned int *dest;

	screen_clear(0);
	dest = (unsigned int *)bgvalue;

	video_grabstop();
	if(video_setformat(VIDEO_PALETTE_RGB32))
		return -1;
	if(video_grabstart())
		return -1;
/*
 * grabs 4 frames and composites them to get a quality background image
 */
/* step 1: grab frame-1 to buffer-1 */
	video_syncframe();
	src = (unsigned int *)video_getaddress();
	bcopy(src, bgimage, SCREEN_WIDTH*SCREEN_HEIGHT*4);
	video_grabframe();
/* step 2: add frame-2 to buffer-1 */
	video_syncframe();
	for(i=0; i<SCREEN_WIDTH*SCREEN_HEIGHT; i++) {
		bgimage[i] = (src[i]&bgimage[i])+(((src[i]^bgimage[i])&0xfefefe)>>1);
	}
	video_grabframe();
/* step 3: grab frame-3 to buffer-2 */
	video_syncframe();
	src = (unsigned int *)video_getaddress();
	bcopy(src, dest, SCREEN_WIDTH*SCREEN_HEIGHT*4);
	video_grabframe();
/* step 4: add frame-4 to buffer-2 */
	video_syncframe();
	for(i=0; i<SCREEN_WIDTH*SCREEN_HEIGHT; i++) {
		dest[i] = (src[i]&dest[i])+(((src[i]^dest[i])&0xfefefe)>>1);
	}
	video_grabframe();
/* step 5: add buffer-3 to buffer-1 */
	for(i=0; i<SCREEN_WIDTH*SCREEN_HEIGHT; i++) {
		bgimage[i] = (bgimage[i]&dest[i])+(((bgimage[i]^dest[i])&0xfefefe)>>1);
		bgvalue[i] = RGBtoY(bgimage[i]);
	}
	if(video_grabstop())
		return -1;
	if(video_setformat(VIDEO_PALETTE_GREY))
		return -1;
	if(video_grabstart())
		return -1;

	for(i=0; i<2; i++) {
		if(screen_mustlock()) {
			if(screen_lock() < 0) {
				return 0;
			}
		}
		dest = (unsigned int *)screen_getaddress();
		if(scale == 2) {
			for(y=0; y<SCREEN_HEIGHT; y++) {
				for(x=0; x<SCREEN_WIDTH; x++) {
					v = bgimage[y*SCREEN_WIDTH+x];
					dest[y*2*SCREEN_WIDTH*2+x*2] = v;
					dest[y*2*SCREEN_WIDTH*2+x*2+1] = v;
					dest[(y*2+1)*SCREEN_WIDTH*2+x*2] = v;
					dest[(y*2+1)*SCREEN_WIDTH*2+x*2+1] = v;
				}
			}
		} else {
			bcopy(bgimage, dest, SCREEN_WIDTH*SCREEN_HEIGHT*4);
		}
		if(screen_mustlock()) {
			screen_unlock();
		}
		screen_update();
		if(doublebuf == 0)
			break;
	}

	return 0;
}

effect *predatorRegister()
{
	effect *entry;
	
	yuvTableInit();

	sharedbuffer_reset();
	bgimage = (unsigned int *)sharedbuffer_alloc(SCREEN_WIDTH*SCREEN_HEIGHT*8);
	if(bgimage == NULL) {
		return NULL;
	}
	bgvalue = (unsigned char *)bgimage + SCREEN_WIDTH*SCREEN_HEIGHT*4;
	buffer = (unsigned char *)bgvalue + SCREEN_WIDTH*SCREEN_HEIGHT;

	entry = (effect *)malloc(sizeof(effect));
	if(entry == NULL) {
		return NULL;
	}
	
	entry->name = effectname;
	entry->start = predatorStart;
	entry->stop = predatorStop;
	entry->draw = predatorDraw;
	entry->event = predatorEvent;

	return entry;
}

int predatorStart()
{
	format = video_getformat();
	if(video_grabstart())
		return -1;
	if(setBackground())
		return -1;

	state = 1;
	return 0;
}

int predatorStop()
{
	if(state) {
		video_grabstop();
		video_setformat(format);
		state = 0;
	}
	return 0;
}

int predatorDraw()
{
	int i, x, y;
	unsigned int v;
	unsigned char *src;
	unsigned int *dest;

	if(video_syncframe())
		return -1;
	src = video_getaddress();
	dest = (unsigned int *)screen_getaddress();

	for(i=0; i<SCREEN_WIDTH*SCREEN_HEIGHT; i++) {
		v = ((int)src[i] - (int)bgvalue[i] + MAGIC_THRESHOLD)&0x7fffffff;
		buffer[i] = (v - MAGIC_THRESHOLD*2)>>31;
	}
	if(video_grabframe())
		return -1;
	if(screen_mustlock()) {
		if(screen_lock() < 0) {
			return 0;
		}
	}
	i = SCREEN_WIDTH+1;
	if(scale == 2) {
		for(y=1; y<SCREEN_HEIGHT-1; y++) {
			for(x=1; x<SCREEN_WIDTH-1; x++) {
				if(buffer[i-SCREEN_WIDTH]
				  +buffer[i-1]+buffer[i]+buffer[i+1]
				  +buffer[i+SCREEN_WIDTH] > 0) {
					v = bgimage[i];
				} else {
					v = bgimage[i+4]&0xfcfcfc;
				}
				dest[y*2*SCREEN_WIDTH*2+x*2] = v;
				dest[y*2*SCREEN_WIDTH*2+x*2+1] = v;
				dest[(y*2+1)*SCREEN_WIDTH*2+x*2] = v;
				dest[(y*2+1)*SCREEN_WIDTH*2+x*2+1] = v;
				i++;
			}
			i+=2;
		}
	} else {
		for(y=1; y<SCREEN_HEIGHT-1; y++) {
			for(x=1; x<SCREEN_WIDTH-1; x++) {
				if(buffer[i-SCREEN_WIDTH]
				  +buffer[i-1]+buffer[i]+buffer[i+1]
				  +buffer[i+SCREEN_WIDTH] > 0) {
					dest[i] = bgimage[i];
				} else {
					dest[i] = bgimage[i+4]&0xfcfcfc;
				}
				i++;
			}
			i+=2;
		}
	}
	if(screen_mustlock()) {
		screen_unlock();
	}
	screen_update();

	return 0;
}

int predatorEvent(SDL_Event *event)
{
	if(event->type == SDL_KEYDOWN) {
		switch(event->key.keysym.sym) {
		case SDLK_SPACE:
			setBackground();
			break;
		default:
			break;
		}
	}
	return 0;
}
