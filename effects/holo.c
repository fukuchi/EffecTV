/*
 * EffecTV - Realtime Digital Video Effector
 * Copyright (C) 2001-2006 FUKUCHI Kentaro
 *
 * HolographicTV - Holographic vision
 * Copyright (C) 2001-2002 FUKUCHI Kentaro
 *
 */

#include <stdlib.h>
#include <string.h>
#include "EffecTV.h"
#include "utils.h"

static int start(void);
static int stop(void);
static int draw(RGB32 *src, RGB32 *dest);
static int event(SDL_Event *);

#define MAGIC_THRESHOLD 40

static char *effectname = "HolographicTV";
static int state = 0;
static RGB32 *bgimage;
static int bgIsSet;
static unsigned int noisepattern[256];

static void setBackground(RGB32 *src)
{
	memcpy(bgimage, src, video_area * PIXEL_SIZE);
	image_bgset_y(bgimage);
	bgIsSet = 1;
}

static void holoInit(void)
{
	int i;

	for(i=0; i<256; i++) {
		noisepattern[i] = (i * i * i / 40000)* i / 256;
	}
}

effect *holoRegister(void)
{
	effect *entry;
	
	bgimage = (RGB32 *)malloc(video_area*PIXEL_SIZE);
	if(bgimage == NULL) {
		return NULL;
	}
	holoInit();

	entry = (effect *)malloc(sizeof(effect));
	if(entry == NULL) {
		return NULL;
	}
	
	entry->name = effectname;
	entry->start = start;
	entry->stop = stop;
	entry->draw = draw;
	entry->event = event;

	return entry;
}

static int start(void)
{
	image_set_threshold_y(MAGIC_THRESHOLD);
	bgIsSet = 0;

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
	static int phase=0;
	int x, y;
	unsigned char *diff;
	RGB32 *bg;
	RGB32 s, t;
	int r, g, b;

	if(!bgIsSet) {
		setBackground(src);
	}

	diff = image_diff_filter(image_bgsubtract_y(src));

	diff += video_width;
	dest += video_width;
	src += video_width;
	bg = bgimage + video_width;
	for(y=1; y<video_height-1; y++) {
		if(((y+phase) & 0x7f)<0x58) {
			for(x=0; x<video_width; x++) {
				if(*diff){
					s = *src;
					t = (s & 0xff) + ((s & 0xff00)>>7) + ((s & 0xff0000)>>16);
					t += noisepattern[inline_fastrand()>>24];
					r = ((s & 0xff0000)>>17) + t;
					g = ((s & 0xff00)>>8) + t;
					b = (s & 0xff) + t;
					r = (r>>1)-100;
					g = (g>>1)-100;
					b = b>>2;
					if(r<20) r=20;
					if(g<20) g=20;
					s = *bg;
					r += (s&0xff0000)>>17;
					g += (s&0xff00)>>9;
					b += ((s&0xff)>>1)+40;
					if(r>255) r = 255;
					if(g>255) g = 255;
					if(b>255) b = 255;
					*dest = r<<16|g<<8|b;
				} else {
					*dest = *bg;
				}
				diff++;
				src++;
				dest++;
				bg++;
			}
		} else {
			for(x=0; x<video_width; x++) {
				if(*diff){
					s = *src;
					t = (s & 0xff) + ((s & 0xff00)>>6) + ((s & 0xff0000)>>16);
					t += noisepattern[inline_fastrand()>>24];
					r = ((s & 0xff0000)>>16) + t;
					g = ((s & 0xff00)>>8) + t;
					b = (s & 0xff) + t;
					r = (r>>1)-100;
					g = (g>>1)-100;
					b = b>>2;
					if(r<0) r=0;
					if(g<0) g=0;
					s = *bg;
					r += ((s&0xff0000)>>17) + 10;
					g += ((s&0xff00)>>9) + 10;
					b += ((s&0xff)>>1) + 40;
					if(r>255) r = 255;
					if(g>255) g = 255;
					if(b>255) b = 255;
					*dest = r<<16|g<<8|b;
				} else {
					*dest = *bg;
				}
				diff++;
				src++;
				dest++;
				bg++;
			}
		}
	}
	phase-=37;

	return 0;
}

static int event(SDL_Event *event)
{
	if(event->type == SDL_KEYDOWN) {
		switch(event->key.keysym.sym) {
		case SDLK_SPACE:
			bgIsSet = 0;
			break;
		default:
			break;
		}
	}
	return 0;
}
