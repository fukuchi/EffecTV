/*
 * EffecTV - Realtime Digital Video Effector
 * Copyright (C) 2001 FUKUCHI Kentarou
 *
 * PuzzleTV - separates the image into blocks and scrambles them.
 *            The blocks can be moved interactively.
 *
 * The origin of PuzzleTV is ``Video Puzzle'' by Suutarou in 1993.
 * It runs on Fujitsu FM-TOWNS.
 */

#include <stdlib.h>
#include <string.h>
#include "../EffecTV.h"
#include "utils.h"

int puzzleStart();
int puzzleStop();
int puzzleDraw();
int puzzleEvent();

#define BLOCKSIZE 80

static char *effectname = "PuzzleTV";
static int stat;
static int *blockpos;
static int *blockoffset;
static int blocksize;
static int blockw;
static int blockh;
static int blocknum;
static int marginw;
static int marginh;
static int spacepos;
static int spacex;
static int spacey;

effect *puzzleRegister()
{
	effect *entry;
	int x, y;

	blocksize = BLOCKSIZE;
	if(video_width < 320) {
		blocksize = video_width / 4;
	}
	if(video_height < 240) {
		if((video_height/3) < blocksize) {
			blocksize = video_height / 3;
		}
	}
	blockw = video_width / blocksize;
	blockh = video_height / blocksize;
	blocknum = blockw * blockh;

	blockpos = (int *)malloc(blocknum*sizeof(int));
	blockoffset = (int *)malloc(blocknum*sizeof(int));
	if(blockpos == NULL || blockoffset == NULL) {
		return NULL;
	}

	for(y=0; y<blockh; y++) {
		for(x=0; x<blockw; x++) {
			blockoffset[y*blockw+x] = y*blocksize*video_width + x*blocksize;
		}
	}

	marginw = video_width - blockw * blocksize;
	marginh = video_height - blockh * blocksize;

	entry = (effect *)malloc(sizeof(effect));
	if(entry == NULL) {
		return NULL;
	}
	
	entry->name = effectname;
	entry->start = puzzleStart;
	entry->stop = puzzleStop;
	entry->draw = puzzleDraw;
	entry->event = puzzleEvent;

	return entry;
}

int puzzleStart()
{
	int i, a, b, c;

	for(i=0; i<blocknum; i++)
		blockpos[i] = i;
	for(i=0; i<20*blockw; i++) {
		/* the number of shuffling times is a rule of thumb. */
		a = fastrand()%(blocknum-1);
		b = fastrand()%(blocknum-1);
		if(a == b)
			b = (b+1)%(blocknum-1);
		c = blockpos[a];
		blockpos[a] = blockpos[b];
		blockpos[b] = c;
	}
	spacepos = blocknum-1;
	spacex = blockw-1;
	spacey = blockh-1;
	if(video_grabstart())
		return -1;
	stat = 1;
	return 0;
}

int puzzleStop()
{
	if(stat) {
		video_grabstop();
		stat = 0;
	}

	return 0;
}

int puzzleDraw()
{
	int  x, y, xx, yy, i;
	RGB32 *src, *dest;
	RGB32 *p, *q;

	if(video_syncframe())
		return -1;
	if(screen_mustlock()) {
		if(screen_lock() < 0) {
			return video_grabframe();
		}
	}
	src = (RGB32 *)video_getaddress();
	if(stretch) {
		dest = stretching_buffer;
	} else {
		dest = (RGB32 *)screen_getaddress();
	}

	i = 0;
	for(y=0; y<blockh; y++) {
		for(x=0; x<blockw; x++) {
			p = &src[blockoffset[blockpos[i]]];
			q = &dest[blockoffset[i]];
			if(spacepos == i) {
				for(yy=0; yy<blocksize; yy++) {
					for(xx=0; xx<blocksize; xx++) {
						q[xx] = 0;
					}
					q += video_width;
				}
			} else {
				for(yy=0; yy<blocksize; yy++) {
					for(xx=0; xx<blocksize; xx++) {
						q[xx] = p[xx];
					}
					q += video_width;
					p += video_width;
				}
			}
			i++;
		}
	}
	p = src + blockw * blocksize;
	q = dest + blockw * blocksize;
	if(marginw) {
		for(y=0; y<blockh*blocksize; y++) {
			for(x=0; x<marginw; x++) {
				*q++ = *p++;
			}
			p += video_width - marginw;
			q += video_width - marginw;
		}
	}
	if(marginh) {
		p = src + (blockh * blocksize) * video_width;
		q = dest + (blockh * blocksize) * video_width;
		bcopy(p, q, marginh*video_width*sizeof(RGB32));
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

int puzzleEvent(SDL_Event *event)
{
	int tmp, nextpos;

	nextpos = -1;
	if(event->type == SDL_KEYDOWN) {
		switch(event->key.keysym.sym) {
		case SDLK_w:
		case SDLK_k:
			if(spacey<blockh-1) {
				nextpos = spacepos + blockw;
				spacey++;
			}
			break;
		case SDLK_s:
		case SDLK_j:
			if(spacey>0) {
				nextpos = spacepos - blockw;
				spacey--;
			}
			break;
		case SDLK_a:
		case SDLK_h:
			if(spacex<blockw-1) {
				nextpos = spacepos + 1;
				spacex++;
			}
			break;
		case SDLK_d:
		case SDLK_l:
			if(spacex>0) {
				nextpos = spacepos - 1;
				spacex--;
			}
			break;
		default:
			break;
		}
	}
	if(nextpos>=0) {
		tmp = blockpos[spacepos];
		blockpos[spacepos] = blockpos[nextpos];
		blockpos[nextpos] = tmp;
		spacepos = nextpos;
	}
	return 0;
}
