/*
 * EffecTV - Realtime Digital Video Effector
 * Copyright (C) 2001 FUKUCHI Kentarou
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
#include "effects/utils.h"

int debug = 0;
int scale = 1;

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
	dotRegister
};

static effect **effectsList;
static int effectMax;
static int currentEffectNum;
static effect *currentEffect;
static int fps = 0;

static void usage()
{
	printf("EffecTV - Realtime Digital Video Effector\n");
	printf("Version: %s\n", VERSION_STRING);
	printf("Usage: effectv [options...]\n");
	printf("Options:\n");
	printf("\tdevice FILE\tuse Video4Linux device FILE\n");
	printf("\tchannel NUMBER\tchannel number of video source\n");
	printf("\tfullscreen\tenable fullscreen mode\n");
	printf("\tdouble\t\tdoubling screen size\n");
	printf("\thardware\tuse direct video memory\n");
	printf("\tfps\t\tshow frames/sec\n");
}

static void drawErrorPattern()
{
	SDL_Rect rect;

	rect.w = SCREEN_WIDTH;
	rect.h = SCREEN_HEIGHT;
	rect.x = 0;
	rect.y = 0;
	SDL_FillRect(screen, &rect, 0);
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
	SDL_WM_SetCaption(currentEffect->name, NULL);
	if(currentEffect->start() < 0)
		return 2;

	return 1;
}

static int startTV()
{
	int flag;
	int frames=0;
	struct timeval tv;
	long lastusec=0, usec=0;
	SDL_Event event;
	char buf[256];

	currentEffectNum = 0;
	currentEffect = NULL;
	flag = changeEffect(currentEffectNum);

	if(fps) {
		gettimeofday(&tv, NULL);
		lastusec = tv.tv_sec*1000000+tv.tv_usec;
		frames = 0;
	}
	while(flag) {
		if(flag == 1) {
			flag = (currentEffect->draw())?2:1;
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
				SDL_WM_SetCaption(buf, NULL);
				lastusec = usec;
				frames = 0;
			}
		}
		while(SDL_PollEvent(&event)) {
			if(event.type == SDL_KEYDOWN) {
				switch(event.key.keysym.sym) {
				case SDLK_UP:
					flag = changeEffect(currentEffectNum-1);
					break;
				case SDLK_DOWN:
					flag = changeEffect(currentEffectNum+1);
					break;
				case SDLK_ESCAPE:
					flag = 0;
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
	int screen_flags = 0;
	char *devfile = NULL;

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
		}
		else if(strncmp(option, "device", 6) == 0) {
			i++;
			if(i<argc) {
				devfile = argv[i];
			} else {
				fprintf(stderr, "missing device file.\n");
			}
		} else if(strncmp(option, "hardware", 8) == 0) {
			screen_flags |= SDL_HWSURFACE;
		} else if(strncmp(option, "fullscreen", 4) == 0) {
			screen_flags |= SDL_FULLSCREEN;
		} else if(strncmp(option, "doublebuffer", 9) == 0) {
			screen_flags |= SDL_DOUBLEBUF;
		} else if(strncmp(option, "double", 6) == 0) {
			scale = 2;
		} else if(strncmp(option, "fps", 3) == 0) {
			fps = 1;
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

	if(sharedbuffer_init(scale)){
		fprintf(stderr, "Memory allocation failed.\n");
		exit(1);
	}
	if(video_init(devfile, channel)) {
		fprintf(stderr, "Video initialization failed.\n");
		exit(1);
	}
	if(screen_init(screen_flags, scale)) {
		fprintf(stderr, "Screen initialization failed.\n");
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
