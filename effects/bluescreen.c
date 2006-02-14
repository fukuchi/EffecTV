/*
 * EffecTV - Realtime Digital Video Effector
 * Copyright (C) 2001-2006 FUKUCHI Kentaro
 *
 * BlueScreenTV - blue sceen effect: changes scene background
 * Copyright (C) 2005-2006 Nicolas Argyrou
 *
 * s: take 4-frames background snapshot
 * d: take delayed background snapshot after 3 seconds
 * space: get 4-frames blue screen background and preset tolerance to 30
 * b: get 24-frames blue screen background and preset tolerance to 20
 * c: decrease tolerance by 1 (0-255)
 * v: increase tolerance by 1 (0-255)
 *
 */

/*
 * Developper's notes:
 * The above filter computes color difference between the current frame
 * the pre-defined bluesceen background, and replaces differences less than
 * the threshold level by another background image.
 * Most webcams do not have a very clean image and the threshold is not enough
 * to avoid noise, so the bluescreen is recorded for 4 or 24 frames and the
 * minimum and maximum colors are saved (this method is better than averaging).
 * To avoid noise the replacement background is averaged over 4 frames. It
 * can be taken after a 3 seconds delay to be able to shoot the screen with
 * a webcam.
 * The color difference algorithm is quite different from the other algorithms
 * included in the effectv package. It uses a max(diff(rgv)) formulae with
 * anitaliasing like high quality photo editors do. Moreover it uses it twice
 * for the maximum and minimum blue screen.
 * To have even less noise a fast blur routine blurs the current frame
 * so that noisy lonely pixels are diluted. This blurring routine may be
 * overriden at compilation time by commenting out the "#define USE_BLUR" line.
 * The "#define PROFILING" line in the source code may be uncommented to try
 * other algorithm optimisations, although a lot has been done to allow a
 * maximum speed on 32bits+ processors.
 */

#include <stdlib.h>
#include <string.h>
#include <unistd.h> /* man 3 sleep */
#include "EffecTV.h"
#include "utils.h"

static int start(void);
static int stop(void);
static int draw(RGB32 *src, RGB32 *dest);
static int event(SDL_Event *);

#define MAGIC_THRESHOLD 30
#define MAGIC_THRESHOLD_BEST 20
#define TOLERANCE_STEP  1
#define SNAPSHOT_DELAY  3
#define BLUESCREEN_FRAMES 4
#define BLUESCREEN_FRAMES_BEST 24
#define USE_BLUR
//#define PROFILING

#ifdef PROFILING
#include <sys/time.h>
#endif

static char *effectname = "BlueScreenTV";
static int state = 0;
static RGB32 *bgimage;
static RGB32 *bluescreen_min;
static RGB32 *bluescreen_max;
static unsigned char tolerance=MAGIC_THRESHOLD;
static unsigned char tolerance2=MAGIC_THRESHOLD*2; /* pre-computation */

static int setBackground(void)
{
	int i;
	RGB32 *src, *tmp;

	tmp=(RGB32*)malloc(video_area * PIXEL_SIZE);
	if(tmp==NULL)
		return -1;

/*
 * grabs 4 frames and composites them to get a quality background image
 * (original code by FUKUCHI Kentaro, debugged by Nicolas Argyrou)
 */
/* step 1: grab frame-1 to buffer-1 */
	video_syncframe();
	memcpy(bgimage, video_getaddress(), video_area * PIXEL_SIZE);
	video_grabframe();
/* step 2: add frame-2 to buffer-1 */
	video_syncframe();
	src=(RGB32*)video_getaddress();
	for(i=0;i<video_area;i++)
		bgimage[i]=(src[i]&bgimage[i])+(((src[i]^bgimage[i])&0xfefefe)>>1);
	video_grabframe();
/* step 3: grab frame-3 to buffer-2 */
	video_syncframe();
	memcpy(tmp, video_getaddress(), video_area * PIXEL_SIZE);
	video_grabframe();
/* step 4: add frame-4 to buffer-2 */
	video_syncframe();
	src = (RGB32 *)video_getaddress();
	for(i=0; i<video_area; i++)
		tmp[i] = (src[i]&tmp[i])+(((src[i]^tmp[i])&0xfefefe)>>1);
	video_grabframe();
/* step 5: add buffer-3 to buffer-1 */
	for(i=0; i<video_area; i++) {
		bgimage[i] = ((bgimage[i]&tmp[i])
			+(((bgimage[i]^tmp[i])&0xfefefe)>>1))&0xfefefe;
	}

	/* displays the composite background
	 * (not needed for this effect)
	for(i=0; i<2; i++) {
		if(screen_lock() < 0) {
			break;
		}
		if(stretch) {
			if(i == 0) {
				memcpy(stretching_buffer, bgimage, video_area*PIXEL_SIZE);
			}
			image_stretch_to_screen();
		} else {
			memcpy((RGB32 *)screen_getaddress(), bgimage,
					video_area*PIXEL_SIZE);
		}
		screen_unlock();
		screen_update();
		if(doublebuf == 0)
			break;
	}
	*/

	free(tmp);

	return 0;
}

