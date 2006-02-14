/*
 * EffecTV - Realtime Digital Video Effector
 * Copyright (C) 2001-2006 FUKUCHI Kentaro
 *
 * SimuraTV - color distortion and mirrored image effector
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
static int event(SDL_Event *event);

static char *effectname = "SimuraTV";
static int stat;
static RGB32 color = 0x000000;
static int mirror = 1;
static int width;
static int hwidth;
static int height;
static int hheight;
static RGB32 colortable[26] = {
	0x000080, 0x0000e0, 0x0000ff,
	0x008000, 0x00e000, 0x00ff00,
	0x008080, 0x00e0e0, 0x00ffff,
	0x800000, 0xe00000, 0xff0000,
	0x800080, 0xe000e0, 0xff00ff,
	0x808000, 0xe0e000, 0xffff00,
	0x808080, 0xe0e0e0, 0xffffff,
	0x76ca0a, 0x3cafaa, 0x60a848, 0x504858, 0x89ba43
};
static const char keytable[26] = {
	'q', 'a', 'z',
	'w', 's', 'x',
	'e', 'd', 'c',
	'r', 'f', 'v',
	't', 'g', 'b',
	'y', 'h', 'n',
	'u', 'j', 'm',
	'i', 'k', 'o', 'l', 'p'
};
static void mirror_no(RGB32 *, RGB32 *);
static void mirror_u(RGB32 *, RGB32 *);
static void mirror_d(RGB32 *, RGB32 *);
static void mirror_r(RGB32 *, RGB32 *);
static void mirror_l(RGB32 *, RGB32 *);
static void mirror_ul(RGB32 *, RGB32 *);
static void mirror_ur(RGB32 *, RGB32 *);
static void mirror_dl(RGB32 *, RGB32 *);
static void mirror_dr(RGB32 *, RGB32 *);

effect *simuraRegister(void)
{
	effect *entry;
	int i;
	RGB32 tmp[26];
	
	for(i=0; i<26; i++) {
		tmp[keytable[i] - 'a'] = colortable[i];
	}
	for(i=0; i<26; i++) {
		colortable[i] = tmp[i];
	}

	width = video_width;
	height = video_height;
	hwidth = video_width/2;
	hheight = video_height/2;

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
	stat = 1;
	return 0;
}

static int stop(void)
{
	stat = 0;
	return 0;
}

static int draw(RGB32 *src, RGB32 *dest)
{
	switch(mirror) {
	case 1:
		mirror_l(src, dest);
		break;
	case 2:
		mirror_r(src, dest);
		break;
	case 3:
		mirror_d(src, dest);
		break;
	case 4:
		mirror_dl(src, dest);
		break;
	case 5:
		mirror_dr(src, dest);
		break;
	case 6:
		mirror_u(src, dest);
		break;
	case 7:
		mirror_ul(src, dest);
		break;
	case 8:
		mirror_ur(src, dest);
		break;
	case 0:
	default:
		mirror_no(src, dest);
		break;
	}

	return 0;
}


static int event(SDL_Event *event)
{
	if(event->type == SDL_KEYDOWN) {
		switch(event->key.keysym.sym) {
		case SDLK_a:
		case SDLK_b:
		case SDLK_c:
		case SDLK_d:
		case SDLK_e:
		case SDLK_f:
		case SDLK_g:
		case SDLK_h:
		case SDLK_i:
		case SDLK_j:
		case SDLK_k:
		case SDLK_l:
		case SDLK_m:
		case SDLK_n:
		case SDLK_o:
		case SDLK_p:
		case SDLK_q:
		case SDLK_r:
		case SDLK_s:
		case SDLK_t:
		case SDLK_u:
		case SDLK_v:
		case SDLK_w:
		case SDLK_x:
		case SDLK_y:
		case SDLK_z:
			color = colortable[event->key.keysym.sym - SDLK_a];
			break;
		case SDLK_1:
		case SDLK_2:
		case SDLK_3:
		case SDLK_4:
		case SDLK_5:
		case SDLK_6:
		case SDLK_7:
		case SDLK_8:
		case SDLK_9:
			mirror = event->key.keysym.sym - SDLK_1;
			break;
		case SDLK_KP1:
		case SDLK_KP2:
		case SDLK_KP3:
		case SDLK_KP4:
		case SDLK_KP5:
		case SDLK_KP6:
		case SDLK_KP7:
		case SDLK_KP8:
		case SDLK_KP9:
			mirror = event->key.keysym.sym - SDLK_KP1;
			break;
		case SDLK_SPACE:
			color = 0;
			break;
		default:
			break;
		}
	}
	return 0;
}

static void mirror_no(RGB32 *src, RGB32 *dest)
{
	int i;

	for(i=0; i<video_area; i++) {
		dest[i] = src[i] ^ color;
	}
}

static void mirror_u(RGB32 *src, RGB32 *dest)
{
	int x, y;

	for(y=0; y<hheight; y++) {
		for(x=0; x<width; x++) {
			dest[y*width+x] = src[y*width+x] ^ color;
			dest[(height-y-1)*width+x] = src[y*width+x] ^ color;
		}
	}
}

static void mirror_d(RGB32 *src, RGB32 *dest)
{
	int x, y;

	for(y=hheight; y<height; y++) {
		for(x=0; x<width; x++) {
			dest[y*width+x] = src[y*width+x] ^ color;
			dest[(height-y-1)*width+x] = src[y*width+x] ^ color;
		}
	}
}

static void mirror_l(RGB32 *src, RGB32 *dest)
{
	int x, y;

	for(y=0; y<height; y++) {
		for(x=0; x<hwidth; x++) {
			dest[y*width+x] = src[y*width+x] ^ color;
			dest[y*width+(width-x-1)] = src[y*width+x] ^ color;
		}
	}
}

static void mirror_r(RGB32 *src, RGB32 *dest)
{
	int x, y;

	for(y=0; y<height; y++) {
		for(x=hwidth; x<width; x++) {
			dest[y*width+x] = src[y*width+x] ^ color;
			dest[y*width+(width-x-1)] = src[y*width+x] ^ color;
		}
	}
}

static void mirror_ul(RGB32 *src, RGB32 *dest)
{
	int x, y;

	for(y=0; y<hheight; y++) {
		for(x=0; x<hwidth; x++) {
			dest[y*width+x] = src[y*width+x] ^ color;
			dest[y*width+(width-x-1)] = src[y*width+x] ^ color;
			dest[(height-y-1)*width+x] = src[y*width+x] ^ color;
			dest[(height-y-1)*width+(width-x-1)] = src[y*width+x] ^ color;
		}
	}
}

static void mirror_ur(RGB32 *src, RGB32 *dest)
{
	int x, y;

	for(y=0; y<hheight; y++) {
		for(x=hwidth; x<width; x++) {
			dest[y*width+x] = src[y*width+x] ^ color;
			dest[y*width+(width-x-1)] = src[y*width+x] ^ color;
			dest[(height-y-1)*width+x] = src[y*width+x] ^ color;
			dest[(height-y-1)*width+(width-x-1)] = src[y*width+x] ^ color;
		}
	}
}

static void mirror_dl(RGB32 *src, RGB32 *dest)
{
	int x, y;

	for(y=hheight; y<height; y++) {
		for(x=0; x<hwidth; x++) {
			dest[y*width+x] = src[y*width+x] ^ color;
			dest[y*width+(width-x-1)] = src[y*width+x] ^ color;
			dest[(height-y-1)*width+x] = src[y*width+x] ^ color;
			dest[(height-y-1)*width+(width-x-1)] = src[y*width+x] ^ color;
		}
	}
}

static void mirror_dr(RGB32 *src, RGB32 *dest)
{
	int x, y;

	for(y=hheight; y<height; y++) {
		for(x=hwidth; x<width; x++) {
			dest[y*width+x] = src[y*width+x] ^ color;
			dest[y*width+(width-x-1)] = src[y*width+x] ^ color;
			dest[(height-y-1)*width+x] = src[y*width+x] ^ color;
			dest[(height-y-1)*width+(width-x-1)] = src[y*width+x] ^ color;
		}
	}
}

