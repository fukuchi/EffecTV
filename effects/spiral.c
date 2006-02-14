/*
 * EffecTV - Realtime Digital Video Effector
 * Copyright (C) 2001-2006 FUKUCHI Kentaro
 *
 * spiral.c: a 'spiraling' effect (even though it isn't really a spiral)
 *  code originally derived from quark.c; additions and changes are
 *  copyright (c) 2001 Sam Mertens.  This code is subject to the provisions of
 *  the GNU Public License.
 *
 * ---------
 *  2001/04/20
 *      The code looks to be about done.  The extra junk I add to the
 *      title bar looks ugly - I just don't know how much to take off.
 *      The TAB key no longer toggles animation: 'A' does.
 *      A lot of these comments can probably be removed/reduced for the next
 *      stable release.
 * ---------
 *  2001/04/16
 *      I've made quick adjustments to my most recent 'experimental' code so
 *      it'll compile with the latest EffecTV-BSB code.  More proper
 *      intergration will commence shortly.
 *      Animation (more accurately, automatic movement of the wave loci along
 *      a fixed path) is implemented.  I'm dissapointed by the results, though.
 *      The following temporary additions have been made to the user interface
 *      to allow testing of the animation:
 *        The window title now displays the current waveform, plus
 *        several animation parameters.  Those parameters are formatted as
 *        "(%0.2fint,%df, %dd)"; the first number is the size of the interval
 *        between steps along the closed path.  The second number is the
 *        number of frames to wait before each step.  The third number is
 *        a quick and dirty way to control wave amplitude; wave table indices
 *        are bitshifted right by that value.
 *      The TAB key toggles animation on and off; animation is off by
 *      default.
 *      INSERT/DELETE increment and decrement the step interval value,
 *      respectively.
 *
 *      HOME/END increment and decrement the # of frames between each
 *      movement of the waves' centerpoint, respectively.
 *
 *      PAGE UP/PAGE DOWN increment and decrement the amount that wave table
 *      entries are bitshifted by, respectively.
 *
 *  Recent changes in the user interface:
 *  1. Hitting space will now cycle among 8 different wave shapes.
 *      The active waveshape's name is displayed in the titlebar.
 *  2. The mouse can be used to recenter the effect.  Left-clicking
 *      moves the center of the effect (which defaults to the center
 *      of the video image) to the mouse pointer's location.  Any other
 *      mouse button will toggle mouse visibility, so the user can see
 *      where they're clicking.
 *  3. The keys '1','2','3','4' and '0' also move the effect center.
 *      '1' through '4' move the center midway between the middle of the
 *      image and each of the four corners, respectively. '0' returns
 *      the center to its default position.
 *
 *	-Sam Mertens
 */

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "EffecTV.h"
#include "utils.h"

#ifndef max
#define max(a,b)    (((a) > (b)) ? (a) : (b))
#endif

#ifndef M_PI
#define M_PI 3.141592653589793238462643383
#endif /* M_PI */

/* Several values must be powers of 2. The *_POWER predefines keep them so. */

#define PLANE_POWER         (4)     // 2 exp 4 = 16
#define WAVE_COUNT_POWER    (3)     // 2 exp 3 = 8
#define WAVE_LENGTH_POWER   (9)     // 2 exp 9 = 512

#define PLANES              (1 << PLANE_POWER)  // 16
#define PLANE_MASK          (PLANES - 1)
#define PLANE_MAX           (PLANES - 1)

#define WAVE_COUNT          (1 << WAVE_COUNT_POWER)   // 8
#define WAVE_MASK           (WAVE_COUNT - 1)
#define WAVE_MAX            (WAVE_COUNT - 1)

#define WAVE_LENGTH         (1 << WAVE_LENGTH_POWER) // 512
#define WAVE_LENGTH_MASK    (WAVE_LENGTH - 1)


#define WAVE_CONCENTRIC_A       0
#define WAVE_SAWTOOTH_UP        1
#define WAVE_SAWTOOTH_DOWN      2 
#define WAVE_TRIANGLE           3

#define WAVE_SINUS              4
#define WAVE_CONCENTRIC_B       5
#define WAVE_LENS               6
#define WAVE_FLAT               7

