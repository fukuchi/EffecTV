/*
 * EffecTV - Realtime Digital Video Effector
 * Copyright (C) 2001-2006 FUKUCHI Kentaro
 *
 * screen.c: screen manager
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <SDL/SDL.h>

#include "EffecTV.h"

/* Main screen for displaying video image */
SDL_Surface *screen = NULL;

/* Screeninfo contains some properties of the screen */
static const SDL_VideoInfo *screeninfo;

/*
 * Screen properties. These variables are immutable after calling screen_init()
 */
/* Flag for double buffering mode. */
int doublebuf = 0;

/* Flag for hardware surface mode. */
int hwsurface = 0;

/* Flag for fullscreen mode. */
int fullscreen = 0;

/* This flag is set when capturing size and screen size are different. */
int stretch;

/* Screen width and height. */
int screen_width;
int screen_height;
/* When screen_width(height) is indivisible by video_width(height),
 * or scale of width and height are different, screen_scale is set to -1. */
int screen_scale;

#ifdef RGB_BGR_CONVERSION
/* Temporal buffer for pixel format conversion */
unsigned char *bgr_buf;
#endif


/* Screen initialization.
 * Before calling this function, screen properties(scale, doublebuf, hwsurface,
 * fullscreen) must be set. In the initializing process, those variables may be
 * reset and they are never changed again during run time.
 */
int screen_init(int w, int h, int s)
{
	int flags = 0;

	if(SDL_Init(SDL_INIT_VIDEO) < 0) {
		fprintf(stderr, "Couldn't initialize SDL: %s\n", SDL_GetError());
		return -1;
	}

	if(hwsurface) {
		flags |= SDL_HWSURFACE;
	}
	if(fullscreen) {
		flags |= SDL_FULLSCREEN;
	}
	if(doublebuf) {
		flags |= SDL_DOUBLEBUF;
	}

	if(w || h) {
		if(w < video_width)
			w = video_width;
		if(h < video_height)
			h = video_height;
		screen_width = w;
		screen_height = h;
		if((w % video_width) || (h % video_height)) {
			screen_scale = -1;
		} else if((w / video_width) == (h / video_height)) {
			screen_scale = w / video_width;
		} else {
			screen_scale = -1;
		}
	} else {
		screen_scale = s;
		screen_width = video_width * s;
		screen_height = video_height * s;
	}

	if((screen_width == video_width) && (screen_height == video_height)) {
		stretch = 0;
	} else {
		stretch = 1;
	}

	screen = SDL_SetVideoMode(screen_width, screen_height,
	                          DEFAULT_DEPTH, flags);
	if(screen == NULL) {
		fprintf(stderr, "Couldn't set display mode: %s\n", SDL_GetError());
		SDL_Quit();
		return -1;
	}
	if(screen->flags != flags) {
		if(hwsurface && !(screen->flags & SDL_HWSURFACE)) {
			hwsurface = 0;
			fprintf(stderr, "Hardware surface is not supported.\n");
		}
		if(fullscreen && !(screen->flags & SDL_FULLSCREEN)) {
			fullscreen = 0;
			fprintf(stderr, "Fullscreen mode is not supported.\n");
		}
		if(doublebuf && !(screen->flags & SDL_DOUBLEBUF)) {
			doublebuf = 0;
			fprintf(stderr, "Double buffer mode is not supported.\n");
		}
	}

#ifdef RGB_BGR_CONVERSION
	bgr_buf = (unsigned char *)malloc(screen_width * screen_height * 4);
	if(bgr_buf == NULL) {
		fprintf(stderr, "Memory allocation failed.\n");
		SDL_Quit();
		return -1;
	}
#endif

	SDL_ShowCursor(SDL_DISABLE);
	atexit(screen_quit);
	screeninfo = SDL_GetVideoInfo();
	return 0;
}

/* screen_quit() is called automatically when the process terminates.
 * This function is registerd in screen_init() by callint atexit(). */
void screen_quit(void)
{
	SDL_ShowCursor(SDL_ENABLE);
	SDL_Quit();
}

/* Returns bits-per-pixel value. */
int screen_bpp(void)
{
	if(screen) {
		return screen->format->BitsPerPixel;
	} else {
		return 0;
	}
}

/* Set the caption. Typically, captions is displayed in the title bar of
 * main screen when EffecTV runs in a windowing system. */
void screen_setcaption(const char *str)
{
	if(screeninfo->wm_available) {
		SDL_WM_SetCaption(str, NULL);
	}
}

/* Fill the screen with the color. When double buffering mode is enable,
 * both buffers are cleared. */
void screen_clear(int color)
{
#ifdef RGB_BGR_CONVERSION
	bzero(bgr_buf, screen_width * screen_height * sizeof(RGB32));
#endif
	SDL_FillRect(screen, NULL, color);
	screen_update();
	if(doublebuf) {
		SDL_FillRect(screen, NULL, color);
		screen_update();
	}
}

/* Toggles fullscreen mode. */
void screen_fullscreen(void)
{
	if(screeninfo->wm_available) {
		if(SDL_WM_ToggleFullScreen(screen))
			fullscreen ^= 1;
	}
}

/* Lock the screen, if needed. */
int screen_lock(void)
{
	if(SDL_MUSTLOCK(screen))
		return SDL_LockSurface(screen);
	return 0;
}

/* Unlock the screen, if needed. */
void screen_unlock(void)
{
	if(SDL_MUSTLOCK(screen))
		SDL_UnlockSurface(screen);
}

#ifdef RGB_BGR_CONVERSION
/* Returns an address of the framebuffer */
unsigned char *screen_getaddress(void)
{
	return bgr_buf;
}

/* Updates the screen */
int screen_update(void)
{
	int i;
	int j;
	unsigned char *src, *dest;
	
	if(screen_lock() < 0) {
		return 0;
	}
	src = bgr_buf;
	dest = (unsigned char *)screen->pixels;
	j = screen_width * screen_height;
	for(i=0; i<j; i++) {
		dest[i*4] = src[i*4+2];
		dest[i*4+1] = src[i*4+1];
		dest[i*4+2] = src[i*4];
	}
	screen_unlock();
	SDL_Flip(screen);

	return 0;
}

#endif /* RGB_BGR_CONVERSION */
