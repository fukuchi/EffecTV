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