/* The *_OFFSET predefines are just precalculations.  There shouldn't normally
** be any need to change them.
*/

#define WAVE_CONCENTRIC_A_OFFSET    (WAVE_CONCENTRIC_A * WAVE_LENGTH)
#define WAVE_SAW_UP_OFFSET          (WAVE_SAWTOOTH_UP * WAVE_LENGTH)
#define WAVE_SAW_DOWN_OFFSET        (WAVE_SAWTOOTH_DOWN * WAVE_LENGTH)
#define WAVE_TRIANGLE_OFFSET        (WAVE_TRIANGLE * WAVE_LENGTH)

#define WAVE_CONCENTRIC_B_OFFSET    (WAVE_CONCENTRIC_B * WAVE_LENGTH)
#define WAVE_LENS_OFFSET            (WAVE_LENS * WAVE_LENGTH)
#define WAVE_SINUS_OFFSET           (WAVE_SINUS * WAVE_LENGTH)
#define WAVE_FLAT_OFFSET            (WAVE_FLAT * WAVE_LENGTH)

typedef char WaveEl;

#define WAVE_ELEMENT_SIZE       (sizeof(WaveEl))
#define WAVE_TABLE_SIZE         (WAVE_COUNT * WAVE_LENGTH * WAVE_ELEMENT_SIZE)

#define FOCUS_INCREMENT_PRESET  (M_PI/2.0)

static int start(void);
static int stop(void);
static int draw(RGB32 *src, RGB32 *dest);
static int event(SDL_Event *event);

static void spiralCreateMap();
static WaveEl* spiralDefineWaves();
static void spiralMoveFocus();

static char *effectname_base = "SpiralTV";
static char effectname[128] = "";
static int state = 0;
static unsigned int *buffer;
static unsigned int *planetable[PLANES];
static int plane;

static int *depthmap;
static int mode = 0;

/*
**  'g_' is for global.  I'm trying to reduce namespace pollution
**  within the spiral module.
**
*/
static int g_focus_x = 0;
static int g_focus_y = 0;

static WaveEl* g_wave_table = NULL;
static const char* g_wave_names[WAVE_COUNT] = { "Concentric A",
                                         "Sawtooth Up",
                                         "Sawtooth Down",
                                         "Triangle",
                                         "Sinusoidal",
                                         "Concentric B",
                                         "Lens",
                                         "Flat"
};

static int g_cursor_state = SDL_DISABLE;
static int g_cursor_local = SDL_DISABLE;

static int g_toggle_xor = 0;
static int g_animate_focus = 0;
static int g_focus_interval = 6;
static int g_focus_counter = 0;
static unsigned int g_depth_shift = 0; // Cheesy way to adjust intensity

// The following are needed only for this specific animation algorithm
static int g_focus_radius = 100;
static double g_focus_degree = 1.0;
static double g_focus_increment = FOCUS_INCREMENT_PRESET;

static void spiralSetName(void)
{
            sprintf(effectname, "%s:%s (%0.2fi/%df/%dd)", effectname_base,
                    g_wave_names[mode],
                    g_focus_increment, g_focus_interval, g_depth_shift);
            
            screen_setcaption(effectname);
}
    
effect *spiralRegister(void)
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
	entry->start = start;
	entry->stop = stop;
	entry->draw = draw;
    entry->event = event;

    g_focus_x = (video_width/2);
    g_focus_y = (video_height/2);
	return entry;
}

static int start(void)
{
	int i;

    g_focus_radius = video_width / 2;
    /*
    ** Allocate space for the frame buffers.  A lot of memory is required -
    ** with the default settings, it totals nearly 5 megs.
    **  (320 * 240 * 4 * 16) = 4915200 bytes
    **
    ** Multiply by 4 for 640x480!
    */
	buffer = (unsigned int *)malloc(video_area * PIXEL_SIZE * PLANES);
	if(buffer == NULL)
		return -1;
	memset(buffer, 0, video_area * PIXEL_SIZE * PLANES);

    /*
    ** Set up the array of pointers to the frame buffers
    */
	for(i=0;i<PLANES;i++) {
		planetable[i] = &buffer[video_area * i];
    }

    /*
    **  We call this code here, and not earlier, so that we don't
    **  waste memory unnecessarily.  (Although it's a trivial amount: 4k)
    */
    if (NULL == g_wave_table)
    {
        g_wave_table = spiralDefineWaves();
        if (NULL == g_wave_table)
        {
            free(buffer);
            return -1;
        }
    }
    
    spiralCreateMap();
    
	plane = PLANE_MAX;

#ifdef DEBUG 
    v4lprint(&vd);
#endif

    g_cursor_state = SDL_ShowCursor(SDL_QUERY);
    SDL_ShowCursor(g_cursor_local);
    spiralSetName();
	state = 1;
	return 0;
}

