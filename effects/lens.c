/*
 * EffecTV - Realtime Digital Video Effector
 * Copyright (C) 2001-2003 FUKUCHI Kentaro
 *
 * lensTV - old skool Demo lens Effect
 * Code taken from "The Demo Effects Colletion" 0.0.4
 * http://www.paassen.tmfweb.nl/retrodemo.html
 *
 *
 * Ported to EffecTV BSB by Buddy Smith
 * Modified from BSB for EffecTV 0.3.x by Ed Tannenbaaum
 * ET added interactive control via mouse as follows....
 * Spacebar toggles interactive mode (off by default)
 * In interactive mode:
 *   Mouse with no buttons pressed moves magnifier
 *   Left button and y movement controls size of magnifier
 *   Right Button and y movement controls magnification.
 *
 * This works best in Fullscreen mode due to mouse trapping
 *
 * You can now read the fine print in the TV advertisements!
 */

#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "EffecTV.h"
#include "utils.h"

static int start(void);
static int stop(void);
static int draw(RGB32 *src, RGB32 *dst);
static int event();
static void init();
static int x=16,y=16;
static int xd=5,yd=5;
static int lens_width=150;
static int lens_zoom = 30;

static char *effectname = "lensTV";
static int state = 0;
static int *lens = NULL;
static int mode ;

static void apply_lens(int ox, int oy,RGB32 *src,RGB32 *dst)
{
    int x, y, noy,pos, nox;
	int *p;

	p = lens;
	for (y = 0; y < lens_width; y++) {
		for (x = 0; x < lens_width; x++) {
			noy=(y+oy); nox=(x+ox);
			if ((nox>=0)&&(noy>=0)&&(nox<video_width)&&(noy<video_height)){
				pos = (noy * video_width) + nox;
				*(dst+pos) = *(src+pos + *p);
			}
			p++;
		}
	}
}


effect *lensRegister()
{
	effect *entry;
	mode=1;
	entry = (effect *)malloc(sizeof(effect));
	if(entry == NULL) return NULL;

	entry->name = effectname;
	entry->start = start;
	entry->stop = stop;
	entry->draw = draw;
	entry->event = event;

	return entry;
}

static int start()
{
	init();
	state = 1;
	return 0;
}

static int stop()
{
	state = 0;
	return 0;
}

static int draw(RGB32 *src, RGB32 *dst)
{
  	memcpy(dst, src, video_area * PIXEL_SIZE);
  	apply_lens(x,y,src,dst);
	if (mode==1){
  		x+= xd; y+=yd;
  		if (x > (video_width - lens_width - 5) || x < 5) xd = -xd;
  		if (y > (video_height - lens_width - 5) || y < 5) yd = -yd;
	}

  	return 0;
} 

