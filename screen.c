/*
 * EffecTV - Realtime Digital Video Effector
 * Copyright (C) 2001-2006,2021 FUKUCHI Kentaro
 *
 * screen.c: screen manager
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <SDL2/SDL.h>

#include "EffecTV.h"
#include "utils.h"

/* Defined in image.c */
extern RGB32 *stretching_buffer;

/* Main screen for displaying video image */
SDL_Surface *screen;
SDL_Window *window;
SDL_Renderer *renderer;
SDL_Texture *mainTexture;

/*
 * Screen properties. These variables are immutable after calling screen_init()
 */

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


/* Screen initialization.
 * Before calling this function, screen properties(scale, fullscreen) must be
 * set. In the initializing process, those variables may be reset and they are
 * never changed again during run time.
 */
int screen_init(int w, int h, int s)
{
	int flags = 0;

	if(SDL_Init(SDL_INIT_VIDEO) < 0) {
		fprintf(stderr, "Couldn't initialize SDL: %s\n", SDL_GetError());
		return -1;
	}

	if(fullscreen) {
		flags |= SDL_WINDOW_FULLSCREEN;
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

	window = SDL_CreateWindow("EffecTV", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
			screen_width, screen_height, flags);
	if(window == NULL) {
		fprintf(stderr, "Couldn't set display mode: %s\n", SDL_GetError());
		SDL_Quit();
		return -1;
	}

	renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_PRESENTVSYNC);
	if(renderer == NULL) {
		fprintf(stderr, "Couldn't get an appropriate renderer: %s\n", SDL_GetError());
		SDL_Quit();
		return -1;
	}

	screen = SDL_CreateRGBSurface(0, screen_width, screen_height, 32,
			0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
	mainTexture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, screen_width, screen_height);

	SDL_ShowCursor(SDL_DISABLE);
	atexit(screen_quit);
	return 0;
}

/* screen_quit() is called automatically when the process terminates.
 * This function is registerd in screen_init() by callint atexit(). */
void screen_quit(void)
{
	SDL_ShowCursor(SDL_ENABLE);
	SDL_Quit();
}

/* Set the caption. Typically, captions is displayed in the title bar of
 * main screen when EffecTV runs in a windowing system. */
void screen_setcaption(const char *str)
{
	SDL_SetWindowTitle(window, str);
}

/* Fill the screen with the color. */
void screen_clear(int r, int g, int b, int a)
{
	SDL_SetRenderDrawColor(renderer, r, g, b, a);
	SDL_RenderClear(renderer);
	SDL_RenderPresent(renderer);
}

/* Toggles fullscreen mode. */
void screen_fullscreen(void)
{
	int flag;

	fullscreen ^= 1;
	if(fullscreen) {
		flag = SDL_WINDOW_FULLSCREEN_DESKTOP;
	} else {
		flag = 0;
	}
	SDL_SetWindowFullscreen(window, flag);
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

/* Returns an address of the framebuffer */
unsigned char *screen_getaddress(void)
{
	if(stretch) {
		return (unsigned char *)stretching_buffer;
	}

	return screen->pixels;
}

/* Updates the screen */
int screen_update(int flag)
{
	if(stretch && flag == 0) {
		image_stretch_to_screen();
	}

	SDL_UpdateTexture(mainTexture, NULL, screen->pixels, screen->pitch);
	SDL_RenderClear(renderer);
	SDL_RenderCopy(renderer, mainTexture, NULL, NULL);
	SDL_RenderPresent(renderer);

	return 0;
}
