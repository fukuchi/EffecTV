/*
 * EffecTV - Realtime Digital Video Effector
 * Copyright (C) 2001 FUKUCHI Kentarou
 *
 * simura.c: color distortion and mirrored image effector
 *
 */

#include <stdlib.h>
#include <string.h>
#include "../EffecTV.h"
#include "utils.h"

int simuraStart();
int simuraStop();
int simuraDraw();
int simuraEvent();

static char *effectname = "SimuraTV";
static int stat;
static int color;
static int mirror;
static int width;
static int hwidth;
static int height;
static int hheight;
static int fulllength;
static int colortable[26] = {
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
static void mirror_no();
static void mirror_u();
static void mirror_d();
static void mirror_r();
static void mirror_l();
static void mirror_ul();
static void mirror_ur();
static void mirror_dl();
static void mirror_dr();

effect *simuraRegister()
{
	effect *entry;
	int i;
	int tmp[26];
	
	for(i=0; i<26; i++) {
		tmp[keytable[i] - 'a'] = colortable[i];
	}
	for(i=0; i<26; i++) {
		colortable[i] = tmp[i];
	}

	width = SCREEN_WIDTH*scale;
	height = SCREEN_HEIGHT*scale;
	hwidth = width/2;
	hheight = height/2;
	fulllength = width*height;

	entry = (effect *)malloc(sizeof(effect));
	if(entry == NULL) {
		return NULL;
	}
	
	entry->name = effectname;
	entry->start = simuraStart;
	entry->stop = simuraStop;
	entry->draw = simuraDraw;
	entry->event = simuraEvent;

	return entry;
}

int simuraStart()
{
	color = 0;
	mirror = 0;
	if(scale == 2){
		if(video_changesize(SCREEN_WIDTH*2, SCREEN_HEIGHT*2))
			return -1;
	}
	if(video_grabstart())
		return -1;

	stat = 1;
	return 0;
}

int simuraStop()
{
	if(stat) {
		video_grabstop();
		if(scale == 2){
			video_changesize(0, 0);
		}
		stat = 0;
	}

	return 0;
}

int simuraDraw()
{
	if(video_syncframe())
		return -1;
	if(screen_mustlock()) {
		if(screen_lock() < 0) {
			return 0;
		}
	}
	switch(mirror) {
	case 1:
		mirror_l();
		break;
	case 2:
		mirror_r();
		break;
	case 3:
		mirror_d();
		break;
	case 4:
		mirror_dl();
		break;
	case 5:
		mirror_dr();
		break;
	case 6:
		mirror_u();
		break;
	case 7:
		mirror_ul();
		break;
	case 8:
		mirror_ur();
		break;
	case 0:
	default:
		mirror_no();
		break;
	}
	if(screen_mustlock()) {
		screen_unlock();
	}
	screen_update();
	if(video_grabframe())
		return -1;

	return 0;
}


int simuraEvent(SDL_Event *event)
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

static void mirror_no()
{
	int i;
	unsigned int *src, *dest;

	src = (unsigned int *)video_getaddress();
	dest = (unsigned int *)screen_getaddress();
	for(i=0; i<fulllength; i++) {
		dest[i] = src[i] ^ color;
	}
}

static void mirror_u()
{
	int x, y;
	unsigned int *src, *dest;

	src = (unsigned int *)video_getaddress();
	dest = (unsigned int *)screen_getaddress();
	for(y=0; y<hheight; y++) {
		for(x=0; x<width; x++) {
			dest[y*width+x] = src[y*width+x] ^ color;
			dest[(height-y-1)*width+x] = src[y*width+x] ^ color;
		}
	}
}

static void mirror_d()
{
	int x, y;
	unsigned int *src, *dest;

	src = (unsigned int *)video_getaddress();
	dest = (unsigned int *)screen_getaddress();
	for(y=hheight; y<height-1; y++) {
		for(x=0; x<width; x++) {
			dest[y*width+x] = src[y*width+x] ^ color;
			dest[(height-y-1)*width+x] = src[y*width+x] ^ color;
		}
	}
}

static void mirror_l()
{
	int x, y;
	unsigned int *src, *dest;

	src = (unsigned int *)video_getaddress();
	dest = (unsigned int *)screen_getaddress();
	for(y=0; y<height; y++) {
		for(x=0; x<hwidth; x++) {
			dest[y*width+x] = src[y*width+x] ^ color;
			dest[y*width+(width-x-1)] = src[y*width+x] ^ color;
		}
	}
}

static void mirror_r()
{
	int x, y;
	unsigned int *src, *dest;

	src = (unsigned int *)video_getaddress();
	dest = (unsigned int *)screen_getaddress();
	for(y=0; y<height; y++) {
		for(x=hwidth; x<width-1; x++) {
			dest[y*width+x] = src[y*width+x] ^ color;
			dest[y*width+(width-x-1)] = src[y*width+x] ^ color;
		}
	}
}

static void mirror_ul()
{
	int x, y;
	unsigned int *src, *dest;

	src = (unsigned int *)video_getaddress();
	dest = (unsigned int *)screen_getaddress();
	for(y=0; y<hheight; y++) {
		for(x=0; x<hwidth; x++) {
			dest[y*width+x] = src[y*width+x] ^ color;
			dest[y*width+(width-x-1)] = src[y*width+x] ^ color;
			dest[(height-y-1)*width+x] = src[y*width+x] ^ color;
			dest[(height-y-1)*width+(width-x-1)] = src[y*width+x] ^ color;
		}
	}
}

static void mirror_ur()
{
	int x, y;
	unsigned int *src, *dest;

	src = (unsigned int *)video_getaddress();
	dest = (unsigned int *)screen_getaddress();
	for(y=0; y<hheight; y++) {
		for(x=hwidth; x<width-1; x++) {
			dest[y*width+x] = src[y*width+x] ^ color;
			dest[y*width+(width-x-1)] = src[y*width+x] ^ color;
			dest[(height-y-1)*width+x] = src[y*width+x] ^ color;
			dest[(height-y-1)*width+(width-x-1)] = src[y*width+x] ^ color;
		}
	}
}

static void mirror_dl()
{
	int x, y;
	unsigned int *src, *dest;

	src = (unsigned int *)video_getaddress();
	dest = (unsigned int *)screen_getaddress();
	for(y=hheight; y<height-1; y++) {
		for(x=0; x<hwidth; x++) {
			dest[y*width+x] = src[y*width+x] ^ color;
			dest[y*width+(width-x-1)] = src[y*width+x] ^ color;
			dest[(height-y-1)*width+x] = src[y*width+x] ^ color;
			dest[(height-y-1)*width+(width-x-1)] = src[y*width+x] ^ color;
		}
	}
}

static void mirror_dr()
{
	int x, y;
	unsigned int *src, *dest;

	src = (unsigned int *)video_getaddress();
	dest = (unsigned int *)screen_getaddress();
	for(y=hheight; y<height-1; y++) {
		for(x=hwidth; x<width-1; x++) {
			dest[y*width+x] = src[y*width+x] ^ color;
			dest[y*width+(width-x-1)] = src[y*width+x] ^ color;
			dest[(height-y-1)*width+x] = src[y*width+x] ^ color;
			dest[(height-y-1)*width+(width-x-1)] = src[y*width+x] ^ color;
		}
	}
}

