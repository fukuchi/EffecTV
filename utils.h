/*
 * EffecTV - Realtime Digital Video Effector
 * Copyright (C) 2001-2006 FUKUCHI Kentaro
 *
 * utils.h: header file for utils
 *
 */

#ifndef __UTILS_H__
#define __UTILS_H__


/* DEFINE's by nullset@dookie.net */
#define RED(n)  ((n>>16) & 0x000000FF)
#define GREEN(n) ((n>>8) & 0x000000FF)
#define BLUE(n)  ((n>>0) & 0x000000FF)
#define RGB(r,g,b) ((0<<24) + (r<<16) + (g <<8) + (b))
#define INTENSITY(n)	( ( (RED(n)+GREEN(n)+BLUE(n))/3))

/*
 * utils.c
 */
int utils_init(void);
void utils_end(void);

void HSItoRGB(double H, double S, double I, int *r, int *g, int *b);

extern unsigned int fastrand_val;
unsigned int fastrand(void);
void fastsrand(unsigned int);
#define inline_fastrand() (fastrand_val=fastrand_val*1103515245+12345)


/*
 * buffer.c
 */

int sharedbuffer_init(void);
void sharedbuffer_end(void);

/* The effects uses shared buffer must call this function at first in
 * each effect registrar.
 */
void sharedbuffer_reset(void);

/* Allocates size bytes memory in shared buffer and returns a pointer to the
 * memory. NULL is returned when the rest memory is not enough for the request.
 */
unsigned char *sharedbuffer_alloc(int);


/*
 * image.c
 */

RGB32 *stretching_buffer;
int image_init(void);
void image_end(void);
void image_stretching_buffer_clear(RGB32 color);
void image_stretch(RGB32 *, int, int, RGB32 *, int, int);
void image_stretch_to_screen(void);

void image_set_threshold_y(int threshold);
void image_bgset_y(unsigned int *src);
unsigned char *image_bgsubtract_y(unsigned int *src);
unsigned char *image_bgsubtract_update_y(unsigned int *src);

void image_set_threshold_RGB(int r, int g, int b);
void image_bgset_RGB(unsigned int *src);
unsigned char *image_bgsubtract_RGB(unsigned int *src);
unsigned char *image_bgsubtract_update_RGB(unsigned int *src);

unsigned char *image_diff_filter(unsigned char *diff);
unsigned char *image_y_over(RGB32 *src);
unsigned char *image_y_under(RGB32 *src);
unsigned char *image_edge(RGB32 *src);

void image_hflip(RGB32 *src, RGB32 *dest, int width, int height);


/*
 * yuv.c
 */

extern int YtoRGB[256];
extern int VtoR[256], VtoG[256];
extern int UtoG[256], UtoB[256];
extern int RtoY[256], RtoU[256], RtoV[256];
extern int GtoY[256], GtoU[256], GtoV[256];
extern int BtoY[256],            BtoV[256];

int yuv_init(void);
unsigned char yuv_RGBtoY(int);

#endif /* __UTILS_H__ */
