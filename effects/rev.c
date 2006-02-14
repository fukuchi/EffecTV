/*
 * EffecTV - Realtime Digital Video Effector
 * Copyright (C) 2001-2006 FUKUCHI Kentaro
 *
 * revTV based on Rutt-Etra Video Synthesizer 1974?
 *
 * (c)2002 Ed Tannenbaum
 *
 * This effect acts like a waveform monitor on each line.
 * It was originally done by deflecting the electron beam on a monitor using
 * additional electromagnets on the yoke of a b/w CRT. Here it is emulated digitally.

 * Experimental tapes were made with this system by Bill and Louise Etra and Woody and Steina Vasulka

 * The line spacing can be controlled using the 1 and 2 Keys.
 * The gain is controlled using the 3 and 4 keys.
 * The update rate is controlled using the 0 and - keys.
 */

#include <stdlib.h>
#include <string.h>
#include "EffecTV.h"
#include "utils.h"

static int start(void);
static int stop(void);
static int draw(RGB32 *src, RGB32 *dest);
static int event(SDL_Event *event);

static int vgrabtime=1;
static int vgrab=0;
static int linespace=6;
static int vscale=50;

static int vthecolor=0xffffffff;

static char *effectname = "RevTV";
static int state = 0;

static void vasulka(RGB32 *src, RGB32 *dst, int srcx, int srcy, int dstx, int dsty, int w, int h);

effect *revRegister(void)
{
	effect *entry;

	entry = (effect *)malloc(sizeof(effect));
	if(entry == NULL) return NULL;

	entry->name = effectname;
	entry->start = start;
	entry->stop = stop;
	entry->draw = draw;
	entry->event = event;

	return entry;
}

static int start(void)
{
	state = 1;
	return 0;
}

static int stop(void)
{
	state = 0;
	return 0;
}


static int draw(RGB32 *src, RGB32 *dst)
{
	vgrab++;
	if (vgrab>=vgrabtime){
		vgrab=0;
		memset(dst, 0, video_area*PIXEL_SIZE); // clear the screen

		vasulka(src, dst, 0, 0, 0, 0, video_width, video_height);
	}

  	return 0;
}

static void vasulka(RGB32 *src, RGB32 *dst, int srcx, int srcy, int dstx, int dsty, int w, int h)
{
	RGB32 *cdst=dst+((dsty*video_width)+dstx);
	RGB32 *nsrc;
	int y,x,R,G,B,yval;
	int offset;

// draw the offset lines
	for (y=srcy; y<h+srcy; y+=linespace){
		for(x=srcx; x<=w+srcx; x++) {
			nsrc=src+(y*video_width)+x;
			// Calc Y Value for curpix
			R = ((*nsrc)&0xff0000)>>(16-1);
			G = ((*nsrc)&0xff00)>>(8-2);
			B = (*nsrc)&0xff;
			yval = y-((short)(R + G + B) / vscale) ;
			offset = x + yval * video_width;
			if(offset >= 0 && offset < video_area) {
				cdst[offset]=vthecolor;
			}
		}
	 }
}


static int event(SDL_Event *event)
{

	if(event->type == SDL_KEYDOWN) {
		switch(event->key.keysym.sym) {

			case SDLK_0:
				vgrabtime++;
				if (vgrabtime==0)vgrabtime =1;
				fprintf(stdout,"vgrabtime=%d\n",vgrabtime);
				break;

			case SDLK_MINUS:
				vgrabtime--;
				if (vgrabtime==0)vgrabtime =1;
				fprintf(stdout,"vgrabtime=%d\n",vgrabtime);
				break;

			case SDLK_2:
				linespace++;
				if (linespace==0) linespace=1;
				fprintf(stdout,"linespace=%d\n",linespace);
				break;

			case SDLK_1:
				linespace--;
				if (linespace==0) linespace=1;
				fprintf(stdout,"linespace=%d\n",linespace);
				break;

			case SDLK_4:
				vscale-=2;
				if (vscale<=0)vscale =1;
				fprintf(stdout,"vscale=%d\n",vscale);
				break;

			case SDLK_3:
				vscale+=2;
				fprintf(stdout,"vscale=%d\n",vscale);
				break;

		default:
			break;
		}
	}

	return 0;
}
