/*
 * EffecTV - Realtime Digital Video Effector
 * Copyright (C) 2001-2002 FUKUCHI Kentaro
 *
 * screen.h: header for screen manager
 *
 */

#ifndef __SCREEN_H__
#define __SCREEN_H__

#include <SDL/SDL.h>

extern SDL_Surface *screen; /* display surface */
extern int stretch; 	/* flag for stretching requirement */
extern int doublebuf;	/* flag for double buffering */
extern int fullscreen;	/* flag for fullscreen mode */
extern int hwsurface;	/* flag for hardware surface */
extern int screen_width;
extern int screen_height;
extern int screen_scale;

/* description of these functions are in screen.c */
int screen_init(int w, int h, int s);
void screen_quit();
int screen_bpp();
void screen_setcaption(const char *str);
void screen_clear(int color);
void screen_fullscreen();

/* Returns memory address of the frame buffer. The value may be changed every drawing
 * phase. You have to call this function at the begining of draw function of effects. */
#ifndef RGB_BGR_CONVERSION
#define screen_getaddress() (screen->pixels)
#else
unsigned char *screen_getaddress();
#endif

/* This function must be called when the effects fished their drawing phase.
 * Until calling this function, the address of the frame buffer is not changed. */
#ifndef RGB_BGR_CONVERSION
#define screen_update() (SDL_Flip(screen))
#else
int screen_update();
#endif

/* If this function returns non-zero, you have to call screen_lock before access
 * the frame buffer. */
#define screen_mustlock() (SDL_MUSTLOCK(screen))

/* Lock the frame buffer */
#define screen_lock() (SDL_LockSurface(screen))

/* Unlock the frame buffer */
#define screen_unlock() (SDL_UnlockSurface(screen))

#endif /* __SCREEN_H__ */
