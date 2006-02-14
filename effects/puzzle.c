/*
 * EffecTV - Realtime Digital Video Effector
 * Copyright (C) 2001-2006 FUKUCHI Kentaro
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
static int event(SDL_Event *event);

#define BLOCKSIZE 80
#define SLIDING_INTERVAL 30
#define AUTOSOLVE_WAIT 300

static char *effectname = "PuzzleTV";
static int stat;

static int blockSize;
static int blockW;
static int blockH;
static int blockNum;

typedef struct {
	int position;
	int srcOffset;
	int destOffset;
} Block;

static Block *blocks;
static int marginW;
static int marginH;
static int phase;
static int movingBlock;
static int spaceBlock;

static int autoSolveTimer;

static void copyBlockImage(RGB32 *src, RGB32 *dest);
static void blockSetSrcOffset(int i);
static void moveBlock(RGB32 *src, RGB32 *dest);
static void autoSolve(void);

effect *puzzleRegister(void)
{
	effect *entry;
	int x, y;

	blockSize = BLOCKSIZE;
	if(video_width < 320) {
		blockSize = video_width / 4;
	}
	if(video_height < 240) {
		if((video_height / 3) < blockSize) {
			blockSize = video_height / 3;
		}
	}
	blockW = video_width / blockSize;
	blockH = video_height / blockSize;
	blockNum = blockW * blockH;

	blocks = (Block *)malloc(blockNum * sizeof(Block));
	if(blocks == NULL) {
		return NULL;
	}

	for(y=0; y<blockH; y++) {
		for(x=0; x<blockW; x++) {
			blocks[y * blockW + x].destOffset
				= (y * video_width + x) * blockSize;
		}
	}

	marginW = video_width - blockW * blockSize;
	marginH = video_height - blockH * blockSize;

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
	int i, a, b, c;

	for(i=0; i<blockNum; i++) {
		blocks[i].position = i;
	}

	for(i=0; i<20*blockW; i++) {
		/* the number of shuffling times is a rule of thumb. */
		a = fastrand()%(blockNum-1);
		b = fastrand()%(blockNum-1);
		if(a == b)
			b = (b+1)%(blockNum-1);
		c = blocks[a].position;
		blocks[a].position = blocks[b].position;
		blocks[b].position = c;
	}

	for(i=0; i<blockNum; i++) {
		blockSetSrcOffset(i);
	}

	phase = 0;
	movingBlock = -1;
	spaceBlock = blockNum - 1;
	autoSolveTimer = AUTOSOLVE_WAIT;

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
	int  y, i;
	RGB32 *p, *q;

	if(autoSolveTimer == 0) {
		autoSolve();
	} else {
		autoSolveTimer--;
	}

	for(i=0; i<blockNum; i++) {
		if(i == movingBlock || i == spaceBlock) {
			q = dest + blocks[i].destOffset;
			for(y=0; y<blockSize; y++) {
				memset(q, 0, blockSize * PIXEL_SIZE);
				q += video_width;
			}
		} else {
			copyBlockImage( src + blocks[i].srcOffset,
						   dest + blocks[i].destOffset);
		}
	}

	if(movingBlock >= 0) {
		moveBlock(src, dest);
	}

	if(marginW) {
		p =  src + blockW * blockSize;
		q = dest + blockW * blockSize;
		for(y=0; y<blockH * blockSize; y++) {
			memcpy(q, p, marginW * PIXEL_SIZE);
			p += video_width;
			q += video_width;
		}
	}
	if(marginH) {
		p =  src + (blockH * blockSize) * video_width;
		q = dest + (blockH * blockSize) * video_width;
		memcpy(q, p, marginH * video_width * PIXEL_SIZE);
	}

	return 0;
}

