/*
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
#include "../EffecTV.h"
#include <math.h>
#include "utils.h"
int TransFormStart();
int TransFormStop();
int TransFormDraw();
int TransFormEvent(SDL_Event *);
static char *effectname = "TransFormTV";
static int state = 0;

int **TableList;
#define TableMax  6 /* # of transforms */
int transform = 0; /* Which transform to use */

int mapFromT(int x,int y, int t) {
  int xd,yd;
  yd = y + (fastrand() >> 30)-2;
  xd = x + (fastrand() >> 30)-2;
  if (xd > video_width) {
    xd-=1;
  }
  return xd + yd*video_width;
}
effect *TransFormRegister()
{
	effect *entry;

  TableList = malloc(TableMax * sizeof(int *)); /* TableMax */
  /* above line is moved from TransFormStart to avoid memory leak.*/

	entry = (effect *)malloc(sizeof(effect));
	if(entry == NULL) return NULL;
	
	entry->name = effectname;
	entry->start = TransFormStart;
	entry->stop = TransFormStop;
	entry->event = TransFormEvent;
	entry->draw = TransFormDraw;
	return entry;
}

void SquareTableInit()
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

int TransFormStart()
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

  if(video_grabstart())
    return -1;
  state = 1;
  return 0;
}

int TransFormStop()
{
  int i;
	if(state) {
		video_grabstop();
		state = 0;
	}
	for (i=0; i < TableMax ; i++) {
	  free(TableList[i]);
	}

	return 0;
}

int TransFormDraw()
{
  int x,y;
  static int t=0;
  RGB32 *src,*dst;
  t++;

	if(video_syncframe())
		return -1;
	if(screen_mustlock()) {
		if(screen_lock() < 0) {
			return video_grabframe();
		}
	}
	src = (RGB32 *)video_getaddress();
	if (stretch) {
	  dst = stretching_buffer;
	} else {
	 dst = (RGB32 *)screen_getaddress();
	}
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
	if (stretch)
	  image_stretch_to_screen();
	
	if(screen_mustlock()) {
	  screen_unlock();
	}
	
	if(video_grabframe())
	  return -1;
	
	return 0;
}


int TransFormEvent(SDL_Event *event) {
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
