/*
 * EffecTV - Realtime Digital Video Effector
 * Copyright (C) 2001-2002 FUKUCHI Kentaro
 *
 * PredatorTV - makes incoming objects invisible like the Predator.
 * Copyright (C) 2001-2002 FUKUCHI Kentaro
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
static RGB32 *bgimage;

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
	memcpy(bgimage, src, video_area*sizeof(RGB32));
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
	memcpy(tmp, src, video_area*sizeof(RGB32));
	video_grabframe();
/* step 4: add frame-4 to buffer-2 */
	video_syncframe();
	for(i=0; i<video_area; i++) {
		tmp[i] = (src[i]&tmp[i])+(((src[i]^tmp[i])&0xfefefe)>>1);
	}
	video_grabframe();
/* step 5: add buffer-3 to buffer-1 */
	for(i=0; i<video_area; i++) {
		bgimage[i] = (bgimage[i]&tmp[i])
			+(((bgimage[i]^tmp[i])&0xfefefe)>>1);
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
				memcpy(stretching_buffer, bgimage, video_area*sizeof(RGB32));
			}
			image_stretch_to_screen();
		} else {
			memcpy((RGB32 *)screen_getaddress(), bgimage,
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

effect *predatorRegister()
{
	effect *entry;
	
	sharedbuffer_reset();
	bgimage = (RGB32 *)sharedbuffer_alloc(video_area*sizeof(RGB32));
	if(bgimage == NULL) {
		return NULL;
	}

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
	image_set_threshold_y(MAGIC_THRESHOLD);
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
		state = 0;
	}
	return 0;
}

int predatorDraw()
{
	int x, y;
	unsigned char *diff;
	RGB32 *dest, *src;

	if(video_syncframe())
		return -1;
	diff = image_bgsubtract_y((RGB32 *)video_getaddress());
	if(video_grabframe())
		return -1;
	diff = image_diff_filter(diff);

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

	dest += video_width;
	diff += video_width;
	src = bgimage + video_width;
	for(y=1; y<video_height-1; y++) {
		for(x=0; x<video_width; x++) {
			if(*diff){
				*dest = src[4] & 0xfcfcfc;
			} else {
				*dest = *src;
			}
			diff++;
			src++;
			dest++;
		}
	}
	if(stretch) {
		image_stretch_to_screen();
	}
	if(screen_mustlock()) {
		screen_unlock();
	}

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
