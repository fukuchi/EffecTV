/*
 * EffecTV - Realtime Digital Video Effector
 * Copyright (C) 2001 FUKUCHI Kentarou
 *
 * From main.c of warp-1.1:
 *
 *      Simple DirectMedia Layer demo
 *      Realtime picture 'gooing'
 *      Released under GPL
 *      by sam lantinga slouken@devolution.com
 */

#include <stdlib.h>
#include <string.h>
#include "../EffecTV.h"
#include "utils.h"
#include <math.h>

#ifndef M_PI
#define M_PI	3.14159265358979323846
#endif

void initWarp();
void disposeWarp ();
void doWarp (int xw, int yw, int cw); 

void *offstable;
Sint32 *disttable;
Sint32 ctable[1024];
Sint32 sintable[1024+256];

int warpStart();
int warpStop();
int warpDraw();




static char *effectname = "warpTV";
static int state = 0;

effect *warpRegister()
{
	effect *entry;

	entry = (effect *)malloc(sizeof(effect));
	if(entry == NULL) return NULL;
	
	entry->name = effectname;
	entry->start = warpStart;
	entry->stop = warpStop;
	entry->draw = warpDraw;
	entry->event = NULL;
	return entry;
}

int warpStart()
{
	if(video_grabstart())
		return -1;
	state = 1;
        initWarp();
	return 0;
}

int warpStop()
{
	if(state) {
		video_grabstop();
		state = 0;
	}
	disposeWarp();
	return 0;
}

void initSinTable () {
	Sint32	*tptr, *tsinptr;
	double	i;

	tsinptr = tptr = sintable;

	for (i = 0; i < 1024; i++)
		*tptr++ = (int) (sin (i*M_PI/512) * 32767);

	for (i = 0; i < 256; i++)
		*tptr++ = *tsinptr++;
}

void initOffsTable () {
	Sint32	width, height, len, y;
	Uint32	*source;
	void	**offptr;
	
	offptr = offstable;
	width  = video_width; height = video_height;
	source = (Uint32 *) video_getaddress();
	len    = video_width;
	for (y = 0; y < height; y++) {
		*offptr++ = (void *) source;
		source += len;
	}
}
      
void initDistTable () {
	Sint32	halfw, halfh, *distptr;
	double	x,y,m;

	halfw = video_width>> 1;
	halfh = video_height >> 1;

	distptr = disttable;

	m = sqrt ((double)(halfw*halfw + halfh*halfh));

	for (y = -halfh; y < halfh; y++)
		for (x= -halfw; x < halfw; x++)
			*distptr++ = ((int)
				( (sqrt (x*x+y*y) * 511.9999) / m)) << 1;
}

void initWarp () {

  offstable = malloc (video_height * sizeof (char *));      
  disttable = malloc (video_width * video_height * sizeof (int));
  initSinTable ();
  initOffsTable ();
  initDistTable ();

}

void disposeWarp () {
  free (disttable);
  free (offstable);
  
}

int warpDraw()
{
  static int tval = 0;
  int xw,yw,cw;
  if(video_syncframe())
    return -1;
  if(screen_mustlock()) {
    if(screen_lock() < 0) {
      return video_grabframe();
    }
  }

  xw  = (int) (sin((tval+100)*M_PI/128) * 30);
  yw  = (int) (sin((tval)*M_PI/256) * -35);
  cw  = (int) (sin((tval-70)*M_PI/64) * 50);
  xw += (int) (sin((tval-10)*M_PI/512) * 40);
  yw += (int) (sin((tval+30)*M_PI/512) * 40);	  

  doWarp(xw,yw,cw);
  tval = (tval+1) &511;
  if (stretch) {
    image_stretch_to_screen();
  }
  if(screen_mustlock()) {
    screen_unlock();
  }
  
  if(video_grabframe())
    return -1;
  
  return 0;
}
void doWarp (int xw, int yw, int cw) {
        Sint32 c,i, x,y, dx,dy, maxx, maxy;
        Sint32 width, height, skip, *ctptr, *distptr;
        Uint32 *destptr;
	Uint32 **offsptr;

        ctptr = ctable;
        distptr = disttable;
        width = video_width;
        height = video_height;
	offsptr = (Uint32 **) offstable;
        destptr = (stretch?stretching_buffer:(Uint32 *) screen_getaddress());
	skip = 0 ; /* video_width*sizeof(RGB32)/4 - video_width;; */
        c = 0;
        for (x = 0; x < 512; x++) {
                i = (c >> 3) & 0x3FE;
                *ctptr++ = ((sintable[i] * yw) >> 15);
                *ctptr++ = ((sintable[i+256] * xw) >> 15);
                c += cw;
        }
        maxx = width - 2; maxy = height - 2;
	/*	printf("Forring\n"); */
        for (y = 0; y < height-1; y++) {
         for (x = 0; x < width; x++) {
 	   i = *distptr++; 
 	   dx = ctable [i+1] + x; 
 	   dy = ctable [i] + y;	 


 	   if (dx < 0) dx = 0; 
 	   else if (dx > maxx) dx = maxx; 
   
 	   if (dy < 0) dy = 0; 
 	   else if (dy > maxy) dy = maxy; 
/* 	   printf("f:%d\n",dy); */
	   /*	   printf("%d\t%d\n",dx,dy); */
 	   *destptr++ = * (offsptr[dy] + dx); 
	 }
	 destptr += skip;
	}
	
}