static int orderMotion(int dir)
{
	int x, y;
	int dx, dy;

	if(movingBlock >= 0) return -1;

	x = spaceBlock % blockW;
	y = spaceBlock / blockW;
	switch(dir) {
		case 0:
			dx =  0; dy =  1;
			break;
		case 1:
			dx =  0; dy = -1;
			break;
		case 2:
			dx =  1; dy =  0;
			break;
		case 3:
			dx = -1; dy =  0;
			break;
		default:
			return -1;
			break;
	}
	if(x + dx < 0 || x + dx >= blockW) return -1;
	if(y + dy < 0 || y + dy >= blockH) return -1;

	movingBlock = (y + dy) * blockW + x + dx;
	phase = SLIDING_INTERVAL - 1;

	return 0;
}

static int event(SDL_Event *event)
{
	if(event->type == SDL_KEYDOWN) {
		switch(event->key.keysym.sym) {
		case SDLK_w:
		case SDLK_k:
			orderMotion(0);
			break;
		case SDLK_s:
		case SDLK_j:
			orderMotion(1);
			break;
		case SDLK_a:
		case SDLK_h:
			orderMotion(2);
			break;
		case SDLK_d:
		case SDLK_l:
			orderMotion(3);
			break;
		default:
			break;
		}
		autoSolveTimer = AUTOSOLVE_WAIT;
	}
	return 0;
}

static void copyBlockImage(RGB32 *src, RGB32 *dest)
{
	int y;
	
	for(y=blockSize; y>0; y--) {
		memcpy(dest, src, blockSize * PIXEL_SIZE);
		src += video_width;
		dest += video_width;
	}
}

static void blockSetSrcOffset(int i)
{
	int x, y;

	x = blocks[i].position % blockW;
	y = blocks[i].position / blockW;

	blocks[i].srcOffset = (y * video_width + x) * blockSize;
}

static void moveBlock(RGB32 *src, RGB32 *dest)
{
	int sx, sy;
	int dx, dy;
	int x, y;

	sx = movingBlock % blockW;
	sy = movingBlock / blockW;
	dx = spaceBlock % blockW;
	dy = spaceBlock / blockW;

	sx *= blockSize;
	sy *= blockSize;
	dx *= blockSize;
	dy *= blockSize;

	x = dx + (sx - dx) * phase / SLIDING_INTERVAL;
	y = dy + (sy - dy) * phase / SLIDING_INTERVAL;

	copyBlockImage(src + blocks[movingBlock].srcOffset,
			dest + y * video_width + x);

	if(autoSolveTimer == 0) {
		phase--;
	} else {
		phase-=2;
	}
	if(phase < 0) {
		int tmp;
		/* Exchanges positions of the moving block and the space */
		tmp = blocks[movingBlock].position;
		blocks[movingBlock].position = blocks[spaceBlock].position;
		blocks[spaceBlock].position = tmp;

		blockSetSrcOffset(movingBlock);
		blockSetSrcOffset(spaceBlock);

		spaceBlock = movingBlock;
		movingBlock = -1;
	}
}

static void autoSolve(void)
{
	/* IMPORTANT BUG: this functions does *NOT* solve the puzzle! */
	static int lastMove = 0;
	static char dir[4];
	int i, j, x, y, max;

	if(movingBlock >= 0) return;
	for(i=0; i<4; i++) {
		dir[i] = i;
	}
	dir[lastMove] = -1;
	x = spaceBlock % blockW;
	y = spaceBlock / blockW;
	if(x <= 0) dir[3] = -1;
	if(x >= blockW - 1) dir[2] = -1;
	if(y <= 0) dir[1] = -1;
	if(y >= blockH - 1) dir[0] = -1;

	max = 0;
	for(i=0; i<3; i++) {
		if(dir[i] == -1) {
			for(j=i+1; j<4; j++) {
				if(dir[j] != -1) {
					dir[i] = dir[j];
					dir[j] = -1;
					max++;
					break;
				}
			}
		} else {
			max++;
		}
	}

	if(max > 0) {
		i = dir[inline_fastrand() % max];
		if(orderMotion(i) == 0) {
			if(i < 2) {
				lastMove = 1 - i;
			} else {
				lastMove = 5 - i;
			}
		}
	}
}
