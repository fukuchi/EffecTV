/*
 * EffecTV - Realtime Video Effector
 * Copyright (C) 2001 FUKUCHI, Kentarou
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * main.c: start up module
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>
#include <SDL/SDL.h>

#include "EffecTV.h"
#include "effects/effects.h"
#include "utils.h"
#include "syserr.xbm"

int debug = 0;
int autoplay = 0;
int autoplay_counter;

static effectRegistFunc *effects_register_list[] =
{
	dumbRegister,
	quarkRegister,
	fireRegister,
	burnRegister,
	blurzoomRegister,
	baltanRegister,
	streakRegister,
	onedRegister,
	dotRegister,
	mosaicRegister,
	puzzleRegister,
	predatorRegister,
	spiralRegister,
	simuraRegister,
	edgeRegister,
	shagadelicRegister,
	noiseRegister,
	agingRegister,
	TransFormRegister,
	lifeRegister,
	sparkRegister,
	warpRegister,
	holoRegister,
	cycleRegister,
	rippleRegister,
	diceRegister,
	dizzyRegister
};

static effect **effectsList;
static int effectMax;
static int currentEffectNum;
static effect *currentEffect;
static int fps = 0;

static void usage()
{
	printf("EffecTV - Realtime Video Effector\n");
	printf("Version: %s\n", VERSION_STRING);
	printf("Usage: effectv [options...]\n");
	printf("Options:\n");
	printf("  -device FILE     use device FILE for video4linux\n");
	printf("  -channel NUMBER  channel number of video source\n");
	printf("  -norm {ntsc,pal,secam,pal-nc,pal-m,pal-n,ntsc-jp}
                   set video norm\n");
	printf("  -freqtab {us-bcast,us-cable,us-cable-hrc,japan-bcast,japan-cable,europe-west,
            europe-east,italy,newzealand,australia,ireland,france,china-bcast,
            southafrica,argentina}
                   set frequency table\n");
	printf("  -fullscreen      set fullscreen mode\n");
	printf("  -hardware        use direct video memory (if possible)\n");
	printf("  -doublebuffer    enable double buffering mode (if possible)\n");
	printf("  -fps             show frames/sec\n");
	printf("  -size WxH        set the size of capturing image\n");
	printf("  -geometry WxH    set the size of screen\n");
	printf("  -scale NUMBER    scaling the screen\n");
	printf("  -autoplay NUMBER changes effects automatically every NUMBER frames\n");
#ifdef USE_VLOOPBACK
	printf("  -vloopback FILE  use device FILE for output of vloopback device\n");
#endif
}

static int parse_geometry(const char *str, int *w, int *h)
{
	int ret;

	ret = sscanf(str, "%dx%d", w, h);
	if(ret != 2) {
		fprintf(stderr, "size specification is wrong.(e.g.\"320x240\")\n");
		return -1;
	}
	if(*w <=0 || *h <= 0) {
		fprintf(stderr, "width and height must be greater than 0.\n");
		return -1;
	}

	return 0;
}

static void drawErrorPattern()
{
/* This is stil quick hack. XBM drawing function can be modularized. */
	int x, y, b;
	int mx, my;
	int sw, sh;
	int bpl, bytes, restbits;
	RGB32 *dest, *q;
	unsigned char *p, v;
	int flag = 0;

	bytes = syserr_xbm_width / 8;
	bpl = (syserr_xbm_width + 7) / 8;
	restbits = syserr_xbm_width - bytes * 8;
	sw = screen_width;
	sh = screen_height;

	if(screen_mustlock()) {
		if(screen_lock() < 0) {
			return;
		}
	}

	if(screen_width >= syserr_xbm_width && screen_height >= syserr_xbm_height) {
		dest = (RGB32 *)screen_getaddress();
	} else {
		if(syserr_xbm_width / sw > syserr_xbm_height / sh) {
			sh = sh * syserr_xbm_width / sw;
			sw = syserr_xbm_width;
		} else {
			sw = sw * syserr_xbm_height / sh;
			sh = syserr_xbm_height;
		}
		dest = (RGB32 *)malloc(sw * sh * sizeof(RGB32));
		flag = 1;
	}
	for(x=0; x<sw*sh; x++) {
		dest[x] = 0xff0000;
	}
	mx = (sw - syserr_xbm_width) / 2;
	my = (sh - syserr_xbm_height) / 2;
	q = dest + my*sw + mx;
	for(y=0; y<syserr_xbm_height; y++) {
		p = syserr_xbm_bits + y * bpl;
		for(x=0; x<bytes; x++) {
			v = *p++;
			for(b=0; b<8; b++) {
				if(v&1) *q = 0;
				q++;
				v = v>>1;
			}
		}
		if(restbits) {
			v = *p++;
			for(b=0; b<restbits; b++) {
				if(v & 1) *q = 0;
				q++;
				v = v>>1;
			}
		}
		q += sw - syserr_xbm_width;
	}
	if(flag) {
		image_stretch(dest, sw, sh, (RGB32 *)screen_getaddress(), screen_width, screen_height);
		free(dest);
	}
	if(screen_mustlock()) {
		screen_unlock();
	}
	screen_update();
}

