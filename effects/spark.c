/*
 * EffecTV - Realtime Digital Video Effector
 * Copyright (C) 2001 FUKUCHI Kentarou
 *
 * SparkTV - spark effect.
 * Copyright (C) 2001 FUKUCHI Kentarou
 *
 */

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "../EffecTV.h"
#include "utils.h"

int sparkStart();
int sparkStop();
int sparkDraw();
int sparkEvent();

static char *effectname = "SparkTV";
static int stat;

struct shortvec {
	int x1;
	int y1;
	int x2;
	int y2;
};

#define SPARK_MAX 10
#define POINT_MAX 100
static struct shortvec sparks[SPARK_MAX];
static int sparks_life[SPARK_MAX];
static int sparks_head;
static int px[POINT_MAX];
static int py[POINT_MAX];
static int pp[POINT_MAX];

static int shortvec_length2(struct shortvec sv)
{
	int dx, dy;

	dx = sv.x2 - sv.x1;
	dy = sv.y2 - sv.y1;

	return (dx*dx+dy*dy);
}

static void draw_sparkline_dx
(int x, int y, int dx, int dy, RGB32 *dest, int width, int height)
{
	int t, i, ady;
	RGB32 *p;

	p = &dest[y*width+x];
	t = dx;
	ady = abs(dy);
	for(i=0; i<=dx; i++) {
		if(y>2 && y<height-2) {
			*(p-width*2) = 0xff;
			*(p-width) = 0x8fff;
			*p = 0xffffff;
			*(p+width) = 0x8fff;
			*(p+width*2) = 0xff;
		}
		p++;
		t -= ady;
		if(t<0) {
			t += dx;
			if(dy<0) {
				y--;
				p -= width;
			} else {
				y++;
				p += width;
			}
		}
	}
}

static void draw_sparkline_dy
(int x, int y, int dx, int dy, RGB32 *dest, int width, int height)
{
	int t, i, adx;
	RGB32 *p;

	p = &dest[y*width+x];
	t = dy;
	adx = abs(dx);
	for(i=0; i<=dy; i++) {
		if(x>2 && x<width-2) {
			*(p-2) = 0xff;
			*(p-1) = 0x8fff;
			*p = 0xffffff;
			*(p+1) = 0x8fff;
			*(p+2) = 0xff;
		}
		p += width;
		t -= adx;
		if(t<0) {
			t += dy;
			if(dx<0) {
				x--;
				p--;
			} else {
				x++;
				p++;
			}
		}
	}
}

static void draw_sparkline
(int x1, int y1, int x2, int y2, RGB32 *dest, int width, int height)
{
	int dx, dy;

	dx = x2 - x1;
	dy = y2 - y1;

	if(abs(dx)>abs(dy)) {
		if(dx<0) {
			draw_sparkline_dx(x2, y2, -dx, -dy, dest, width, height);
		} else {
			draw_sparkline_dx(x1, y1, dx, dy, dest, width, height);
		}
	} else {
		if(dy<0) {
			draw_sparkline_dy(x2, y2, -dx, -dy, dest, width, height);
		} else {
			draw_sparkline_dy(x1, y1, dx, dy, dest, width, height);
		}
	}
}

static void break_line(int a, int b, int width, int height)
{
	int dx, dy;
	int x, y;
	int c;
	int len;

	dx = px[b] - px[a];
	dy = py[b] - py[a];
	if((dx*dx+dy*dy)<100 || (b-a)<3) {
		pp[a] = b;
		return;
	}
	len = (abs(dx)+abs(dy))/4;
	x = px[a] + dx/2 - len/2 + len*(int)((inline_fastrand())&255)/256;
	y = py[a] + dy/2 - len/2 + len*(int)((inline_fastrand())&255)/256;
	if(x<0) x = 0;
	if(y<0) y = 0;
	if(x>=width) x = width - 1;
	if(y>=height) y = height - 1;
	c = (a+b)/2;
	px[c] = x;
	py[c] = y;
	break_line(a, c, width, height);
	break_line(c, b, width, height);
}

static void draw_spark(struct shortvec sv, RGB32 *dest, int width, int height)
{
	int i;

	px[0] = sv.x1;
	py[0] = sv.y1;
	px[POINT_MAX-1] = sv.x2;
	py[POINT_MAX-1] = sv.y2;
	break_line(0, POINT_MAX-1, width, height);
	for(i=0; pp[i]>0; i=pp[i]) {
		draw_sparkline(px[i], py[i], px[pp[i]], py[pp[i]], dest, width, height);
	}

}

static struct shortvec scanline_dx(int dir, int y1, int y2, unsigned char *diff)
{
	int i, x, y;
	int dy;
	int width = video_width;
	struct shortvec sv;
	int start = 0;

