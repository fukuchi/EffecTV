/*
 * EffecTV - Realtime Digital Video Effector
 * Copyright (C) 2001-2002 FUKUCHI Kentaro
 *
 * LifeTV - Play John Horton Conway's `Life' game with video input.
 * Copyright (C) 2001-2002 FUKUCHI Kentaro
 *
 * This idea is stolen from Nobuyuki Matsushita's installation program of
 * ``HoloWall''. (See http://www.csl.sony.co.jp/person/matsu/index.html)
 */

#include <stdlib.h>
#include <string.h>
#include "../EffecTV.h"
#include "utils.h"

int lifeStart();
int lifeStop();
int lifeDraw();
int lifeEvent();

static char *effectname = "LifeTV";
static int stat;
static unsigned char *field, *field1, *field2;

static void clear_field()
{
	bzero(field1, video_area);
}

effect *lifeRegister()
{
	effect *entry;
	
	sharedbuffer_reset();
	field = (unsigned char *)sharedbuffer_alloc(video_area*2);
	if(field == NULL) {
		return NULL;
	}

	entry = (effect *)malloc(sizeof(effect));
	if(entry == NULL) {
		return NULL;
	}
	
	entry->name = effectname;
	entry->start = lifeStart;
	entry->stop = lifeStop;
	entry->draw = lifeDraw;
	entry->event = lifeEvent;

	return entry;
}

int lifeStart()
{
	screen_clear(0);
	image_stretching_buffer_clear(0);
	image_set_threshold_y(40);
	field1 = field;
	field2 = field + video_area;
	clear_field();
	if(video_grabstart())
		return -1;

	stat = 1;
	return 0;
}

int lifeStop()
{
	if(stat) {
		video_grabstop();
		stat = 0;
	}

	return 0;
}

int lifeDraw()
{
	int x, y;
	RGB32 *src, *dest;
	unsigned char *p, *q, v;
	unsigned char sum, sum1, sum2, sum3;
	int width;
	RGB32 pix;

	width = video_width;

	if(video_syncframe())
		return -1;
	src = (RGB32 *)video_getaddress();
	p = image_diff_filter(image_bgsubtract_update_y(src));
	for(x=0; x<video_area; x++) {
		field1[x] |= p[x];
	}

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

	p = field1 + 1;
	q = field2 + width + 1;
	dest += width + 1;
	src += width + 1;
/* each value of cell is 0 or 0xff. 0xff can be treated as -1, so
 * following equations treat each value as negative number. */
	for(y=1; y<video_height-1; y++) {
		sum1 = 0;
		sum2 = p[0] + p[width] + p[width*2];
		for(x=1; x<width-1; x++) {
			sum3 = p[1] + p[width+1] + p[width*2+1];
			sum = sum1 + sum2 + sum3;
			v = 0 - ((sum==0xfd)|((p[width]!=0)&(sum==0xfc)));
			*q++ = v;
			pix = (signed char)v;
//			pix = pix >> 8;
			*dest++ = pix | *src++;
			sum1 = sum2;
			sum2 = sum3;
			p++;
		}
		p += 2;
		q += 2;
		src += 2;
		dest += 2;
	}
	if(stretch) {
		image_stretch_to_screen();
	}
	p = field1;
	field1 = field2;
	field2 = p;
	if(screen_mustlock()) {
		screen_unlock();
	}
	if(video_grabframe())
		return -1;

	return 0;
}

int lifeEvent(SDL_Event *event)
{
	if(event->type == SDL_KEYDOWN) {
		switch(event->key.keysym.sym) {
		case SDLK_SPACE:
			clear_field();
			break;
		default:
			break;
		}
	}
	return 0;
}
