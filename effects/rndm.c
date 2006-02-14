/*
 * EffecTV - Realtime Digital Video Effector
 * Copyright (C) 2001-2006 FUKUCHI Kentaro
 *
 * rndmTV Random noise based on video signal
 * (c)2002 Ed Tannenbaum <et@et-arts.com>
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

static int rgrabtime=1;
static int rgrab=0;
static int rthecolor=0xffffffff;
static int rmode=1;

static char *effectname = "RndmTV";
static int state = 0;

effect *rndmRegister(void)
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
	int i, tmp, rtmp;

	rgrab++;
	if (rgrab>=rgrabtime){
		rgrab=0;

  	//bzero(dst, video_area*sizeof(RGB32)); // clear the screen
		if (rmode==0){
  	//static

			for (i=0; i<video_height*video_width; i++){
				if((inline_fastrand()>>24)<((*src)&0xff00)>>8){

					*dst=rthecolor;
				}
				else{
					*dst=0x00;
				}
				src++ ; dst++;
			}
		} else {
			for (i=0; i<video_height*video_width; i++){
				/*
				tmp=0;
				if((inline_fastrand()>>24)<((*src)&0xff00)>>8){
					tmp=0xff00;
				}
				if((inline_fastrand()>>24)<((*src)&0xff0000)>>16){
					tmp=tmp |0xff0000;
				}
				if((inline_fastrand()>>24)<((*src)&0xff)){
					tmp=tmp |0xff;
				}
					*dst=tmp;
				*/
				tmp=0;
				rtmp=inline_fastrand()>>24;
				if (rtmp < ((*src)&0xff00)>>8){
					tmp=0xff00;
				}
				if(rtmp < ((*src)&0xff0000)>>16){
					tmp=tmp |0xff0000;
				}
				if(rtmp < ((*src)&0xff)){
					tmp=tmp |0xff;
				}
					*dst=tmp;
				src++ ; dst++;
			}
		}
	}

  	return 0;
}



static int event(SDL_Event *event)
{

	if(event->type == SDL_KEYDOWN) {
		switch(event->key.keysym.sym) {



				case SDLK_0:
					rgrabtime++;
                        if (rgrabtime==0)rgrabtime =1;

                        fprintf(stdout,"rgrabtime=%d\n",rgrabtime);
                        break;


				case SDLK_MINUS:
                        rgrabtime--;
                        if (rgrabtime==0)rgrabtime =1;

                        fprintf(stdout,"rgrabtime=%d\n",rgrabtime);
                        break;
				
				case SDLK_SPACE:
					rmode++;
                        if (rmode==2)rmode =0;

                        fprintf(stdout,"rmode=%d\n",rmode);
                        break;



		default:
			break;
		}
	}

	return 0;
}
