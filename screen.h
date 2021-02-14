/*
 * EffecTV - Realtime Digital Video Effector
 * Copyright (C) 2001-2006,2021 FUKUCHI Kentaro
 *
 * screen.h: header for screen manager
 *
 */

#ifndef __SCREEN_H__
#define __SCREEN_H__

#include <SDL2/SDL.h>

extern SDL_Window *window;  /* main window */
extern SDL_Surface *screen; /* display surface */
extern int stretch;         /* flag for stretching requirement */
extern int fullscreen;      /* flag for fullscreen mode */
extern int screen_width;
extern int screen_height;
extern int screen_scale;

/* description of these functions are in screen.c */
int screen_init(int w, int h, int s);
void screen_quit(void);
int screen_bpp(void);
void screen_setcaption(const char *str);
void screen_clear(int r, int g, int b, int a);
void screen_fullscreen(void);
int screen_lock(void);
void screen_unlock(void);
unsigned char *screen_getaddress(void);
int screen_update(int);

#endif /* __SCREEN_H__ */
