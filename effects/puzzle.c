/*
 * EffecTV - Realtime Digital Video Effector
 * Copyright (C) 2001-2003 FUKUCHI Kentaro
 *
 * PuzzleTV - separates the image into blocks and scrambles them.
 *            The blocks can be moved interactively.
 *
 * The origin of PuzzleTV is ``Video Puzzle'' by Suutarou in 1993.
 * It runs on Fujitsu FM-TOWNS.
 */

#include <stdlib.h>
#include <string.h>
#include "EffecTV.h"
#include "utils.h"

static int start(void);
static int stop(void);
static int draw(RGB32 *src, RGB32 *dest);
static int event();

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
	entry->start = start;
	entry->stop = stop;
	entry->draw = draw;
	entry->event = event;

	return entry;
}

static int start()
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

	stat = 1;
	return 0;
}

static int stop()
{
	stat = 0;
	return 0;
}

static int draw(RGB32 *src, RGB32 *dest)
{
	int  x, y, xx, yy, i;
	RGB32 *p, *q;

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
		memcpy(q, p, marginh*video_width*sizeof(RGB32));
	}

	return 0;
}

static int event(SDL_Event *event)
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