static int stop(void)
{
	if(state) {
		if(buffer)
        {
			free(buffer);
        }

        /*
        ** Prevent a small memory leak (4k)
        */
        if (NULL != g_wave_table)
        {
            free(g_wave_table);
            g_wave_table = NULL;
        }
        
        SDL_ShowCursor(g_cursor_state);
        state = 0;
	}

	return 0;
}

static int draw(RGB32 *src, RGB32 *dest)
{
    int x, y, i;
	int cf;

	memcpy(planetable[plane], src, video_width * video_height * PIXEL_SIZE);

    if (g_animate_focus)
    {
        spiralMoveFocus();
    }
    
	i = 0;
	for(y = 0; y < video_height; y++) {
		for(x = 0; x < video_width; x++) {
			cf = (plane + depthmap[i]) & PLANE_MASK;
			dest[i] = (planetable[cf])[i];
			i++;
		}
	}

	plane--;
	plane &= PLANE_MASK;

	return 0;
}


static int event(SDL_Event *event)
{
	if(event->type == SDL_KEYDOWN) {
		switch(event->key.keysym.sym) {
		case SDLK_SPACE:
            mode++;
            mode &= WAVE_MASK;

            spiralSetName();
            spiralCreateMap();
			break;

        case SDLK_a:
            g_animate_focus = ~g_animate_focus;
            break;

        case SDLK_x:
            g_toggle_xor = ~g_toggle_xor;
            break;
            
        case SDLK_0:
            g_focus_y = (video_height/2);
            g_focus_x = (video_width/2);
            g_focus_increment = FOCUS_INCREMENT_PRESET;
            spiralCreateMap();
            spiralSetName();
            break;

        case SDLK_INSERT:
            g_focus_increment *= 1.25;
            spiralSetName();
            break;
            
        case SDLK_DELETE:
            g_focus_increment *= 0.80;
            spiralSetName();
            break;

        case SDLK_HOME:
            g_focus_interval++;
            if (60 < g_focus_interval)
            {
                g_focus_interval = 60;      // More than enough, I think
            }
            spiralSetName();
            break;
            
        case SDLK_END:
            g_focus_interval--;
            if (0 >= g_focus_interval)
            {
                g_focus_interval = 1;       // Smaller would be pointless
            }
            spiralSetName();
            break;

        case SDLK_PAGEUP:
            if (5 > g_depth_shift)
            {
                g_depth_shift++;
            }
            spiralSetName();
            spiralCreateMap();
            break;
            
        case SDLK_PAGEDOWN:
            if (0 < g_depth_shift)
            {
                g_depth_shift--;
            }
            spiralSetName();
            spiralCreateMap();
            break;
            
        case SDLK_1:
            g_focus_y = video_height/4;
            g_focus_x = video_width/4;
            spiralCreateMap();
            break;
            
        case SDLK_2:
            g_focus_y = video_height/4;
            g_focus_x = 3 * video_width/4;
            spiralCreateMap();
            break;
            
        case SDLK_3:
            g_focus_y = 3 * video_height/4;
            g_focus_x = video_width/4;
            spiralCreateMap();
            break;
            
        case SDLK_4:
            g_focus_y = 3 * video_height/4;
            g_focus_x = 3 * video_width/4;
            spiralCreateMap();
            break;
            
		default:
			break;
		}
	}
    else if (SDL_MOUSEBUTTONDOWN == event->type)
    {
        if (SDL_BUTTON_LEFT == event->button.button)
        {
            g_focus_y = event->button.y / screen_scale;
            g_focus_x = event->button.x / screen_scale;
            spiralCreateMap();
        }
        else
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
    }
    
	return 0;
}

