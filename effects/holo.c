/*
 * EffecTV - Realtime Digital Video Effector
 * Copyright (C) 2001 FUKUCHI Kentarou
 *
 * HolographicTV - Holographic vision
 * Copyright (C) 2001 FUKUCHI Kentarou
 *
 */

#include <stdlib.h>
#include <string.h>
#include "../EffecTV.h"
#include "utils.h"

int holoStart();
int holoStop();
int holoDraw();
int holoEvent(SDL_Event *);

#define MAGIC_THRESHOLD 40

static char *effectname = "HolographicTV";
static int state = 0;
static RGB32 *bgimage;
static unsigned int noisepattern[256];

static int setBackground()
{
	int i;
	RGB32 *src, *tmp;

	screen_clear(0);
	tmp = (RGB32 *)malloc(video_area * sizeof(RGB32));
	if(tmp == NULL)
		return -1;

/*
 * grabs 4 frames and composites them to get a quality background image
 */
/* step 1: grab frame-1 to buffer-1 */
	video_syncframe();
	src = (RGB32 *)video_getaddress();
	memcpy(src, bgimage, video_area*sizeof(RGB32));
	video_grabframe();
/* step 2: add frame-2 to buffer-1 */
	video_syncframe();
	for(i=0; i<video_area; i++) {
		bgimage[i] = (src[i]&bgimage[i])+(((src[i]^bgimage[i])&0xfefefe)>>1);
	}
	video_grabframe();
/* step 3: grab frame-3 to buffer-2 */
	video_syncframe();
	src = (RGB32 *)video_getaddress();
	memcpy(src, tmp, video_area*sizeof(RGB32));
	video_grabframe();
/* step 4: add frame-4 to buffer-2 */
	video_syncframe();
	for(i=0; i<video_area; i++) {
		tmp[i] = (src[i]&tmp[i])+(((src[i]^tmp[i])&0xfefefe)>>1);
	}
	video_grabframe();
/* step 5: add buffer-3 to buffer-1 */
	for(i=0; i<video_area; i++) {
		bgimage[i] = ((bgimage[i]&tmp[i])
			+(((bgimage[i]^tmp[i])&0xfefefe)>>1))&0xfefefe;
	}
	image_bgset_y(bgimage);

	for(i=0; i<2; i++) {
		if(screen_mustlock()) {
			if(screen_lock() < 0) {
				break;
			}
		}
		if(stretch) {
			if(i == 0) {
				memcpy(bgimage, stretching_buffer, video_area*sizeof(RGB32));
			}
			image_stretch_to_screen();
		} else {
			memcpy(bgimage, (RGB32 *)screen_getaddress(),
					video_area*sizeof(RGB32));
		}
		if(screen_mustlock()) {
			screen_unlock();
		}
		screen_update();
		if(doublebuf == 0)
			break;
	}
	free(tmp);

	return 0;
}

static void holoInit()
{
	int i;

	for(i=0; i<256; i++) {
		noisepattern[i] = (i * i * i / 40000)* i / 256;
	}
}

effect *holoRegister()
{
	effect *entry;
	
	sharedbuffer_reset();
	bgimage = (RGB32 *)sharedbuffer_alloc(video_area*sizeof(RGB32));
	if(bgimage == NULL) {
		return NULL;
	}
	holoInit();

	entry = (effect *)malloc(sizeof(effect));
	if(entry == NULL) {
		return NULL;
	}
	
	entry->name = effectname;
	entry->start = holoStart;
	entry->stop = holoStop;
	entry->draw = holoDraw;
	entry->event = holoEvent;

	return entry;
}

int holoStart()
{
	image_set_threshold_y(MAGIC_THRESHOLD);
	if(video_grabstart())
		return -1;
	if(setBackground())
		return -1;

	state = 1;
	return 0;
}

int holoStop()
{
	if(state) {
		video_grabstop();
		state = 0;
	}
	return 0;
}

int holoDraw()
{
	static int phase=0;
	int x, y;
	unsigned char *diff;
	RGB32 *src, *dest, *bg;
	RGB32 s, t;
	int r, g, b;

	if(video_syncframe())
		return -1;
	src = (RGB32 *)video_getaddress();
	diff = image_diff_filter(image_bgsubtract_y(src));

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
	if(stretch) {
		image_stretch_to_screen();
	}
	if(screen_mustlock()) {
		screen_unlock();
	}
	if(video_grabframe())
		return -1;
	phase-=37;

	return 0;
}

int holoEvent(SDL_Event *event)
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
