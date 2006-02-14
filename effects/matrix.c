/*
 * EffecTV - Realtime Digital Video Effector
 * Copyright (C) 2001-2006 FUKUCHI Kentaro
 *
 * matrixTV - A Matrix Like effect.
 * This plugin for EffectTV is under GNU General Public License
 * See the "COPYING" that should be shiped with this source code
 * Copyright (C) 2001-2003 Monniez Christophe
 * d-fence@swing.be
 *
 * 2003/12/24 Kentaro Fukuchi
 * - Completely rewrote but based on Monniez's idea.
 * - Uses edge detection, not only G value of each pixel.
 * - Added 4x4 font includes number, alphabet and Japanese Katakana characters.
 */

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "EffecTV.h"
#include "utils.h"

#include "matrixFont.xpm"
#define CHARNUM 80
#define FONT_W 4
#define FONT_H 4
#define FONT_DEPTH 4

static int start(void);
static int stop(void);
static int draw(RGB32 *src, RGB32 *dest);
static int event(SDL_Event *);

static char *effectname = "MatrixTV";
static int stat;
static int mode = 0;
static unsigned char font[CHARNUM * FONT_W * FONT_H];
static unsigned char *cmap;
static unsigned char *vmap;
static unsigned char *img;
static int mapW, mapH;
static RGB32 palette[256 * FONT_DEPTH];
static int pause;

typedef struct {
	int mode;
	int y;
	int timer;
	int speed;
} Blip;

#define MODE_NONE 0
#define MODE_FALL 1
#define MODE_STOP 2
#define MODE_SLID 3

static Blip *blips;

static RGB32 green(unsigned int v);
static void setPalette(void);
static void setPattern(void);
static void drawChar(RGB32 *dest, unsigned char c, unsigned char v);
static void createImg(RGB32 *src);
static void updateCharMap(void);

effect *matrixRegister(void)
{
	effect *entry;
	
	sharedbuffer_reset();
	mapW = video_width / FONT_W;
	mapH = video_height / FONT_H;
	cmap = (unsigned char *)sharedbuffer_alloc(mapW * mapH);
	vmap = (unsigned char *)sharedbuffer_alloc(mapW * mapH);
	img = (unsigned char *)sharedbuffer_alloc(mapW * mapH);
	if(cmap == NULL || vmap == NULL || img == NULL) {
		return NULL;
	}

	blips = (Blip *)sharedbuffer_alloc(mapW * sizeof(Blip));
	if(blips == NULL) {
		return NULL;
	}

	setPattern();
	setPalette();

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
	memset(cmap, CHARNUM - 1, mapW * mapH * sizeof(unsigned char));
	memset(vmap, 0, mapW * mapH * sizeof(unsigned char));
	memset(blips, 0, mapW * sizeof(Blip));
	pause = 0;

	stat = 1;
	return 0;
}

static int stop(void)
{
	stat = 0;
	return 0;
}

static int draw(RGB32 *src, RGB32 *dest)
{
	int x, y;
	RGB32 *p, *q;
	unsigned char *c, *v, *i;
	unsigned int val;
	RGB32 a, b;

	if(pause == 0) {
		updateCharMap();
		createImg(src);
	}

	c = cmap;
	v = vmap;
	i = img;

	p = dest;
	for(y=0; y<mapH; y++) {
		q = p;
		for(x=0; x<mapW; x++) {
			val = *i | *v;
//			if(val > 255) val = 255;
			drawChar(q, *c, val);
			i++;
			v++;
			c++;
			q += FONT_W;
		}
		p += video_width * FONT_H;
	}

	if(mode == 1) {
		for(x=0; x<video_area; x++) {
			a = *dest;
			b = *src++;
			b = (b & 0xfefeff) >> 1;
			*dest++ = a | b;
		}
	}

	return 0;
}

/* Create edge-enhanced image data from the input */
static void createImg(RGB32 *src)
{
	int x, y;
	RGB32 *p;
	unsigned char *q;
	unsigned int val;
	RGB32 pc, pr, pb; //center, right, below
	int r, g, b;

	q = img;

	for(y=0; y<mapH; y++) {
		p = src;
		for(x=0; x<mapW; x++) {
			pc = *p;
			pr = *(p + FONT_W - 1);
			pb = *(p + video_width * (FONT_H - 1));

			r = (int)(pc & 0xff0000) >> 15;
			g = (int)(pc & 0x00ff00) >> 7;
			b = (int)(pc & 0x0000ff) * 2;

			val = (r + 2*g + b) >> 5; // val < 64

			r -= (int)(pr & 0xff0000)>>16;
			g -= (int)(pr & 0x00ff00)>>8;
			b -= (int)(pr & 0x0000ff);
			r -= (int)(pb & 0xff0000)>>16;
			g -= (int)(pb & 0x00ff00)>>8;
			b -= (int)(pb & 0x0000ff);

			val += (r * r + g * g + b * b)>>5;

			if(val > 160) val = 160; // want not to make blip from the edge.
			*q = (unsigned char)val;

			p += FONT_W;
			q++;
		}
		src += video_width * FONT_H;
	}
}

#define WHITE 0.45
static RGB32 green(unsigned int v)
{
	unsigned int w;

	if(v < 256) {
		return ((int)(v*WHITE)<<16)|(v<<8)|(int)(v*WHITE);
	}

	w = v - (int)(256*WHITE);
	if(w > 255) w = 255;
	return (w << 16) + 0xff00 + w;
}

static void setPalette(void)
{
	int i;

	for(i=0; i<256; i++) {
		palette[i*FONT_DEPTH  ] = 0;
		palette[i*FONT_DEPTH+1] = green(0x44 * i / 170);
		palette[i*FONT_DEPTH+2] = green(0x99 * i / 170);
		palette[i*FONT_DEPTH+3] = green(0xff * i / 170);
	}
}