static void spiralCreateMap(void)
{
    int x;
    int y;
    int rel_x;
    int rel_y;
    int yy;
    float x_ratio;
    float y_ratio;
    int v;
    int i;
    int wave_offset;
    
    /*
    ** The following code generates the default depth map.
    */
    i = 0;
    wave_offset = mode * WAVE_LENGTH;

    x_ratio = 320.0 / video_width;
    y_ratio = 240.0 / video_height;
    
	for (y=0; y<video_height; y++) {

        rel_y = (g_focus_y - y) * y_ratio;
        yy = rel_y * rel_y;
        
		for(x=0; x<video_width; x++) {
            rel_x = (g_focus_x - x) * x_ratio;
#ifdef PS2
            v = ((int)sqrtf(yy + rel_x*rel_x)) & WAVE_LENGTH_MASK;
#else
            v = ((int)sqrt(yy + rel_x*rel_x)) & WAVE_LENGTH_MASK;
#endif
            depthmap[i++] = g_wave_table[wave_offset + v] >> g_depth_shift;
		}
	}
    
    return;
}

static WaveEl* spiralDefineWaves(void)
{
    WaveEl* wave_table;
    int     i;
    int     w;
    int     iw;
#ifdef PS2
    float  sinus_val = M_PI/2.0;
#else
    double  sinus_val = M_PI/2.0;
#endif

    // This code feels a little like a hack, but at least it contains
    // all like-minded hacks in one place.
    
    wave_table = (WaveEl*)malloc(WAVE_TABLE_SIZE);
    if (NULL == wave_table)
    {
        return NULL;
    }

    w = ((int)sqrt(video_height * video_height + video_width * video_width));
    for (i = 0; i < WAVE_LENGTH; i++)
    {
        // The 'flat' wave is very easy to compute :)
        wave_table[WAVE_FLAT_OFFSET + i] = 0;
        
        wave_table[WAVE_SAW_UP_OFFSET + i] = i & PLANE_MASK;
        wave_table[WAVE_SAW_DOWN_OFFSET + i] = PLANE_MAX - (i & PLANE_MASK);
        if (i & PLANES)
        {
            wave_table[WAVE_TRIANGLE_OFFSET + i] = (~i) & PLANE_MASK;
        }
        else
        {
            wave_table[WAVE_TRIANGLE_OFFSET + i] = i & PLANE_MASK;
        }

        iw = i / (w/(PLANES*2));

        if (iw & PLANES)
        {
            wave_table[WAVE_CONCENTRIC_A_OFFSET + i] = (~iw) & PLANE_MASK;
        }
        else
        {
            wave_table[WAVE_CONCENTRIC_A_OFFSET + i] = iw & PLANE_MASK;
        }

        wave_table[WAVE_CONCENTRIC_B_OFFSET + i] = (i*PLANES)/w;
        
        wave_table[WAVE_LENS_OFFSET + i] = i >> 3;

#ifdef PS2
        wave_table[WAVE_SINUS_OFFSET + i] = ((PLANES/2) +
                                             (int)((PLANES/2 - 1) * sinf(sinus_val))) & PLANE_MASK;
#else
        wave_table[WAVE_SINUS_OFFSET + i] = ((PLANES/2) +
                                             (int)((PLANES/2 - 1) * sin(sinus_val))) & PLANE_MASK;
#endif
        sinus_val += M_PI/PLANES;
    }
    
    return (wave_table);
}

static void spiralMoveFocus(void)
{
    g_focus_counter++;

    //  We'll only switch maps every X frames.
    if (g_focus_interval <= g_focus_counter)
    {
        g_focus_counter = 0;

        g_focus_x = (g_focus_radius * cos(g_focus_degree)) + (video_width/2);
        
        g_focus_y = (g_focus_radius * sin(g_focus_degree*2.0)) + (video_height/2);

        spiralCreateMap();
        g_focus_degree += g_focus_increment;
        if ((2.0*M_PI) <= g_focus_degree)
        {
            g_focus_degree -= (2.0*M_PI);
        }
    }
    return;
}
