/*
 * EffecTV - Realtime Digital Video Effector
 * Copyright (C) 2001-2002 FUKUCHI Kentaro
 *
 * DeinterlaceTV - deinterlaces the video.
 * Copyright (C) 2001 Casandro (einStein@donau.de
 *
 */

#include <stdlib.h>
#include <string.h>
#include "../EffecTV.h"
#include "utils.h"
#include <math.h>

int DeinterlaceStart();
int DeinterlaceStop();
int DeinterlaceDraw();

static char *effectname = "DeinterlaceTV";
static int state = 0;

effect *DeinterlaceRegister()
{
	effect *entry;

	entry = (effect *)malloc(sizeof(effect));
	if(entry == NULL) return NULL;
	
	entry->name = effectname;
	entry->start = DeinterlaceStart;
	entry->stop = DeinterlaceStop;
	entry->draw = DeinterlaceDraw;
	entry->event = NULL;

	return entry;
}

int DeinterlaceStart()
{
	if(video_grabstart())
		return -1;
	state = 1;
	return 0;
}

int DeinterlaceStop()
{
	if(state) {
		video_grabstop();
		state = 0;
	}

	return 0;
}

/*int abs(int x)
{
  int y;
  y=x;
  if (y<0) y=0-x;
  return y;
}*/

static int Difference(int a,int b)
{
	return abs(GREEN(a)-GREEN(b));
}

static int MixPixels(int a,int b)
{
	return RGB(((  RED(a)+  RED(b))/2),
		   ((GREEN(a)+GREEN(b))/2),
		   (( BLUE(a)+ BLUE(b))/2));
}

int DeinterlaceDraw()
{
  int x,y;
  int zeile1a,zeile2a,zeile3a,zeile4a;
  int zeile1b,zeile2b,zeile3b,zeile4b;
  int zeile1c,zeile2c,zeile3c,zeile4c;
  int outp1,outp2,outp3,outp4,outp5,outp6;
  int d1,d2;
  RGB32 *src,*dst;
   
	if(video_syncframe())
		return -1;
	if(screen_mustlock()) {
		if(screen_lock() < 0) {
			return video_grabframe();
		}
	}
	src = (RGB32 *)video_getaddress();
	if(stretch) {
		dst = stretching_buffer;
	} else {
		dst = (RGB32 *)screen_getaddress();
	}
	
	for (y=1;y < video_height-2; y+=2)
	  for (x=0;x<video_width; x+=3){
	    zeile1a = *(RGB32 *)(src+(y-1)*video_width+x);
	    zeile2a = *(RGB32 *)(src+(y+0)*video_width+x);
	    zeile3a = *(RGB32 *)(src+(y+1)*video_width+x);
	    zeile4a = *(RGB32 *)(src+(y+2)*video_width+x);
	    zeile1b = *(RGB32 *)(src+(y-1)*video_width+x+1);
	    zeile2b = *(RGB32 *)(src+(y+0)*video_width+x+1);
	    zeile3b = *(RGB32 *)(src+(y+1)*video_width+x+1);
	    zeile4b = *(RGB32 *)(src+(y+2)*video_width+x+1);
	    zeile1c = *(RGB32 *)(src+(y-1)*video_width+x+2);
	    zeile2c = *(RGB32 *)(src+(y+0)*video_width+x+2);
	    zeile3c = *(RGB32 *)(src+(y+1)*video_width+x+2);
	    zeile4c = *(RGB32 *)(src+(y+2)*video_width+x+2);
	    
	    outp3 = zeile2b;
	    outp4 = zeile3b;
	    
	    outp1=zeile2a;
	    outp2=zeile3a;
	    outp5=zeile2c;
	    outp6=zeile3c;
	    d1=Difference(MixPixels(zeile1a,zeile1c),MixPixels(zeile3a,zeile3c))+
	       Difference(MixPixels(zeile2a,zeile2c),MixPixels(zeile4a,zeile4c));
	    d2=Difference(MixPixels(zeile1a,zeile1c),MixPixels(zeile4a,zeile4c))+
	       Difference(MixPixels(zeile2a,zeile2c),MixPixels(zeile3a,zeile3c));
	    if ((d1) < (d2))
	      {outp2=MixPixels(zeile2a,zeile4a);
	       outp4=MixPixels(zeile2b,zeile4b);
	       outp6=MixPixels(zeile2c,zeile4c);}
	    *(RGB32 *)(dst+x+(y-1)*video_width) = outp1;
	    *(RGB32 *)(dst+x+(y+0)*video_width) = outp2;
	    *(RGB32 *)(dst+x+1+(y-1)*video_width) = outp3;
	    *(RGB32 *)(dst+x+1+(y+0)*video_width) = outp4;
	    *(RGB32 *)(dst+x+2+(y-1)*video_width) = outp5;
	    *(RGB32 *)(dst+x+2+(y+0)*video_width) = outp6;
	  }				  
	
	if(stretch) {
		image_stretch_to_screen();
	}
	if(screen_mustlock()) {
		screen_unlock();
	}

	if(video_grabframe())
		return -1;

	return 0;
}
