/*
 * EffecTV - Realtime Digital Video Effector
 * Copyright (C) 2001-2003 FUKUCHI Kentaro
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
#include "EffecTV.h"
#include "utils.h"
#include <math.h>

#ifndef M_PI
#define M_PI	3.14159265358979323846
#endif

static void initWarp();
static void disposeWarp ();
static void doWarp (int xw, int yw, int cw,RGB32 *src,RGB32 *dst);

static int *offstable;
static Sint32 *disttable;
static Sint32 ctable[1024];
static Sint32 sintable[1024+256];

static int start(void);
static int stop(void);
static int draw(RGB32 *src, RGB32 *dest);

static char *effectname = "warpTV";
static int state = 0;

effect *warpRegister()
{
	effect *entry;

	entry = (effect *)malloc(sizeof(effect));
	if(entry == NULL) return NULL;
	
	entry->name = effectname;
	entry->start = start;
	entry->stop = stop;
	entry->draw = draw;
	entry->event = NULL;
	return entry;
}

static int start()
{
    initWarp();
	state = 1;
	return 0;
}

static int stop()
{
	if(state) {
		state = 0;
		disposeWarp();
	}
	return 0;
}

static void initSinTable () {
	Sint32	*tptr, *tsinptr;
	double	i;

	tsinptr = tptr = sintable;

	for (i = 0; i < 1024; i++)
		*tptr++ = (int) (sin (i*M_PI/512) * 32767);

	for (i = 0; i < 256; i++)
		*tptr++ = *tsinptr++;
}

static void initOffsTable () {
	int y;
	
	for (y = 0; y < video_height; y++) {
		offstable[y] = y * video_width;
	}
}
      
static void initDistTable () {
	Sint32	halfw, halfh, *distptr;
#ifdef PS2
	float	x,y,m;
#else
	double	x,y,m;
#endif

	halfw = video_width>> 1;
	halfh = video_height >> 1;

	distptr = disttable;

	m = sqrt ((double)(halfw*halfw + halfh*halfh));

	for (y = -halfh; y < halfh; y++)
		for (x= -halfw; x < halfw; x++)
#ifdef PS2
			*distptr++ = ((int)
				( (sqrtf (x*x+y*y) * 511.9999) / m)) << 1;
#else
			*distptr++ = ((int)
				( (sqrt (x*x+y*y) * 511.9999) / m)) << 1;
#endif
}

static void initWarp () {

  offstable = (int *)malloc (video_height * sizeof (int));      
  disttable = malloc (video_width * video_height * sizeof (int));
  initSinTable ();
  initOffsTable ();
  initDistTable ();

}

static void disposeWarp () {
  free (disttable);
  free (offstable);
  
}

static int draw(RGB32 *src, RGB32 *dst)
{
  static int tval = 0;
  int xw,yw,cw;

  xw  = (int) (sin((tval+100)*M_PI/128) * 30);
  yw  = (int) (sin((tval)*M_PI/256) * -35);
  cw  = (int) (sin((tval-70)*M_PI/64) * 50);
  xw += (int) (sin((tval-10)*M_PI/512) * 40);
  yw += (int) (sin((tval+30)*M_PI/512) * 40);	  

  doWarp(xw,yw,cw,src,dst);
  tval = (tval+1) &511;
  
  return 0;
}

static void doWarp (int xw, int yw, int cw,RGB32 *src,RGB32 *dst) {
        Sint32 c,i, x,y, dx,dy, maxx, maxy;
        Sint32 width, height, skip, *ctptr, *distptr;
        Uint32 *destptr;
//	Uint32 **offsptr;

        ctptr = ctable;
        distptr = disttable;
        width = video_width;
        height = video_height;
        destptr = dst;
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
 	   *destptr++ = src[offstable[dy]+dx]; 
	 }
	 destptr += skip;
	}
	
}