	dy = 256 * (y2 - y1) / width;
	y = y1 * 256;
	if(dir == 1) {
		x = 0;
	} else {
		x = width - 1;
	}
	for(i=0; i<width; i++) {
		if(start == 0) {
			if(diff[(y>>8)*width+x]) {
				sv.x1 = x;
				sv.y1 = y>>8;
				start = 1;
			}
		} else {
			if(diff[(y>>8)*width+x] == 0) {
				sv.x2 = x;
				sv.y2 = y>>8;
				start = 2;
				break;
			}
		}
		y += dy;
		x += dir;
	}
	if(start == 0) {
		sv.x1 = sv.x2 = sv.y1 = sv.y2 = 0;
	}
	if(start == 1) {
		sv.x2 = x - dir;
		sv.y2 = (y - dy)>>8;
	}
	return sv;
}

static struct shortvec scanline_dy(int dir, int x1, int x2, unsigned char *diff)
{
	int i, x, y;
	int dx;
	int width = video_width;
	int height = video_height;
	struct shortvec sv;
	int start = 0;

	dx = 256 * (x2 - x1) / height;
	x = x1 * 256;
	if(dir == 1) {
		y = 0;
	} else {
		y = height - 1;
	}
	for(i=0; i<height; i++) {
		if(start == 0) {
			if(diff[y*width+(x>>8)]) {
				sv.x1 = x>>8;
				sv.y1 = y;
				start = 1;
			}
		} else {
			if(diff[y*width+(x>>8)] == 0) {
				sv.x2 = x>>8;
				sv.y2 = y;
				start = 2;
				break;
			}
		}
		x += dx;
		y += dir;
	}
	if(start == 0) {
		sv.x1 = sv.x2 = sv.y1 = sv.y2 = 0;
	}
	if(start == 1) {
		sv.x2 = (x - dx)>>8;
		sv.y2 = y - dir;
	}
	return sv;
}

#define MARGINE 20
static struct shortvec detectEdgePoints(unsigned char *diff)
{
	int p1, p2;
	int d;

	d = fastrand()>>30;
	switch(d) {
	case 0:
		p1 = fastrand()%(video_width - MARGINE*2);
		p2 = fastrand()%(video_width - MARGINE*2);
		return scanline_dy(1, p1, p2, diff);
	case 1:
		p1 = fastrand()%(video_width - MARGINE*2);
		p2 = fastrand()%(video_width - MARGINE*2);
		return scanline_dy(-1, p1, p2, diff);
	case 2:
		p1 = fastrand()%(video_height - MARGINE*2);
		p2 = fastrand()%(video_height - MARGINE*2);
		return scanline_dx(1, p1, p2, diff);
	default:
	case 3:
		p1 = fastrand()%(video_height - MARGINE*2);
		p2 = fastrand()%(video_height - MARGINE*2);
		return scanline_dx(-1, p1, p2, diff);
	}
}

static int setBackground()
{
	if(video_syncframe())
		return -1;
	image_bgset_y((RGB32 *)video_getaddress());
	if(video_grabframe())
		return -1;

	return 0;
}

effect *sparkRegister()
{
	effect *entry;

	entry = (effect *)malloc(sizeof(effect));
	if(entry == NULL) {
		return NULL;
	}
	
	entry->name = effectname;
	entry->start = sparkStart;
	entry->stop = sparkStop;
	entry->draw = sparkDraw;
	entry->event = sparkEvent;

	return entry;
}

int sparkStart()
{
	int i;
	
	for(i=0; i<POINT_MAX; i++) {
		pp[i] = 0;
	}
	for(i=0; i<SPARK_MAX; i++) {
		sparks_life[i] = 0;
	}
	sparks_head = 0;
	image_set_threshold_y(40);
	if(video_grabstart())
		return -1;
	if(setBackground())
		return -1;

	stat = 1;
	return 0;
}

int sparkStop()
{
	if(stat) {
		video_grabstop();
		stat = 0;
	}

	return 0;
}

int sparkDraw()
{
	int i;
	RGB32 *src, *dest;
	unsigned char *diff;
	struct shortvec sv;

	if(video_syncframe())
		return -1;
	if(screen_mustlock()) {
		if(screen_lock() < 0) {
			return 0;
		}
	}
	src = (RGB32 *)video_getaddress();
	if(stretch) {
		dest = stretching_buffer;
	} else {
		dest = (RGB32 *)screen_getaddress();
	}

	diff = image_diff_filter(image_bgsubtract_y(src));
	bcopy(src, dest, video_area*sizeof(RGB32));
	if(video_grabframe())
		return -1;

	sv = detectEdgePoints(diff);
	if((inline_fastrand()&0x10000000) == 0) {
		if(shortvec_length2(sv)>400) {
			sparks[sparks_head] = sv;
			sparks_life[sparks_head] = (inline_fastrand()>>29) + 2;
			sparks_head = (sparks_head+1) % SPARK_MAX;
		}
	}
	for(i=0; i<SPARK_MAX; i++) {
		if(sparks_life[i]) {
			draw_spark(sparks[i], dest, video_width, video_height);
			sparks_life[i]--;
		}
	}
	
	if(stretch) {
		image_stretch_to_screen();
	}
	if(screen_mustlock()) {
		screen_unlock();
	}

	return 0;
}

int sparkEvent(SDL_Event *event)
{
	if(event->type == SDL_KEYDOWN) {
		switch(event->key.keysym.sym) {
		case SDLK_SPACE:
			setBackground();
			break;
		default:
			break;
		}
	}
	return 0;
}
