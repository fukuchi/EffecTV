/*
 * EffecTV - Realtime Digital Video Effector
 * Copyright (C) 2001 FUKUCHI Kentarou
 *
 * rndmTV Random noise based on video signal
 * (c)2002 Ed Tannenbaum <et@et-arts.com>
 *
 */

#include <stdlib.h>
#include <string.h>
#include "../EffecTV.h"
#include "utils.h"

int rndmEvent();
int rndmStart();
int rndmStop();
int rndmDraw();

int rgrabtime=2;
int rgrab=0;
int rthecolor=0xffffffff;
int rmode=1;

static char *effectname = "rndmTV";
static int state = 0;

effect *rndmRegister()
{
	effect *entry;

	entry = (effect *)malloc(sizeof(effect));
	if(entry == NULL) return NULL;

	entry->name = effectname;
	entry->start = rndmStart;
	entry->stop = rndmStop;
	entry->draw = rndmDraw;
	entry->event = rndmEvent;

	return entry;
}

int rndmStart()
{

	if(video_grabstart())
		return -1;
	state = 1;
	return 0;
}

int rndmStop()
{
	if(state) {
		video_grabstop();
		state = 0;
	}

	return 0;
}


int rndmDraw()
{
	int i, tmp, rtmp;
	RGB32 *src,*dst;


	src = (RGB32 *)video_getaddress();
	if (stretch) {
		dst = stretching_buffer;
  		} else {
		dst = (RGB32 *)screen_getaddress();
	}

	if (stretch) image_stretch_to_screen();
	if(video_syncframe())return -1;
  	if(screen_mustlock()) {
    		if(screen_lock() < 0) {
      			return video_grabframe();
    		}
	}
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
	if(screen_mustlock()) screen_unlock();
  	if(video_grabframe()) return -1;
  	return 0;
}



int rndmEvent(SDL_Event *event)
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
