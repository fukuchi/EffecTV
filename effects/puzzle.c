/*
 * EffecTV - Realtime Digital Video Effector
 * Copyright (C) 2001 FUKUCHI Kentarou
 *
 * puzzle.c: separates into blocks and scrambles them. The user can move blocks.
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
static int blockpos[640*480/BLOCKSIZE/BLOCKSIZE];
static unsigned int blockoffset[640*480/BLOCKSIZE/BLOCKSIZE];
static int blockw;
static int blockh;
static int blocknum;
static int spacepos;
static int spacex;
static int spacey;
static int *framebuf;
static int realscale;

effect *puzzleRegister()
{
	effect *entry;
	int x, y;
	
	entry = (effect *)malloc(sizeof(effect));
	if(entry == NULL) {
		return NULL;
	}
	
	entry->name = effectname;
	entry->start = puzzleStart;
	entry->stop = puzzleStop;
	entry->draw = puzzleDraw;
	entry->event = puzzleEvent;

	if(stretch) {
		realscale = 1;
		sharedbuffer_reset();
		framebuf = (unsigned int *)sharedbuffer_alloc(SCREEN_AREA*PIXEL_SIZE);
		if(framebuf == NULL)
			return NULL;
	} else {
		realscale = scale;
	}
	blockw = realscale*SCREEN_WIDTH/BLOCKSIZE;
	blockh = realscale*SCREEN_HEIGHT/BLOCKSIZE;
	blocknum = blockw*blockh;
	for(y=0; y<blockh; y++) {
		for(x=0; x<blockw; x++) {
			blockoffset[y*blockw+x] = y*BLOCKSIZE*SCREEN_WIDTH*realscale
			                        + x*BLOCKSIZE;
		}
	}

	return entry;
}

int puzzleStart()
{
	int i, a, b, c;

	for(i=0; i<blocknum; i++)
		blockpos[i] = i;
	for(i=0; i<100*scale; i++) {
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
	if(hireso){
		if(video_changesize(SCREEN_WIDTH*2, SCREEN_HEIGHT*2))
			return -1;
	}
	if(video_grabstart())
		return -1;
	stat = 1;
	return 0;
}

int puzzleStop()
{
	if(stat) {
		video_grabstop();
		if(hireso){
			video_changesize(0, 0);
		}
		stat = 0;
	}

	return 0;
}

int puzzleDraw()
{
	int  x, y, xx, yy, i;
	unsigned int *src, *dest;
	unsigned int *p, *q;

	if(video_syncframe())
		return -1;
	if(screen_mustlock()) {
		if(screen_lock() < 0) {
			return 0;
		}
	}
	src = (unsigned int *)video_getaddress();
	if(stretch) {
		dest = framebuf;
	} else {
		dest = (unsigned int *)screen_getaddress();
	}

	i = 0;
	for(y=0; y<blockh; y++) {
		for(x=0; x<blockw; x++) {
			p = &src[blockoffset[blockpos[i]]];
			q = &dest[blockoffset[i]];
			if(spacepos == i) {
				for(yy=0; yy<BLOCKSIZE; yy++) {
					for(xx=0; xx<BLOCKSIZE; xx++) {
						q[xx] = 0;
					}
					q += SCREEN_WIDTH*realscale;
				}
			} else {
				for(yy=0; yy<BLOCKSIZE; yy++) {
					for(xx=0; xx<BLOCKSIZE; xx++) {
						q[xx] = p[xx];
					}
					q += SCREEN_WIDTH*realscale;
					p += SCREEN_WIDTH*realscale;
				}
			}
			i++;
		}
	}
	if(stretch) {
		image_stretch(framebuf, (unsigned int *)screen_getaddress());
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
