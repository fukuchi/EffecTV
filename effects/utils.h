/*
 * EffecTV - Realtime Digital Video Effector
 * Copyright (C) 2001 FUKUCHI Kentarou
 *
 * utils.h: header file for utils
 *
 */

/*
 * utils.c
 */
void HSI2RGB(double H, double S, double I, int *r, int *g, int *b);

unsigned int fastrand();
void fastsrand(unsigned int);

/*
 * buffer.c
 */

int sharedbuffer_init(int);
void sharedbuffer_reset();
unsigned char *sharedbuffer_alloc(int);

/*
 * yuv.c
 */

extern int YtoRGB[256];
extern int VtoR[256], VtoG[256];
extern int UtoG[256], UtoB[256];
extern int RtoY[256], RtoU[256], RtoV[256];
extern int GtoY[256], GtoU[256], GtoV[256];
extern int BtoY[256],            BtoV[256];

int yuvTableInit();
unsigned char RGBtoY(int);
