/*
 * EffecTV - Realtime Digital Video Effector
 * Copyright (C) 2001 FUKUCHI Kentarou
 *
 * screen.h: header for screen manager
 *
 */

#ifndef __SCREEN_H__
#define __SCREEN_H__

#include <SDL/SDL.h>

extern SDL_Surface *screen;

int screen_init(int flags, int scale);
void screen_quit();
int screen_bpp();
void screen_setcaption(const char *str);
void screen_clear(int color);

#define screen_getaddress() (screen->pixels)
#define screen_update() (SDL_Flip(screen))
#define screen_mustlock() (SDL_MUSTLOCK(screen))
#define screen_lock() (SDL_LockSurface(screen))
#define screen_unlock() (SDL_UnlockSurface(screen))

#endif /* __SCREEN_H__ */
