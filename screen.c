/*
 * EffecTV - Realtime Digital Video Effector
 * Copyright (C) 2001 FUKUCHI Kentarou
 *
 * screen.c: screen manager
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <SDL/SDL.h>

#include "EffecTV.h"
#include "screen.h"

SDL_Surface *screen = NULL;
const SDL_VideoInfo *videoinfo;

/*
 * screen_init - initialize screen
 *
 * depth: bits per pixel
 * flags: screen flags
 * scale: scale of screen size
 */
int screen_init(int flags, int scale)
{
	if(SDL_Init(SDL_INIT_VIDEO) < 0) {
		fprintf(stderr, "Couldn't initialize SDL: %s\n", SDL_GetError());
		return -1;
	}

	screen = SDL_SetVideoMode(SCREEN_WIDTH*scale, SCREEN_HEIGHT*scale,
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
	SDL_ShowCursor(0);
	atexit(screen_quit);
	videoinfo = SDL_GetVideoInfo();
	return 0;
}

void screen_quit()
{
	SDL_ShowCursor(1);
	SDL_Quit();
}

int screen_bpp()
{
	if(screen) {
		return screen->format->BitsPerPixel;
	} else {
		return 0;
	}
}

void screen_setcaption(const char *str)
{
	if(videoinfo->wm_available) {
		SDL_WM_SetCaption(str, NULL);
	}
}

void screen_clear(int color)
{
	SDL_FillRect(screen, NULL, color);
	screen_update();
	if(doublebuf) {
		SDL_FillRect(screen, NULL, color);
		screen_update();
	}
}