static int registEffects()
{
	int i, n;
	effect *entry;

	n = sizeof(effects_register_list)/sizeof(effectRegistFunc *);
	effectsList = (effect **)malloc(n*sizeof(effect *));
	effectMax = 0;
	for(i=0;i<n;i++) {
		entry = (*effects_register_list[i])();
		if(entry) {
			printf("%.40s OK.\n",entry->name);
			effectsList[effectMax] = entry;
			effectMax++;
		}
	}
	printf("%d effects are available.\n",effectMax);
	return effectMax;
}

static int changeEffect(int num)
{
/* return value:
 *  0: fatal error
 *  1: success
 *  2: not available
 */
	if(currentEffect)
		currentEffect->stop();
	currentEffectNum = num;
	if(currentEffectNum < 0)
		currentEffectNum += effectMax;
	if(currentEffectNum >= effectMax)
		currentEffectNum -= effectMax;
	currentEffect = effectsList[currentEffectNum];
	screen_setcaption(currentEffect->name);
	if(currentEffect->start() < 0)
		return 2;

	return 1;
}

static int startTV()
{
	int ret;
	int flag;
	int frames=0;
	struct timeval tv;
	long lastusec=0, usec=0;
	SDL_Event event;
	char buf[256];
	static int alt_pressed = 0;

	currentEffectNum = 0;
	currentEffect = NULL;
	flag = changeEffect(currentEffectNum);
	if(autoplay) {
		autoplay_counter = autoplay;
	}

	if(fps) {
		gettimeofday(&tv, NULL);
		lastusec = tv.tv_sec*1000000+tv.tv_usec;
		frames = 0;
	}
	while(flag) {
		if(flag == 1) {
			ret = currentEffect->draw();
			if(ret < 0) {
				flag = 2;
			} else if(ret == 0) {
#ifdef USE_VLOOPBACK
				if(vloopback) {
					vloopback_push();
				}
#endif
				screen_update();
			}
		}
		if (flag == 2) {
			drawErrorPattern();
			flag = 3;
		}
		if(fps) {
			frames++;
			if(frames == 100) {
				gettimeofday(&tv, NULL);
				usec = tv.tv_sec*1000000+tv.tv_usec;
				sprintf(buf, "%s (%2.2f fps)", currentEffect->name, (float)100000000/(usec - lastusec));
				screen_setcaption(buf);
				lastusec = usec;
				frames = 0;
			}
		}
		if(autoplay) {
			autoplay_counter--;
			if(autoplay_counter == 0) {
				autoplay_counter = autoplay;
				flag = changeEffect(currentEffectNum+1);
			}
		}
		while(SDL_PollEvent(&event)) {
			if(event.type == SDL_KEYDOWN) {
				switch(event.key.keysym.sym) {
				case SDLK_UP:
					flag = changeEffect(currentEffectNum-1);
					if(autoplay)
						autoplay_counter = autoplay;
					break;
				case SDLK_DOWN:
					flag = changeEffect(currentEffectNum+1);
					if(autoplay)
						autoplay_counter = autoplay;
					break;
				case SDLK_LEFT:
					video_setfreq(-1);
					break;
				case SDLK_RIGHT:
					video_setfreq(1);
					break;
				case SDLK_TAB:
					horizontal_flip = horizontal_flip ^ 1;
					break;
				case SDLK_LALT:
				case SDLK_RALT:
					alt_pressed = 1;
					break;
				case SDLK_RETURN:
					if(alt_pressed) {
						screen_fullscreen();
					}
					break;
				case SDLK_ESCAPE:
					flag = 0;
					break;
				default:
					break;
				}
			}
			if(event.type == SDL_KEYUP) {
				switch(event.key.keysym.sym) {
				case SDLK_LALT:
				case SDLK_RALT:
					alt_pressed = 0;
					break;
				default:
					break;
				}
			}
			if(event.type == SDL_QUIT) flag=0;
			if(currentEffect->event) {
				currentEffect->event(&event);
			}
		}
	}
	currentEffect->stop();
	return 0;
}

