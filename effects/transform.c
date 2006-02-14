/*
 * EffecTV - Realtime Digital Video Effector
 * Copyright (C) 2001-2006 FUKUCHI Kentaro
 *
 * Plugin for EffecTV by Fukuchi Kentarou
 * Written by clifford smith <nullset@dookie.net>
 * 
 * TransForm.c: Performs positinal translations on images
 * 
 * Space selects b/w the different transforms
 *
 * basicaly TransformList contains an array of 
 * values indicating where pixels go.
 * This value could be stored here or generated. The ones i use
 * here are generated.
 * Special value: -1 = black, -2 = get values from mapFromT(x,y,t)
 * ToDo: support multiple functions ( -3 to -10 or something?)
 * Note: the functions CAN return -1 to mean black....
 * 
 */

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "EffecTV.h"
#include "utils.h"

static int start(void);
static int stop(void);
static int draw(RGB32 *src, RGB32 *dest);
static int event(SDL_Event *);
static char *effectname = "TransFormTV";
static int state = 0;

static int **TableList;
#define TableMax  6 /* # of transforms */
static int transform = 0; /* Which transform to use */

static int mapFromT(int x,int y, int t) {
  int xd,yd;
  yd = y + (fastrand() >> 30)-2;
  xd = x + (fastrand() >> 30)-2;
  if (xd > video_width) {
    xd-=1;
  }
  return xd + yd*video_width;
}

effect *TransFormRegister(void)
{
	effect *entry;

  TableList = malloc(TableMax * sizeof(int *)); /* TableMax */
  /* above line is moved from TransFormStart to avoid memory leak.*/

	entry = (effect *)malloc(sizeof(effect));
	if(entry == NULL) return NULL;
	
	entry->name = effectname;
	entry->start = start;
	entry->stop = stop;
	entry->event = event;
	entry->draw = draw;
	return entry;
}

static void SquareTableInit(void)
{
	const int size = 16;
	int x, y, tx, ty;

	for(y=0; y<video_height; y++) {
		ty = y % size - size / 2;
		if((y/size)%2)
			ty = y - ty;
		else
			ty = y + ty;
		if(ty<0) ty = 0;
		if(ty>=video_height) ty = video_height - 1;
		for(x=0; x<video_width; x++) {
			tx = x % size - size / 2;
			if((x/size)%2)
				tx = x - tx;
			else
				tx = x + tx;
			if(tx<0) tx = 0;
			if(tx>=video_width) tx = video_width - 1;
			TableList[5][x+y*video_width] = ty*video_width+tx;
		}
	}
}

static int start(void)
{
  int x,y,i;
//   int xdest,ydest;

  for (i=0;i < TableMax ; i++) {
    TableList[i]= malloc(sizeof(int) * video_width * video_height);
  }

  for (y=0;y < video_height;y++) {
    for (x=0;x < video_width;x++) {

      TableList[0][x+y*video_width]=     
	TableList[1][(video_width-1-x)+y*video_width] = 
	TableList[2][x+(video_height-1-y)*video_width] = 
	TableList[3][(video_width-1-x)+(video_height-1-y)*video_width] = 
	x+y*video_width;
      TableList[4][x+y*video_width] = -2; /* Function */
	}

  }
  SquareTableInit();

  state = 1;
  return 0;
}

static int stop(void)
{
  int i;

  if(state) {
	state = 0;
	for (i=0; i < TableMax ; i++) {
	  free(TableList[i]);
	}

  }
	return 0;
}

static int draw(RGB32 *src, RGB32 *dst)
{
  int x,y;
  static int t=0;
  t++;

	for (y=0;y < video_height;y++)
	  for (x=0;x < video_width;x++) {
	    int dest,value;
	    // printf("%d,%d\n",x,y);
	    dest = TableList[transform][x + y*video_width];
	    if (dest == -2) {
	      dest = mapFromT(x,y,t);
	    }
	    if (dest == -1) {
	      value = 0;
	    } else {
	      value = *(RGB32 *)(src + dest);	 
	    }
	    *(RGB32 *)(dst+x+y*video_width) = value;
	    
	  }

	return 0;
}


static int event(SDL_Event *event) {
  if (event->type == SDL_KEYDOWN) {
    switch(event->key.keysym.sym) {
    case SDLK_SPACE:
      transform++;
      if (transform >= TableMax) {
	transform = 0;
      }
      break;
    default:
      break;
    }
  }
  return 0;
}