static int setBlueScreen(int frames)
{
	int i, k;
	RGB32 *src;

	memset(bluescreen_min,-1,video_area*PIXEL_SIZE);
	memset(bluescreen_max,0,video_area*PIXEL_SIZE);

        /* grabs frames, keep min and max to get a bluescreen image */
	for(k=0;k<frames;k++)
	{
		video_syncframe();
        	src = (RGB32 *)video_getaddress();
		for(i=0;i<video_area;i++)
		{
			if (RED(bluescreen_min[i])>RED(src[i]))
				bluescreen_min[i] = (bluescreen_min[i] & 0x00FFFF) | (src[i] & 0xFF0000);
			if (GREEN(bluescreen_min[i])>GREEN(src[i]))
				bluescreen_min[i] = (bluescreen_min[i] & 0xFF00FF) | (src[i] & 0x00FF00);
			if (BLUE(bluescreen_min[i])>BLUE(src[i]))
				bluescreen_min[i] = (bluescreen_min[i] & 0xFFFF00) | (src[i] & 0x0000FF);
			if (RED(bluescreen_max[i])<RED(src[i]))
				bluescreen_max[i] = (bluescreen_max[i] & 0x00FFFF) | (src[i] & 0xFF0000);
			if (GREEN(bluescreen_max[i])<GREEN(src[i]))
				bluescreen_max[i] = (bluescreen_max[i] & 0xFF00FF) | (src[i] & 0x00FF00);
			if (BLUE(bluescreen_max[i])<BLUE(src[i]))
				bluescreen_max[i] = (bluescreen_max[i] & 0xFFFF00) | (src[i] & 0x0000FF);
		}
		video_grabframe();
	}

	return 0;
}

effect *bluescreenRegister(void)
{
	effect *entry;
	
	sharedbuffer_reset();
	bgimage = (RGB32 *)sharedbuffer_alloc(video_area*PIXEL_SIZE);
	if(bgimage == NULL) {
		return NULL;
	}

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
	if(setBackground())
		return -1;

	bluescreen_min = (RGB32 *)malloc(video_area*PIXEL_SIZE);
	if(bluescreen_min == NULL) {
		return -1;
	}

	bluescreen_max = (RGB32 *)malloc(video_area*PIXEL_SIZE);
	if(bluescreen_max == NULL) {
		return -1;
	}

	if(setBlueScreen(BLUESCREEN_FRAMES))
		return -1;

	state = 1;
	return 0;
}

static int stop(void)
{
	free(bluescreen_min); /* from the bitbucket you came ... */
	free(bluescreen_max); /* ... to the bit bucket you shall return */
	state = 0;
	return 0;
}