int main(int argc, char **argv)
{
	int i;
	char *option;
	int channel = 0;
	int norm = DEFAULT_VIDEO_NORM;
	int freqtab = 0;
	char *devfile = NULL;
#ifdef USE_VLOOPBACK
	char *vloopbackfile = NULL;
#endif
	int vw, vh; /* video width,height */
	int sw, sh, ss; /* screen width,height,scale */

	vw = vh = sw = sh = 0;
	ss = 1;

	for(i=1;i<argc;i++) {
		option = argv[i];
		if(*option == '-')
			option++;
		if (strncmp(option, "channel", 2) == 0) {
			i++;
			if(i<argc) {
				channel = atoi(argv[i]);
			} else {
				fprintf(stderr, "missing channel number.\n");
				exit(1);
			}
		} else if(strcmp(option, "norm") == 0) {
			i++;
			if(i<argc) {
				if((norm = videox_getnorm(argv[i])) < 0) {
					fprintf(stderr, "norm %s is not supported.\n", argv[i]);
					exit(1);
				}
			} else {
				fprintf(stderr, "missing norm.\n");
				exit(1);
			}
		} else if(strcmp(option, "freqtab") == 0) {
			i++;
			if(i<argc) {
				if((freqtab = videox_getfreq(argv[i])) < 0) {
					fprintf(stderr, "frequency table %s is not supported.\n", argv[i]);
					exit(1);
				}
			} else {
				fprintf(stderr, "missing frequency table.\n");
				exit(1);
			}
		} else if(strncmp(option, "device", 6) == 0) {
			i++;
			if(i<argc) {
				devfile = argv[i];
			} else {
				fprintf(stderr, "missing device file.\n");
				exit(1);
			}
#ifdef USE_VLOOPBACK
		} else if(strncmp(option, "vloopback", 5) == 0) {
			i++;
			if(i<argc) {
				vloopbackfile = argv[i];
				vloopback = 1;
			} else {
				fprintf(stderr, "missing device file.\n");
				exit(1);
			}
#endif
		} else if(strcmp(option, "hardware") == 0) {
			hwsurface = 1;
		} else if(strncmp(option, "fullscreen", 4) == 0) {
			fullscreen = 1;
		} else if(strncmp(option, "doublebuffer", 9) == 0) {
			doublebuf = 1;
		} else if(strcmp(option, "fps") == 0) {
			fps = 1;
		} else if(strcmp(option, "autoplay") == 0) {
			i++;
			if(i<argc) {
				autoplay = atoi(argv[i]);
				if(autoplay <= 0) {
					fprintf(stderr, "interval frames must be greater than 0.\n");
					exit(1);
				}
			} else {
				fprintf(stderr, "missing a number of interval frames.\n");
				exit(1);
			}
		} else if(strcmp(option, "size") == 0) {
			i++;
			if(i<argc) {
				if(parse_geometry(argv[i], &vw, &vh)) {
					exit(1);
				}
			} else {
				fprintf(stderr, "missing capturing size specification.\n");
				exit(1);
			}
		} else if(strcmp(option, "geometry") == 0) {
			i++;
			if(i<argc) {
				if(parse_geometry(argv[i], &sw, &sh)) {
					exit(1);
				}
			} else {
				fprintf(stderr, "missing screen size specification.\n");
				exit(1);
			}
		} else if(strcmp(option, "scale") == 0) {
			i++;
			if(i<argc) {
				ss = atoi(argv[i]);
				if(ss <= 0) {
					fprintf(stderr, "scale value must be greater than 0.\n");
					exit(1);
				}
			} else {
				fprintf(stderr, "missing a scale value.\n");
				exit(1);
			}
		} else if(strncmp(option, "help", 1) == 0) {
			usage();
			exit(0);
		} else {
			fprintf(stderr, "invalid option %s\n",argv[i]);
			usage();
			exit(1);
		}
	}

	srand(time(NULL));
	fastsrand(time(NULL));

	if(sw > 0 && vw == 0) {
	/* screen size is specified while capturing is not.*/
		vw = sw / ss;
		vh = sh / ss;
	}
//	if(debug) {
//		v4ldebug(1);
//	}
	if(video_init(devfile, channel, norm, freqtab, vw, vh)) {
		fprintf(stderr, "Video initialization failed.\n");
		exit(1);
	}
	if(screen_init(sw, sh, ss)) {
		fprintf(stderr, "Screen initialization failed.\n");
		exit(1);
	}
	if(debug) {
		fprintf(stderr, "capturing size: %dx%d\n",video_width, video_height);
		fprintf(stderr, "screen size: %dx%d\n",screen_width, screen_height);
		fprintf(stderr, "scale = %d; stretch = %d\n",screen_scale, stretch);
	}
#ifdef USE_VLOOPBACK
	if(vloopback) {
		if(vloopback_init(vloopbackfile)) {
			fprintf(stderr, "Vloopback initialization failed\n");
			exit(1);
		}
	}
#endif
	if(sharedbuffer_init()){
		fprintf(stderr, "Memory allocation failed.\n");
		exit(1);
	}
	if(utils_init()) {
		fprintf(stderr, "Utility function initialization failed.\n");
		exit(1);
	}

	if(registEffects() == 0) {
		fprintf(stderr, "No available effect.\n");
		exit(1);
	}

//	showTitle();
	startTV();

	return 0;
}