static void setPattern(void)
{
	int c, l, x, y, cx, cy;
	char *p;
	unsigned char v;

	/* FIXME: This code is highly depends on the structure of bundled */
	/*        matrixFont.xpm. */
	for(l = 0; l < 32; l++) {
		p = matrixFont[5 + l];
		cy = l /4;
		y = l % 4;
		for(c = 0; c < 40; c++) {
			cx = c / 4;
			x = c % 4;
			switch(*p) {
				case ' ':
					v = 0;
					break;
				case '.':
					v = 1;
					break;
				case 'o':
					v = 2; 
					break;
				case 'O':
				default:
					v = 3;
					break;
			}
			font[(cy * 10 + cx) * FONT_W * FONT_H + y * FONT_W + x] = v;
			p++;
		}
	}
}

static void drawChar(RGB32 *dest, unsigned char c, unsigned char v)
{
	int x, y, i;
	int *p;
	unsigned char *f;

	i = 0;
	if(v == 255) { // sticky characters
		v = 160;
	}

	p = &palette[(int)v * FONT_DEPTH];
	f = &font[(int)c * FONT_W * FONT_H];
	for(y=0; y<FONT_H; y++) {
		for(x=0; x<FONT_W; x++) {
			*dest++ = p[*f];
			f++;
		}
		dest += video_width - FONT_W;
	}
}

static void darkenColumn(int);
static void blipNone(int);
static void blipFall(int);
static void blipStop(int);
static void blipSlide(int);

static void updateCharMap(void)
{
	int x;

	for(x=0; x<mapW; x++) {
		darkenColumn(x);
		switch(blips[x].mode) {
			default:
			case MODE_NONE:
				blipNone(x);
				break;
			case MODE_FALL:
				blipFall(x);
				break;
			case MODE_STOP:
				blipStop(x);
				break;
			case MODE_SLID:
				blipSlide(x);
				break;
		}
	}
}

static void darkenColumn(int x)
{
	int y;
	unsigned char *p;
	int v;

	p = vmap + x;
	for(y=0; y<mapH; y++) {
		v = *p;
		if(v < 255) {
			v *= 0.9;
			*p = v;
		}
		p += mapW;
	}
}

static void blipNone(int x)
{
	unsigned int r;

	// This is a test code to reuse a randome number for multi purpose. :-P
	// Of course it isn't good code because fastrand() doesn't generate ideal
	// randome numbers.
	r = inline_fastrand();

	if((r & 0xf0) == 0xf0) {
		blips[x].mode = MODE_FALL;
		blips[x].y = 0;
		blips[x].speed = (r >> 30) + 1;
		blips[x].timer = 0;
	} else if((r & 0x0f000) ==  0x0f000) {
		blips[x].mode = MODE_SLID;
		blips[x].timer = (r >> 28) + 15;
		blips[x].speed = ((r >> 24) & 3) + 2;
	}
}

static void blipFall(int x)
{
	int i, y;
	unsigned char *p, *c;
	unsigned int r;

	y = blips[x].y;
	p = vmap + x + y * mapW;
	c = cmap + x + y * mapW;

	for(i=blips[x].speed; i>0; i--) {
		if(blips[x].timer > 0) {
			*p = 255;
		} else {
			*p = 254 - i * 10;
		}
		*c = inline_fastrand() % CHARNUM;
		p += mapW;
		c += mapW;
		y++;
		if(y >= mapH) break;
	}
	if(blips[x].timer > 0) {
		blips[x].timer--;
	}

	if(y >= mapH) {
		blips[x].mode = MODE_NONE;
	}

	blips[x].y = y;

	if(blips[x].timer == 0) {
		r = inline_fastrand();
		if((r & 0x3f00) == 0x3f00) {
			blips[x].timer = (r >> 28) + 8;
		} else if(blips[x].speed > 1 && (r & 0x7f) == 0x7f) {
			blips[x].mode = MODE_STOP;
			blips[x].timer = (r >> 26) + 30;
		}
	}
}

static void blipStop(int x)
{
	int y;

	y = blips[x].y;
	vmap[x + y * mapW] = 254;
	cmap[x + y * mapW] = inline_fastrand() % CHARNUM;

	blips[x].timer--;

	if(blips[x].timer < 0) {
		blips[x].mode = MODE_FALL;
	}
}

static void blipSlide(int x)
{
	int y, dy;
	unsigned char *p;

	blips[x].timer--;
	if(blips[x].timer < 0) {
		blips[x].mode = MODE_NONE;
	}

	p = cmap + x + mapW * (mapH - 1);
	dy = mapW * blips[x].speed;

	for(y=mapH - blips[x].speed; y>0; y--) {
		*p = *(p - dy);
		p -= mapW;
	}
	for(y=blips[x].speed; y>0; y--) {
		*p = inline_fastrand() % CHARNUM;
		p -= mapW;
	}
}

static int event(SDL_Event *event)
{
	if(event->type == SDL_KEYDOWN) {
		switch(event->key.keysym.sym) {
		case SDLK_SPACE:
			memset(cmap, CHARNUM - 1, mapW * mapH * sizeof(unsigned char));
			memset(vmap, 0, mapW * mapH * sizeof(unsigned char));
			memset(blips, 0, mapW * sizeof(Blip));
			pause = 1;
			break;
		case SDLK_1:
		case SDLK_KP1:
			mode = 0;
			break;
		case SDLK_2:
		case SDLK_KP2:
			mode = 1;
			break;
		default:
			break;
		}
	} else if(event->type == SDL_KEYUP) {
		if(event->key.keysym.sym == SDLK_SPACE) {
			pause = 0;
		}
	}

	return 0;
}
