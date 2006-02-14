/*
 * EffecTV - Realtime Digital Video Effector
 * Copyright (C) 2001-2006 FUKUCHI Kentaro
 *
 * dice.c: a 'dicing' effect
 *  copyright (c) 2001 Sam Mertens.  This code is subject to the provisions of
 *  the GNU Public License.
 *
 * I suppose this looks similar to PuzzleTV, but it's not. The screen is
 * divided into small squares, each of which is rotated either 0, 90, 180 or
 * 270 degrees.  The amount of rotation for each square is chosen at random.
 *
 * Controls:
 *      c   -   shrink the size of the squares, down to 1x1.
 *      v   -   enlarge the size of the squares, up to 32x32.
 *      space - generate a new random rotation map.
 *
 */

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "EffecTV.h"
#include "utils.h"

#ifndef max
#define max(a,b)    (((a) > (b)) ? (a) : (b))
#endif

#define DEFAULT_CUBE_BITS   4
#define MAX_CUBE_BITS       5
#define MIN_CUBE_BITS       0


typedef enum _dice_dir {
    Up = 0,
    Right = 1,
    Down = 2,
    Left = 3
} DiceDir;

static int start(void);
static int stop(void);
static int draw(RGB32 *src, RGB32 *dest);
static int event(SDL_Event *event);
static void diceCreateMap(void);

static char *effectname = "DiceTV";
static int state = 0;

static char* dicemap;

static int g_cube_bits = DEFAULT_CUBE_BITS;
static int g_cube_size = 0;
static int g_map_height = 0;
static int g_map_width = 0;

effect *diceRegister(void)
{
	effect *entry;

	sharedbuffer_reset();
	dicemap = (char*)sharedbuffer_alloc(video_height * video_width * sizeof(char));
	if(dicemap == NULL) {
		return NULL;
	}

    diceCreateMap();
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
    diceCreateMap();
    
#ifdef DEBUG 
    v4lprint(&vd);
#endif
    

    state = 1;
	return 0;
}

static int stop(void)
{
    state = 0;
	return 0;
}

static int draw(RGB32 *src, RGB32 *dest)
{
    int i;
    int map_x, map_y, map_i;
    int base;
    int dx, dy, di;

	map_i = 0;
	for(map_y = 0; map_y < g_map_height; map_y++)
    {
		for(map_x = 0; map_x < g_map_width; map_x++)
        {
            base = (map_y << g_cube_bits) * video_width + (map_x << g_cube_bits);
            switch (dicemap[map_i])
            {
            case Up:
                // fprintf(stderr, "U");
                for (dy = 0; dy < g_cube_size; dy++)
                {
                    i = base + dy * video_width;
                    for (dx = 0; dx < g_cube_size; dx++)
                    {
                        dest[i] = src[i];
                        i++;
                    }
                }
                break;
            case Left:
                // fprintf(stderr, "L");
                for (dy = 0; dy < g_cube_size; dy++)
                {
                    i = base + dy * video_width;
                    for (dx = 0; dx < g_cube_size; dx++)
                    {
                        di = base + (dx * video_width) + (g_cube_size - dy - 1);
                        dest[di] = src[i];
                        i++;
                    }
                }
                break;
            case Down:
                // fprintf(stderr, "D");
                for (dy = 0; dy < g_cube_size; dy++)
                {
                    di = base + dy * video_width;
                    i = base + (g_cube_size - dy - 1) * video_width + g_cube_size;
                    for (dx = 0; dx < g_cube_size; dx++)
                    {
                        i--;
                        dest[di] = src[i];
                        di++;
                    }
                }
                break;
            case Right:
                // fprintf(stderr, "R");
                for (dy = 0; dy < g_cube_size; dy++)
                {
                    i = base + (dy * video_width);
                    for (dx = 0; dx < g_cube_size; dx++)
                    {
                        di = base + dy + (g_cube_size - dx - 1) * video_width;
                        dest[di] = src[i];
                        i++;
                    }
                }
                break;
            default:
                // fprintf(stderr, "E");
                // This should never occur
                break;
            }
			map_i++;
		}
        // fprintf(stderr,"\n");
	}

	return 0;
}


static int event(SDL_Event *event)
{
	if(event->type == SDL_KEYDOWN) {
		switch(event->key.keysym.sym) {
        case SDLK_c:
            if (MIN_CUBE_BITS < g_cube_bits)
            {
                g_cube_bits--;
                diceCreateMap();
            }
            break;
        case SDLK_v:
            if (MAX_CUBE_BITS > g_cube_bits)
            {
                g_cube_bits++;
                diceCreateMap();
            }
            break;
		case SDLK_SPACE:
            diceCreateMap();
			break;
            
		default:
			break;
		}
	}
    
	return 0;
}

static void diceCreateMap(void)
{
    int x;
    int y;
    int i;
    
    g_map_height = video_height >> g_cube_bits;
    g_map_width = video_width >> g_cube_bits;
    g_cube_size = 1 << g_cube_bits;

    i = 0;
	for (y=0; y<g_map_height; y++) {

		for(x=0; x<g_map_width; x++) {
            // dicemap[i] = ((i + y) & 0x3); /* Up, Down, Left or Right */
            dicemap[i] = (inline_fastrand() >> 24) & 0x03;
            i++;
		}
	}
    
    return;
}

