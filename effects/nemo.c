/*
 * EffecTV - Realtime Digital Video Effector
 * Copyright (C) 2001 FUKUCHI Kentarou
 *
 * nemo.c: a effect I saw last week in a science-discovery-for-kids-style
 * museum called Nemo in Amsterdam, the Netherlands.
 *
 * Ico Doornekamp - ico@zevv.nl
 *
 * ---------
 *  2002/03/07
 *
 * Shamelessly grabbed 99% of the code from the spiral effect
 * 
 */

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "../EffecTV.h"
#include "utils.h"


int nemoStart();
int nemoStop();
int nemoDraw();

int nemoEvent();

static char *effectname_base = "NemoTV";
static char effectname[128] = "";
static int state = 0;
static unsigned int *buffer;
static unsigned int *planetable[512];
static int planes;
static int plane;

static int *depthmap;
static int mode = 0;


static int g_cursor_state = SDL_DISABLE;
static int g_cursor_local = SDL_DISABLE;


void nemoSetName()
{
            sprintf(effectname, "Nemo");
            screen_setcaption(effectname);
}
    
effect *nemoRegister()
{
	effect *entry;
    
	sharedbuffer_reset();
	depthmap = (int *)sharedbuffer_alloc(video_width * video_height * sizeof(int));
	if(depthmap == NULL) {
		return NULL;
	}

	entry = (effect *)malloc(sizeof(effect));
	if(entry == NULL) {
		free(buffer);
		return NULL;
	}

    strcpy(effectname, effectname_base);
	entry->name = effectname;
	entry->start = nemoStart;
	entry->stop = nemoStop;
	entry->draw = nemoDraw;
    entry->event = nemoEvent;

	return entry;
}

int nemoStart()
{
	int i;
    int screen_area;

    screen_area = video_width * video_height;
    /*
    ** Allocate space for the frame buffers.  An awful lot of memory is required -
    */
    
    planes = video_height;
    
    
    printf("Reserving %.1f Mb for %d planes\n", 
    	(screen_area * PIXEL_SIZE * planes)/(1024*1024.0),
    	planes);
    
	buffer = (unsigned int *)malloc(screen_area * PIXEL_SIZE * planes);
	if(buffer == NULL) { 
		fprintf(stderr, "malloc failed\n");
		return -1;
	}

    /*
    ** Set up the array of pointers to the frame buffers
    */
	for(i=0;i<planes;i++) {
		planetable[i] = &buffer[screen_area * i];
    }

	plane = 0;

	if (video_grabstart())
    {
		return -1;
    }
        

    g_cursor_state = SDL_ShowCursor(SDL_QUERY);
    SDL_ShowCursor(g_cursor_local);
    nemoSetName();
	state = 1;
	
	return 0;
}

int nemoStop()
{
	if(state) {
		video_grabstop();
        
		if(buffer)
        {
			free(buffer);
        }

        SDL_ShowCursor(g_cursor_state);
        state = 0;
	}

	return 0;
}

int nemoDraw()
{
    int x, y, i;
	int cf;
	unsigned int *src, *dest;

	if(video_syncframe())
		return -1;
	src = (unsigned int *)video_getaddress();

	if(stretch) {
		dest = stretching_buffer;
	} else {
		dest = (RGB32 *)screen_getaddress();
	}
    
	bcopy(src, planetable[plane], video_width * video_height * PIXEL_SIZE);
	if(video_grabframe())
		return -1;
	if(screen_mustlock()) {
		if(screen_lock() < 0) {
			return 0;
		}
	}

	i = 0;
	for(y = 0; y < video_height; y++) {
		for(x = 0; x < video_width; x++) {
			cf =  (int)(y*((planes*1.0)/(video_height*1.0))+plane) % planes;
			dest[i] = (planetable[cf])[i];
			i++;
		}
	}
    

	if(stretch) {
		image_stretch_to_screen();
	}

	if(screen_mustlock()) {
		screen_unlock();
	}
	plane--;
	if(plane < 0) {
		plane = planes-1;
	}

	return 0;
}


int nemoEvent(SDL_Event *event)
{
	if(event->type == SDL_KEYDOWN) {
		switch(event->key.keysym.sym) {
		case SDLK_SPACE:
            mode++;

            nemoSetName();
    	   break;

		default:
			break;
		}
	}
    else if (SDL_MOUSEBUTTONDOWN == event->type)
    {
            // Toggle the mouse pointer visibility
            if (SDL_DISABLE == g_cursor_local)
            {
                g_cursor_local = SDL_ENABLE;
            }
            else
            {
                g_cursor_local = SDL_DISABLE;
            }
            SDL_ShowCursor(g_cursor_local);
    }
    
	return 0;
}

// end