static void init() {

  int x,y,r,d;

  if(lens != NULL) {
	  free(lens);
  }
  lens = (int *)malloc(lens_width * lens_width * sizeof(int));
  memset(lens, 0, lens_width * lens_width * sizeof(int));

    /* generate the lens distortion */
    r = lens_width/2;
    d = lens_zoom;

    /* the shift in the following expression is a function of the
     * distance of the current point from the center of the sphere.
     * If you imagine:
     *
     *       eye
     *
     *   .-~~~~~~~-.    sphere surface
     * .`           '.
     * ---------------  viewing plane
     *        .         center of sphere
     *
     * For each point across the viewing plane, draw a line from the
     * point on the sphere directly above that point to the center of
     * the sphere.  It will intersect the viewing plane somewhere
     * closer to the center of the sphere than the original point.
     * The shift function below is the end result of the above math,
     * given that the height of the point on the sphere can be derived
     * from:
     *
     * x^2 + y^2 + z^2 = radius^2
     *
     * x and y are known, z is based on the height of the viewing
     * plane.
     *
     * The radius of the sphere is the distance from the center of the
     * sphere to the edge of the viewing plane, which is a neat little
     * triangle.  If d = the distance from the center of the sphere to
     * the center of the plane (aka, lens_zoom) and r = half the width
     * of the plane (aka, lens_width/2) then radius^2 = d^2 + r^2.
     *
     * Things become simpler if we take z=0 to be at the plane's
     * height rather than the center of the sphere, turning the z^2 in
     * the expression above to (z+d)^2, since the center is now at
     * (0, 0, -d).
     *
     * So, the resulting function looks like:
     *
     * x^2 + y^2 + (z+d)^2 = d^2 + r^2
     *
     * Expand the (z-d)^2:
     *
     * x^2 + y^2 + z^2 + 2dz + d^2 = d^2 + r^2
     *
     * Rearrange things to be a quadratic in terms of z:
     *
     * z^2 + 2dz + x^2 + y^2 - r^2 = 0
     *
     * Note that x, y, and r are constants, so apply the quadratic
     * formula:
     *
     * For ax^2 + bx + c = 0,
     * 
     * x = (-b +- sqrt(b^2 - 4ac)) / 2a
     *
     * We can ignore the negative result, because we want the point at
     * the top of the sphere, not at the bottom.
     *
     * x = (-2d + sqrt(4d^2 - 4 * (x^2 + y^2 - r^2))) / 2
     *
     * Note that you can take the -4 out of both expressions in the
     * square root to put -2 outside, which then cancels out the
     * division:
     *
     * z = -d + sqrt(d^2 - (x^2 + y^2 - r^2))
     *
     * This now gives us the height of the point on the sphere
     * directly above the equivalent point on the plane.  Next we need
     * to find where the line between this point and the center of the
     * sphere at (0, 0, -d) intersects the viewing plane at (?, ?, 0).
     * This is a matter of the ratio of line below the plane vs the
     * total line length, multiplied by the (x,y) coordinates.  This
     * ratio can be worked out by the height of the line fragment
     * below the plane, which is d, and the total height of the line,
     * which is d + z, or the height above the plane of the sphere
     * surface plus the height of the plane above the center of the
     * sphere.
     *
     * ratio = d/(d + z)
     *
     * Subsitute in the formula for z:
     *
     * ratio = d/(d + -d + sqrt(d^2 - (x^2 + y^2 - r^2))
     *
     * Simplify to:
     *
     * ratio = d/sqrt(d^2 - (x^2 + y^2 - r^2))
     *
     * Since d and r are constant, we now have a formula we can apply
     * for each (x,y) point within the sphere to give the (x',y')
     * coordinates of the point we should draw to project the image on
     * the plane to the surface of the sphere.  I subtract the
     * original (x,y) coordinates to give an offset rather than an
     * absolute coordinate, then convert that offset to the image
     * dimensions, and store the offset in a matrix the size of the
     * intersecting circle.  Drawing the lens is then a matter of:
     *
     * screen[coordinate] = image[coordinate + lens[y][x]]
     *
     */

    /* it is sufficient to generate 1/4 of the lens and reflect this
     * around; a sphere is mirrored on both the x and y axes */
    for (y = 0; y < r; y++) {
        for (x = 0; x < r; x++) {
            int ix, iy, offset, dist;
			dist = x*x + y*y - r*r;
			if(dist < 0) {
                double shift = d/sqrt(d*d - dist);
                ix = x * shift - x;
                iy = y * shift - y;
            } else {
                ix = 0;
                iy = 0;
            }
            offset = (iy * video_width + ix);
			lens[(r - y)*lens_width + r - x] = -offset;
			lens[(r + y)*lens_width + r + x] = offset;
            offset = (-iy * video_width + ix);
			lens[(r + y)*lens_width + r - x] = -offset;
			lens[(r - y)*lens_width + r + x] = offset;
        }
    }
}

static void clipmag()
{
	if (y<0-(lens_width/2)+1)y=0-(lens_width/2)+1;
	if (y>=video_height-lens_width/2-1)y=video_height-lens_width/2-1;

	if (x<0-(lens_width/2)+1) x=0-lens_width/2+1;
	if(x>=video_width-lens_width/2-1)x=video_width-lens_width/2-1;
}

static int event(SDL_Event *event)
{
		if(event->type == SDL_KEYDOWN) {
			switch(event->key.keysym.sym) {

			  	case SDLK_SPACE:
					mode=!mode;

                        		//fprintf(stdout,"mode=%d\n",mode);
				break;

				default: 
				break;

			}
		}

		else { 
			if(mode==0){
			   if (SDL_MOUSEMOTION == event->type){

				switch(event->button.button) {
          				case SDL_BUTTON_LEFT:
						lens_width = lens_width + (event->motion.yrel);
						if (lens_width>video_height)lens_width=video_height;
						if (lens_width<3)lens_width=3;
						init();
						clipmag();

					break;

					case 2: // Right button on 2 button mice / middle on three?
        				case 4: // Right on  my wheel mouuse
						//lens_width = event->button.y;
						lens_zoom = lens_zoom + event->motion.yrel;
						if (lens_zoom<5) lens_zoom=5;
						if (lens_zoom>200) lens_zoom=200;
						init();
						//fprintf(stdout,"y=%d\n",y);fprintf(stdout,"x=%d\n",x);
					break;
					default:
						y = y+(event->motion.yrel);
						x = x+(event->motion.xrel);
						clipmag();
					//fprintf(stdout,"y=%d\n",y);fprintf(stdout,"x=%d\n",x);
					break;


				}
			   }
			}



    	}

	return 0;
}