static int draw(RGB32 *src, RGB32 *dest)
{
#ifdef PROFILING
struct timeval tv0;
gettimeofday(&tv0,NULL);
int errors=0;
#endif

	int k;
	int d;
	RGB32 *bg=bgimage;
	RGB32 *bs_min=bluescreen_min;
	RGB32 *bs_max=bluescreen_max;
#ifdef USE_BLUR
	unsigned char rold=0, gold=0, bold=0;
#endif

	for(k=0; k<video_area; k++)
	{
		/* retreive source */
#ifdef USE_BLUR
		unsigned char r0,g0,b0;
		r0=(*src)>>16;
		g0=(*src)>>8;
		b0=*src;
		/* blur */
		unsigned char r,g,b;
		r=(rold>>1)+(r0>>1);
		g=(gold>>1)+(g0>>1);
		b=(bold>>1)+(b0>>1);
#else
		unsigned char r,g,b;
		r=(*src)>>16;
		g=(*src)>>8;
		b=*src;
#endif

		/* use max and min bluescreen to avoid noise */
		unsigned char rmin,gmin,bmin;
		rmin=(*bs_min)>>16;
		gmin=(*bs_min)>>8;
		bmin=*bs_min;
		unsigned char rmax,gmax,bmax;
		rmax=(*bs_max)>>16;
		gmax=(*bs_max)>>8;
		bmax=*bs_max;

		/* max(dr,dg,db)) method (optimized for 32bits+ processors) */
		d=r<rmin?rmin-r:r>rmax?r-rmax:0;
		unsigned char dgmin=g<gmin?gmin-g:g>gmax?g-gmax:0;
		if (d<dgmin) d=dgmin;
		unsigned char dbmin=b<bmin?bmin-b:b>bmax?b-bmax:0;
		if (d<dbmin) d=dbmin;

#ifdef PROFILING
		if (d>tolerance)
			errors++;
#endif

		/* with antialiasing */
		if (d*2>tolerance*3)
			*dest=*src;
		else if (d*2<tolerance)
			*dest=*bg;
		else
		{
			unsigned char rbg,gbg,bbg;
			rbg=*bg>>16;
			gbg=*bg>>8;
			bbg=*bg;
			/* oh yeah, that's it */
			int m1=(d<<1)-tolerance;
			int m2=m1-tolerance2;
			float k1=(float)m1/(float)tolerance2;
			float k2=(float)m2/(float)tolerance2;
#ifdef USE_BLUR
			*dest=
				 (((int)(r0*k1 - rbg*k2))<<16)
				|(((int)(g0*k1 - gbg*k2))<<8)
				| ((int)(b0*k1 - bbg*k2))
				;
#else
			*dest=
				 (((int)(r*k1 - rbg*k2))<<16)
				|(((int)(g*k1 - gbg*k2))<<8)
				| ((int)(b*k1 - bbg*k2))
				;
#endif
		}

#ifdef USE_BLUR
		rold=r0; gold=g0; bold=b0;
#endif

		src++;
		dest++;
		bg++;
		bs_min++;
		bs_max++;
	}

#ifdef PROFILING
struct timeval tv;
gettimeofday(&tv,NULL);
struct timeval tvdiff;
tvdiff.tv_usec=tv.tv_usec-tv0.tv_usec;
tvdiff.tv_sec=tv.tv_sec-tv0.tv_sec;
if (tvdiff.tv_usec<0)
{
	tvdiff.tv_usec+=1000000;
	tvdiff.tv_sec--;
}
static double tvmed=0;
static double nmed=0;
tvmed+=((double)tvdiff.tv_sec)+((double)tvdiff.tv_usec)/1000000.0;
nmed++;
printf("%g %g %g %g %d\n",tvmed/nmed,nmed/tvmed,tvmed/nmed*25.0,nmed,errors);
if (nmed==1000) exit(1);
#endif
	return 0;
}

static int event(SDL_Event *event)
{
	if(event->type == SDL_KEYDOWN) {
		switch(event->key.keysym.sym) {
		case SDLK_SPACE:
			setBlueScreen(BLUESCREEN_FRAMES);
			tolerance=MAGIC_THRESHOLD;
			tolerance2=MAGIC_THRESHOLD*2;
			break;
		case SDLK_b:
			setBlueScreen(BLUESCREEN_FRAMES_BEST);
			tolerance=MAGIC_THRESHOLD_BEST;
			tolerance2=MAGIC_THRESHOLD_BEST*2;
			break;
		case SDLK_s:
			setBackground();
			break;
		case SDLK_d:
			sleep(SNAPSHOT_DELAY);
			setBackground();
			break;
		case SDLK_c:
			tolerance-=TOLERANCE_STEP;
			tolerance2-=TOLERANCE_STEP*2;
			break;
		case SDLK_v:
			tolerance+=TOLERANCE_STEP;
			tolerance2+=TOLERANCE_STEP*2;
			break;
		default:
			break;
		}
	}
	return 0;
}

