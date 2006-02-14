/*
 * EffecTV - Realtime Video Effector
 * Copyright (C) 2001-2006 FUKUCHI Kentaro
 *
 * main.c: start up module
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
#include "palette.h"
#ifdef USE_VLOOPBACK
#include "vloopback.h"
#endif

/* The number of loops for retry of video grabbing. In the default setting,
 * EffecTV will try to restart grabbing process every 1 second. */
#define RETRY_LOOP 30

int debug = 0; /* 0 = off, 1 = less debug messages, 2 = more debug messages. */
int autoplay = 0;
int autoplay_counter;

static effectRegisterFunc *effects_register_list[] =
{
	dumbRegister,
	quarkRegister,
	fireRegister,
	burnRegister,
	blurzoomRegister,
	streakRegister,
	baltanRegister,
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
	dizzyRegister,
	DeinterlaceRegister,
	nervousRegister,
	rndmRegister,
	revRegister,
	rdsRegister,
	lensRegister,
	diffRegister,
	scrollRegister,
	warholRegister,
	matrixRegister,
	pupRegister,
	chameleonRegister,
	opRegister,
	nervousHalfRegister,
	slofastRegister,
	displayWallRegister,
	bluescreenRegister,
	colstreakRegister,
	timeDistortionRegister,
	edgeBlurRegister,
};

static effect **effectsList;
static int effectMax;
static int currentEffectNum;
static effect *currentEffect;
static int fps = 0;

static void fpsInit(void);
static void fpsCount(void);
static void drawErrorPattern(void);
static int parseGeometry(const char *str, int *w, int *h);

static void usage(void)
{
	printf("EffecTV - Realtime Video Effector\n");
	printf("Version: %s\n", VERSION_STRING);
	printf("Usage: effectv [options...]\n");
	printf("Options:\n");
	printf("  -device FILE     use device FILE for video4linux\n");
	printf("  -channel NUMBER  channel number of video source\n");
	printf("  -norm {ntsc,pal,secam,pal-nc,pal-m,pal-n,ntsc-jp}\n"
           "                   set video norm\n");
	printf("  -freqtab {us-bcast,us-cable,us-cable-hrc,japan-bcast,japan-cable,europe-west,\n"
           "            europe-east,italy,newzealand,australia,ireland,france,china-bcast,\n"
           "            southafrica,argentina,canada-cable,australia-optus}\n"
           "                   set frequency table\n");
	printf("  -fullscreen      set fullscreen mode\n");
	printf("  -hardware        use direct video memory (if possible)\n");
	printf("  -doublebuffer    enable double buffering mode (if possible)\n");
	printf("  -fps             show frames/sec\n");
	printf("  -size WxH        set the size of capturing image\n");
	printf("  -geometry WxH    set the size of screen\n");
	printf("  -scale NUMBER    scaling the screen\n");
	printf("  -autoplay NUMBER changes effects automatically every NUMBER frames\n");
	printf("  -palette {rgb24,rgb565,rgb555,yuv422,yuv422p,yuv420p,yuv411p,yuv410p,grey}\n"
           "                   set the palette of capturing device. It is detected\n"
           "                   automatically by default.\n");
#ifdef USE_VLOOPBACK
	printf("  -vloopback FILE  use device FILE for output of vloopback device\n");
#endif
}

static void keyUsage(void)
{
	printf( "---------------\n"
			"Key description\n"
			"Up/Down     change effect.\n"
			"Right/Left  change TV channel.\n"
			"Space       capture a background image(for FireTV, BurningTV, etc.).\n"
			"            change mode(for SpiralTV, TransFormTV)\n"
			"ALT+Enter   fullscreen mode(toggle).\n"
			"TAB         Horizontal flipping(toggle).\n"
			"Escape      Quit\n"
			"ALT+0-9     change video input channel.\n"
			"F1/F2       increase/decrease brightness of video input.\n"
			"F3/F4       increase/decrease hue.\n"
			"F5/F6       increase/decrease color balance.\n"
			"F7/F8       increase/decrease contrast.\n"
			"F12         show this usage.\n");
}

