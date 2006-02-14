/*
 * EffecTV - Realtime Digital Video Effector
 * Copyright (C) 2001-2006 FUKUCHI Kentaro
 *
 * palette.h: header for pixel format converter
 *
 */

#ifndef __PALETTE_H__
#define __PALETTE_H__

typedef void palette_converter_toRGB32(unsigned char *, RGB32 *, int, int);
typedef void palette_converter_fromRGB32(RGB32 *, int, int, unsigned char *, int, int);
struct palette_converter_toRGB32_map
{
	int palette;
	palette_converter_toRGB32 (*converter);
	palette_converter_toRGB32 (*converter_hflip);
};
struct palette_converter_fromRGB32_map
{
	int palette;
	palette_converter_fromRGB32 (*converter);
};

int palette_init(void);
void palette_end(void);
int palette_check_supported_converter_toRGB32(int, palette_converter_toRGB32 **, palette_converter_toRGB32 **);
void palette_get_supported_converter_toRGB32(palette_converter_toRGB32 **, palette_converter_toRGB32 **);
palette_converter_fromRGB32 *palette_get_supported_converter_fromRGB32(int);

/* palettex - a palette support function */

struct palettelist
{
	char *tag;
	int palette;
	char *name;
};

int palettex_getpalette(const char *);

#endif /* __PALETTE_H__ */
