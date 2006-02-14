/*
 * EffecTV - Realtime Digital Video Effector
 * Copyright (C) 2001-2006 FUKUCHI Kentaro
 *
 * PredatorTV - makes incoming objects invisible like the Predator.
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

static char *effectname = "PredatorTV";
static int state = 0;
static RGB32 *bgimage;

static int setBackground(void)
{
	int i;
	RGB32 *src, *tmp;

	tmp = (RGB32 *)malloc(video_area * PIXEL_SIZE);
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
		if(screen_lock() < 0) {
			break;
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
		screen_unlock();
		screen_update();
		if(doublebuf == 0)
			break;
	}
	free(tmp);

	return 0;
}

effect *predatorRegister(void)
{
	effect *entry;
	
	sharedbuffer_reset();
	bgimage = (RGB32 *)sharedbuffer_alloc(video_area*PIXEL_SIZE);
	if(bgimage == NULL) {
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

	return entry;
}

static int start(void)
{
	image_set_threshold_y(MAGIC_THRESHOLD);
	if(setBackground())
		return -1;

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
	int x, y;
	unsigned char *diff;

	diff = image_bgsubtract_y(src);
	diff = image_diff_filter(diff);

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

	return 0;
}

static int event(SDL_Event *event)
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