static int registerEffects(void)
{
	int i, n;
	effect *entry;

	n = sizeof(effects_register_list)/sizeof(effectRegisterFunc *);
	effectsList = (effect **)malloc(n*sizeof(effect *));
	effectMax = 0;
	for(i=0;i<n;i++) {
		entry = (*effects_register_list[i])();
		if(entry) {
			printf("%s OK.\n",entry->name);
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
	while(currentEffectNum < 0)
		currentEffectNum += effectMax;
	while(currentEffectNum >= effectMax)
		currentEffectNum -= effectMax;
	currentEffect = effectsList[currentEffectNum];
	screen_setcaption(currentEffect->name);
	screen_clear(0);
	if(stretch) {
		image_stretching_buffer_clear(0);
	}
	if(currentEffect->start() < 0)
		return 2;

	return 1;
}

static int searchEffect(const char *name)
{
	int i, num, len1, len2;
	char *p;

	len1 = strlen(name);
	if(len1 > 2) {
		if(strncasecmp(&name[len1-2], "TV", 2) == 0) {
			len1 -= 2;
		}
	}

	num = -1;
	for(i=0; i<effectMax; i++) {
		p = effectsList[i]->name;
		len2 = strlen(p);
		if(len2 > 2) {
			if(strncasecmp(&p[len2-2], "TV", 2) == 0) {
				len2 -= 2;
			}
		}
		if(len1 != len2)
			continue;
		if(strncasecmp(name, effectsList[i]->name, len1) == 0) {
			num = i;
			break;
		}
	}
	if(num == -1) {
		fprintf(stderr, "Couldn't find \"%s\". Starts DumbTV.\n", name);
		num = 0;
	}

	return num;
}

static int startTV(const char *startEffect)
{
	int ret;
	int flag;
	SDL_Event event;
	int eventDone;
	RGB32 *src, *dest;
	int retry = 0;

	ret = video_grabstart();
	if(ret != 0) {
		fprintf(stderr, "Failed to grab image.\n");
		exit(1);
	}

	if(startEffect != NULL) {
		currentEffectNum = searchEffect(startEffect);
	} else {
		currentEffectNum = 0;
	}
	currentEffect = NULL;
	flag = changeEffect(currentEffectNum);
	if(autoplay) {
		autoplay_counter = autoplay;
	}

	if(fps) {
		fpsInit();
	}

	/* main loop */
	while(flag) {
		if(flag == 1) {
			ret = video_syncframe();
			if(ret != 0) {
				flag = 2;
			} else {
				if(screen_lock() == 0) {
					src = (RGB32 *)video_getaddress();
					if(stretch) {
						dest = stretching_buffer;
					} else {
						dest = (RGB32 *)screen_getaddress();
					}

					ret = currentEffect->draw(src, dest);

					if(ret == 0) {
						if(stretch) {
							image_stretch_to_screen();
						}
					}

					screen_unlock();
				}

				if(ret < 0) {
					flag = 2;
				} else {
#ifdef USE_VLOOPBACK
					if(vloopback) {
						vloopback_push();
					}
#endif
					screen_update();
				}
				ret = video_grabframe();
				if(ret != 0) {
					flag = 2;
				}
			}
		}
		if (flag == 2) {
			drawErrorPattern();
			flag = 3;
			retry = RETRY_LOOP;
		}
		if (flag == 3) {
			usleep(300);
			if(retry > 0) {
				retry--;
				if(retry == 0) {
					if(video_retry() == 0) {
						video_grabstart();
						flag = changeEffect(currentEffectNum);
					} else {
						retry = RETRY_LOOP;
					}
				}
			}
		}

		if(fps) {
			fpsCount();
		}

		if(flag == 1 && autoplay) {
			autoplay_counter--;
			if(autoplay_counter == 0) {
				autoplay_counter = autoplay;
				flag = changeEffect(currentEffectNum+1);
			}
		}

		while(SDL_PollEvent(&event)) {
			eventDone = 0;

			if(event.type == SDL_KEYDOWN) {
				eventDone = 1;
				switch(event.key.keysym.sym) {
				case SDLK_UP:
					if(flag == 1) {
						flag = changeEffect(currentEffectNum-1);
						if(autoplay)
							autoplay_counter = autoplay;
					}
					break;
				case SDLK_DOWN:
					if(flag == 1) {
						flag = changeEffect(currentEffectNum+1);
						if(autoplay)
							autoplay_counter = autoplay;
					}
					break;
				case SDLK_LEFT:
					video_setfreq(-1);
					break;
				case SDLK_RIGHT:
					video_setfreq(1);
					break;
				case SDLK_TAB:
					video_horizontalFlip ^= 1;
					break;
				case SDLK_F1:
					video_change_brightness(+4096);
					break;
				case SDLK_F2:
					video_change_brightness(-4096);
					break;
				case SDLK_F3:
					video_change_hue(+4096);
					break;
				case SDLK_F4:
					video_change_hue(-4096);
					break;
				case SDLK_F5:
					video_change_color(+4096);
					break;
				case SDLK_F6:
					video_change_color(-4096);
					break;
				case SDLK_F7:
					video_change_contrast(+4096);
					break;
				case SDLK_F8:
					video_change_contrast(-4096);
					break;
				case SDLK_F12:
					keyUsage();
					flag = 2;
					break;
				case SDLK_0:
				case SDLK_1:
				case SDLK_2:
				case SDLK_3:
				case SDLK_4:
				case SDLK_5:
				case SDLK_6:
				case SDLK_7:
				case SDLK_8:
				case SDLK_9:
					if(event.key.keysym.mod & KMOD_ALT) {
						video_change_channel(event.key.keysym.sym - SDLK_0);
					} else {
						eventDone = 0;
					}
					break;
				case SDLK_KP0:
				case SDLK_KP1:
				case SDLK_KP2:
				case SDLK_KP3:
				case SDLK_KP4:
				case SDLK_KP5:
				case SDLK_KP6:
				case SDLK_KP7:
				case SDLK_KP8:
				case SDLK_KP9:
					if(event.key.keysym.mod & KMOD_ALT) {
						video_change_channel(event.key.keysym.sym - SDLK_KP0);
					} else {
						eventDone = 0;
					}
					break;
				case SDLK_RETURN:
					if(event.key.keysym.mod & KMOD_ALT) {
						screen_fullscreen();
					}
					break;
				case SDLK_ESCAPE:
					flag = 0;
					break;
				default:
					eventDone = 0;
					break;
				}
			}
			if(event.type == SDL_KEYUP) { /* Just as a place holder */
				switch(event.key.keysym.sym) {
				default:
					break;
				}
			}
			if(event.type == SDL_QUIT) {
				flag=0;
				eventDone = 1;
			}
			if(eventDone == 0 && currentEffect->event) {
				currentEffect->event(&event);
			}
		}
	}
	currentEffect->stop();

	video_grabstop();

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
	int palette = 0;
	char *startEffect = NULL;

	vw = vh = sw = sh = 0;
	ss = 1;

	for(i=1;i<argc;i++) {
		option = argv[i];
		if(*option != '-') {
			break;
		} else {
			option++;
		}
		if (strncmp(option, "channel", 2) == 0) {
			i++;
			if(i<argc) {
				channel = atoi(argv[i]);
			} else {
				fprintf(stderr, "-channel: missing channel number.\n");
				exit(1);
			}
		} else if(strcmp(option, "norm") == 0) {
			i++;
			if(i<argc) {
				if((norm = videox_getnorm(argv[i])) < 0) {
					fprintf(stderr, "-norm: norm %s is not supported.\n", argv[i]);
					exit(1);
				}
			} else {
				fprintf(stderr, "-norm: missing norm.\n");
				exit(1);
			}
		} else if(strcmp(option, "freqtab") == 0) {
			i++;
			if(i<argc) {
				if((freqtab = videox_getfreq(argv[i])) < 0) {
					fprintf(stderr, "-freqtab: frequency table %s is not supported.\n", argv[i]);
					exit(1);
				}
			} else {
				fprintf(stderr, "-freqtab: missing frequency table.\n");
				exit(1);
			}
		} else if(strncmp(option, "device", 6) == 0) {
			i++;
			if(i<argc) {
				devfile = argv[i];
			} else {
				fprintf(stderr, "-device: missing device file.\n");
				exit(1);
			}
#ifdef USE_VLOOPBACK
		} else if(strncmp(option, "vloopback", 5) == 0) {
			i++;
			if(i<argc) {
				vloopbackfile = argv[i];
				vloopback = 1;
			} else {
				fprintf(stderr, "-vloopback: missing device file.\n");
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
					fprintf(stderr, "-autoplay: interval frames must be greater than 0.\n");
					exit(1);
				}
			} else {
				fprintf(stderr, "-autoplay: missing a number of interval frames.\n");
				exit(1);
			}
		} else if(strcmp(option, "size") == 0) {
			i++;
			if(i<argc) {
				if(parseGeometry(argv[i], &vw, &vh)) {
					exit(1);
				}
			} else {
				fprintf(stderr, "-size: missing capturing size specification.\n");
				exit(1);
			}
		} else if(strcmp(option, "geometry") == 0) {
			i++;
			if(i<argc) {
				if(parseGeometry(argv[i], &sw, &sh)) {
					exit(1);
				}
			} else {
				fprintf(stderr, "-geometry: missing screen size specification.\n");
				exit(1);
			}
		} else if(strcmp(option, "scale") == 0) {
			i++;
			if(i<argc) {
				ss = atoi(argv[i]);
				if(ss <= 0) {
					fprintf(stderr, "-scale: scale value must be greater than 0.\n");
					exit(1);
				}
			} else {
				fprintf(stderr, "-scale: missing a scale value.\n");
				exit(1);
			}
		} else if(strncmp(option, "palette", 3) == 0) {
			i++;
			if(i<argc) {
				if((palette = palettex_getpalette(argv[i])) < 0) {
					fprintf(stderr, "-palette: palette %s is not supported.\n",argv[i]);
					exit(1);
				}
			} else {
				fprintf(stderr, "-palette: missing palette name.\n");
				exit(1);
			}
		} else if(strncmp(option, "debug", 5) == 0) {
			debug = 1;
		} else if(strncmp(option, "help", 1) == 0) {
			usage();
			exit(0);
		} else {
			fprintf(stderr, "invalid option %s\n",argv[i]);
			usage();
			exit(1);
		}
	}
	if(i < argc) {
		startEffect = argv[i];
	}

	srand(time(NULL));
	fastsrand(time(NULL));

	if(sw > 0 && vw == 0) {
	/* screen size is specified while capturing is not.*/
		vw = sw / ss;
		vh = sh / ss;
	}

	if(debug > 1) {
		v4ldebug(1);
	}

	if(palette_init()) {
		fprintf(stderr, "Palette initialization failed.\n");
		exit(1);
	}
	if(video_init(devfile, channel, norm, freqtab, vw, vh, palette)) {
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
			fprintf(stderr, "Vloopback initialization failed.\n");
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

	if(registerEffects() == 0) {
		fprintf(stderr, "No available effect.\n");
		exit(1);
	}

//	showTitle();
	keyUsage();
	startTV(startEffect);

#ifdef MEM_DEBUG
	if(debug) {
		palette_end();
		utils_end();
		sharedbuffer_end();
	}
#endif

	video_quit();

	return 0;
}

/* Parse string contains geometry size */

static int parseGeometry(const char *str, int *w, int *h)
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

/* FPS counter */

static int fps_frames;
static long fps_lastusec;

static void fpsInit(void)
{
	fps_frames = 0;
	fps_lastusec = 0;
}

static void fpsCount(void)
{
	long usec;
	char buf[256];
	struct timeval tv;

	fps_frames++;
	if(fps_frames == 100) {
		gettimeofday(&tv, NULL);
		usec = tv.tv_sec * 1000000 + tv.tv_usec;
		if(usec - fps_lastusec < 0) {
			snprintf(buf, 256, "N/A");
		} else {
			snprintf(buf, 256, "%s (%2.2f fps)", currentEffect->name, (float)100000000/(usec - fps_lastusec));
		}

		screen_setcaption(buf);

		fps_lastusec = usec;
		fps_frames = 0;
	}
}

static void drawErrorPattern(void)
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

	if(screen_lock() < 0) {
		return;
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
	screen_unlock();
	screen_update();
}
