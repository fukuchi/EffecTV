/*
 * EffecTV - Realtime Digital Video Effector
 * Copyright (C) 2001-2006 FUKUCHI Kentaro
 *
 * yuv.c: YUV(YCbCr) color system utilities
 *
 */

#include "EffecTV.h"
#include "utils.h"

/*
 * conversion from YUV to RGB
 *   r = 1.164*(y-16) + 1.596*(v-128);
 *   g = 1.164*(y-16) - 0.813*(v-128) - 0.391*(u-128);
 *   b = 1.164*(y-16)                 + 2.018*(u-128);
 * conversion from RGB to YUV
 *   y =  0.257*r + 0.504*g + 0.098*b + 16
 *   u = -0.148*r - 0.291*g + 0.439*b + 128
 *   v =  0.439*r - 0.368*g - 0.071*b + 128
 */

int YtoRGB[256];
int VtoR[256], VtoG[256];
int UtoG[256], UtoB[256];
int RtoY[256], RtoU[256], RtoV[256];
int GtoY[256], GtoU[256], GtoV[256];
int BtoY[256],            BtoV[256];

int yuv_init(void)
{
	int i;

	for(i=0; i<256; i++) {
		YtoRGB[i] =  1.164*(i-16);
		VtoR[i] =  1.596*(i-128);
		VtoG[i] = -0.813*(i-128);
		UtoG[i] = -0.391*(i-128);
		UtoB[i] =  2.018*(i-128);
		RtoY[i] =  0.257*i;
		RtoU[i] = -0.148*i;
		RtoV[i] =  0.439*i;
		GtoY[i] =  0.504*i;
		GtoU[i] = -0.291*i;
		GtoV[i] = -0.368*i;
		BtoY[i] =  0.098*i;
		BtoV[i] = -0.071*i;
	}

	return 0;
}

unsigned char yuv_RGBtoY(int rgb)
{
	int i;

	i =  RtoY[(rgb>>16)&0xff]
	   + GtoY[(rgb>>8)&0xff]
	   + BtoY[rgb&0xff]
	   + 16;

	return i;
}

unsigned char yuv_RGBtoU(int rgb)
{
	int i;

	i =  RtoU[(rgb>>16)&0xff]
	   + GtoU[(rgb>>8)&0xff]
	   + RtoV[rgb&0xff] /* BtoU == RtoV */
	   + 128;

	return i;
}

unsigned char yuv_RGBtoV(int rgb)
{
	int i;

	i =  RtoV[(rgb>>16)&0xff]
	   + GtoV[(rgb>>8)&0xff]
	   + BtoV[rgb&0xff]
	   + 128;

	return i;
}
