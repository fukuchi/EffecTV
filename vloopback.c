/*
 * EffecTV - Realtime Digital Video Effector
 * Copyright (C) 2001-2006 FUKUCHI Kentaro
 *
 * vloopback.c: vloopback device manager
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <signal.h>
#include <linux/videodev.h>
#include <v4lutils.h>
#include <pthread.h>

#include "EffecTV.h"
#include "palette.h"
#include "vloopback.h"

#define MAXIOCTL 1024

int vloopback = 0;

static int outputfd;
static unsigned char *gbuf;

static char *output_devname;
static char ioctlbuf[MAXIOCTL];
static int pixel_format;
static int pixel_depth;
static int vloopback_width;
static int vloopback_height;
static int max_framesize;
static int max_gbufsize;
static int gbufqueue[2];
static int gbufstat[2];
#define GBUFFER_UNUSED   0
#define GBUFFER_GRABBING 1
#define GBUFFER_DONE     2
#define GBUFFER_ERROR    3
static pthread_cond_t gbufcond;
static pthread_mutex_t gbufmutex;
static pthread_t signal_loop_thread;
static int signal_loop_initialized;
static int signal_loop_thread_flag;
static palette_converter_fromRGB32 *converter;

#define MAX_WIDTH screen_width
#define MAX_HEIGHT screen_height
#define MIN_WIDTH 32
#define MIN_HEIGHT 32

#if VLOOPBACK_VERSION > 83
#define VIDIOCSINVALID _IO('v',BASE_VIDIOCPRIVATE+1)
#endif

static void gbuf_clear(void)
{
	gbufstat[0] = GBUFFER_UNUSED;
	gbufstat[1] = GBUFFER_UNUSED;
	gbufqueue[0] = -1;
	gbufqueue[1] = -1;
}

static inline int gbuf_lock(void)
{
	return pthread_mutex_lock(&gbufmutex);
}

static inline int gbuf_unlock(void)
{
	return pthread_mutex_unlock(&gbufmutex);
}

static unsigned char *vloopback_mmap(int dev, int memsize)
{
	unsigned char *map;
	
	map = mmap(0, memsize, PROT_READ|PROT_WRITE, MAP_SHARED, dev, 0);
	if(map < 0) {
		perror("vloopback_mmap:mmap");
		return NULL;
	}
	return map;	
}

#if 0
/* never used */
static int vloopback_munmap(unsigned char *map, int memsize)
{
	if(munmap(map, memsize) < 0) {
		perror("vloopback_munmap:munmap");
		return -1;
	}
	return 0;
}
#endif

#if VLOOPBACK_VERSION > 83
static int v4l_ioctlhandler(unsigned long int cmd, void *arg)
#else
static int v4l_ioctlhandler(unsigned int cmd, void *arg)
#endif
{
	switch(cmd) {
#if VLOOPBACK_VERSION > 83
		case VIDIOCGCAP:
#else
		case VIDIOCGCAP & 0xff:
#endif
		{
			struct video_capability *vidcap = arg;

			snprintf(vidcap->name, 32, "EffecTV vloopback output");
			vidcap->type = VID_TYPE_CAPTURE;
			vidcap->channels = 1;
			vidcap->audios = 0;
			vidcap->maxwidth = MAX_WIDTH;
			vidcap->maxheight = MAX_HEIGHT;
			vidcap->minwidth = MIN_WIDTH;
			vidcap->minheight = MIN_HEIGHT;
			return 0;
		}

#if VLOOPBACK_VERSION > 83
		case VIDIOCGCHAN:
#else
		case VIDIOCGCHAN & 0xff:
#endif
		{
			struct video_channel *vidchan = arg;

			if(vidchan->channel != 0)
				return EINVAL;
			vidchan->flags = 0;
			vidchan->tuners = 0;
			vidchan->type = VIDEO_TYPE_CAMERA;
			strncpy(vidchan->name, "EffecTV", 32);
			return 0;
		}

#if VLOOPBACK_VERSION > 83
		case VIDIOCSCHAN:
#else
		case VIDIOCSCHAN & 0xff:
#endif
		{
			int *v = arg;

			if(v[0] != 0)
				return EINVAL;
			return 0;
		}

#if VLOOPBACK_VERSION > 83
		case VIDIOCGPICT:
#else
		case VIDIOCGPICT & 0xff:
#endif
		{
			struct video_picture *vidpic = arg;

			vidpic->colour = 0xffff;
			vidpic->hue = 0xffff;
			vidpic->brightness = 0xffff;
			vidpic->contrast = 0xffff;
			vidpic->whiteness = 0xffff;
			vidpic->depth = pixel_depth;
			vidpic->palette = pixel_format;
			return 0;
		}

#if VLOOPBACK_VERSION > 83
		case VIDIOCSPICT:
#else
		case VIDIOCSPICT & 0xff:
#endif
		{
			struct video_picture *vidpic = arg;

			if(vidpic->palette != pixel_format) {
				converter = palette_get_supported_converter_fromRGB32(vidpic->palette);
				if(converter == NULL) {
					fprintf(stderr, "vloopback: unsupported pixel format(%d) is requested.\n",vidpic->palette);
					return EINVAL;
				}
				pixel_format = vidpic->palette;
			}
			return 0;
		}

#if VLOOPBACK_VERSION > 83
		case VIDIOCGWIN:
#else
		case VIDIOCGWIN & 0xff:
#endif
		{
			struct video_window *vidwin = arg;

			vidwin->x = 0;
			vidwin->y = 0;
			vidwin->width = vloopback_width;
			vidwin->height = vloopback_height;
			vidwin->chromakey = 0;
			vidwin->flags = 0;
			vidwin->clipcount = 0;
		}

#if VLOOPBACK_VERSION > 83
		case VIDIOCSWIN:
#else
		case VIDIOCSWIN & 0xff:
#endif
		{
			struct video_window *vidwin = arg;

			if(vidwin->width > MAX_WIDTH || vidwin->height > MAX_HEIGHT)
				return EINVAL;
			if(vidwin->width < MIN_WIDTH || vidwin->height < MIN_HEIGHT)
				return EINVAL;
			if(vidwin->flags)
				return EINVAL;
			vloopback_width = vidwin->width;
			vloopback_height = vidwin->height;
			return 0;
		}

#if VLOOPBACK_VERSION > 83
		case VIDIOCSYNC:
#else
		case VIDIOCSYNC & 0xff:
#endif
		{
			int frame = *(int *)arg;
			int ret = 0;

			if(frame < 0 || frame > 1)
				return EINVAL;
			gbuf_lock();
			switch(gbufstat[frame]) {
			case GBUFFER_UNUSED:
				ret = 0;
				break;
			case GBUFFER_GRABBING:
				while(gbufstat[frame] == GBUFFER_GRABBING) {
					pthread_cond_wait(&gbufcond, &gbufmutex);
				}
				if(gbufstat[frame] == GBUFFER_DONE) {
					gbufstat[frame] = GBUFFER_UNUSED;
					ret = 0;
				} else {
					gbufstat[frame] = GBUFFER_UNUSED;
					ret = EIO;
				}
				break;
			case GBUFFER_DONE:
				gbufstat[frame] = GBUFFER_UNUSED;
				ret = 0;
				break;
			case GBUFFER_ERROR:
			default:
				gbufstat[frame] = GBUFFER_UNUSED;
				ret = EIO;
				break;
			}
			gbuf_unlock();
			return ret;
		}

#if VLOOPBACK_VERSION > 83
		case VIDIOCMCAPTURE:
#else
		case VIDIOCMCAPTURE & 0xff:
#endif
		{
			struct video_mmap *vidmmap = arg;

			if(vidmmap->width > MAX_WIDTH || vidmmap->height > MAX_HEIGHT) {
				fprintf(stderr, "vloopback: requested capture size is too big(%dx%d).\n",vidmmap->width, vidmmap->height);
				return EINVAL;
			}
			if(vidmmap->width < MIN_WIDTH || vidmmap->height < MIN_HEIGHT) {
				fprintf(stderr, "vloopback: requested capture size is to small(%dx%d).\n",vidmmap->width, vidmmap->height);
				return EINVAL;
			}
			if(vidmmap->format != pixel_format) {
				converter = palette_get_supported_converter_fromRGB32(vidmmap->format);
				if(converter == NULL) {
					fprintf(stderr, "vloopback: unsupported pixel format(%d) is requested.\n",vidmmap->format);
					return EINVAL;
				}
				pixel_format = vidmmap->format;
			}
			if(vidmmap->frame > 1 || vidmmap->frame < 0)
				return EINVAL;
			vloopback_width = vidmmap->width;
			vloopback_height = vidmmap->height;

			if(gbufstat[vidmmap->frame] != GBUFFER_UNUSED) {
				if(gbufqueue[0] == vidmmap->frame || gbufqueue[1] == vidmmap->frame) {
					return EBUSY;
				}
			}
			gbuf_lock();
			gbufstat[vidmmap->frame] = GBUFFER_GRABBING;
			if(gbufqueue[0] < 0) {
				gbufqueue[0] = vidmmap->frame;
			} else  {
				gbufqueue[1] = vidmmap->frame;
			}
			gbuf_unlock();
			return 0;
		}

#if VLOOPBACK_VERSION > 83
		case VIDIOCGMBUF:
#else
		case VIDIOCGMBUF & 0xff:
#endif
		{
			struct video_mbuf *vidmbuf = arg;

			vidmbuf->size = max_gbufsize;
			vidmbuf->frames = 2;
			vidmbuf->offsets[0] = 0;
			vidmbuf->offsets[1] = max_framesize;
			return 0;
		}

		default:
		{
			return EPERM;
		}
	}
}

static int signal_loop_init(void)
{
	outputfd = open(output_devname, O_RDWR);
	if(outputfd < 0) {
		fprintf(stderr, "vloopback: couldn't open output device file %s\n",output_devname);
		return -1;
	}
	pixel_format = VIDEO_PALETTE_RGB32;
	pixel_depth = 4;
	converter = palette_get_supported_converter_fromRGB32(pixel_format);
	vloopback_width = screen_width;
	vloopback_height = screen_height;
	max_framesize = vloopback_width * vloopback_height * sizeof(RGB32);
	max_gbufsize = max_framesize * 2;
	gbuf = vloopback_mmap(outputfd, max_gbufsize);
	gbuf_clear();
	return 0;
}

static void *signal_loop(void *arg)
{
	int signo;
	int size, ret;
	struct pollfd ufds;
	sigset_t sigset;
#if VLOOPBACK_VERSION > 83
	unsigned long int cmd;
#endif

	if(signal_loop_init()) {
		signal_loop_initialized = -1;
		return NULL;
	}
	signal_loop_initialized = 1;

	sigemptyset(&sigset);
	sigaddset(&sigset, SIGIO);
	while(signal_loop_thread_flag) {
		sigwait(&sigset, &signo);
		if(signo != SIGIO)
			continue;
		ufds.fd = outputfd;
		ufds.events = POLLIN;
		ufds.revents = 0;
		poll(&ufds, 1, 1000);
		if(!ufds.revents & POLLIN) {
			fprintf(stderr, "vloopback: received signal but got negative on poll.\n");
			continue;
		}
		size = read(outputfd, ioctlbuf, MAXIOCTL);
#if VLOOPBACK_VERSION > 83
		if(size >= sizeof(unsigned long int)) {
			memcpy(&cmd, ioctlbuf, sizeof(unsigned long int));
			if(cmd == 0) {
#else
		if(size >= 1) {
			if(ioctlbuf[0] == 0) {
#endif
				fprintf(stderr, "vloopback: client closed device.\n");
				gbuf_lock();
				gbuf_clear();
				gbuf_unlock();
				continue;
			}
#if VLOOPBACK_VERSION > 83
			ret = v4l_ioctlhandler(cmd, ioctlbuf+sizeof(unsigned long int));
			if(ret) {
				/* new vloopback patch supports a way to return EINVAL to
				 * a client. */
				memset(ioctlbuf+sizeof(unsigned long int), 0xff, MAXIOCTL-sizeof(unsigned long int));
				fprintf(stderr, "vloopback: ioctl %lx unsuccessfully handled.\n", cmd);
				ioctl(outputfd, VIDIOCSINVALID);
			}
			if(ioctl(outputfd, cmd, ioctlbuf+sizeof(unsigned long int))) {
				fprintf(stderr, "vloopback: ioctl %lx unsuccessfull.\n", cmd);
			}
#else
			ret = v4l_ioctlhandler(ioctlbuf[0], ioctlbuf+1);
			if(ret) {
				memset(ioctlbuf+1, 0xff, MAXIOCTL-1);
				fprintf(stderr, "vloopback: ioctl %d unsuccessfull.\n", ioctlbuf[0]);
			}
			ioctl(outputfd, ioctlbuf[0], ioctlbuf+1);
#endif
		}
	}
	return NULL;
}

int vloopback_init(char *name)
{
	sigset_t sigset;

	output_devname = name;

	sigemptyset(&sigset);
	sigaddset(&sigset, SIGIO);
	pthread_sigmask(SIG_BLOCK, &sigset, NULL);

	pthread_mutex_init(&gbufmutex, NULL);
	pthread_cond_init(&gbufcond, NULL);
	signal_loop_thread_flag = 1;
	signal_loop_initialized = 0;
	pthread_create(&signal_loop_thread, NULL, signal_loop, NULL);

	while(signal_loop_initialized == 0) {
		usleep(100000);
	}
	if(signal_loop_initialized == -1)
		return -1;
	fprintf(stderr, "vloopback: video pipelining is OK.\n");
	atexit(vloopback_quit);
	return 0;
}

/* vloopback_quit() is called automatically when the process terminates.
 * This function is registerd in vloopback_init() by calling atexit(). */
void vloopback_quit(void)
{
	close(outputfd);
	fprintf(stderr, "vloopback: video pipelining is stopped.\n");
}

int vloopback_push(void)
{
	int frame;
	RGB32 *src;
	unsigned char *dest;

	if(gbufqueue[0] >= 0) {
		gbuf_lock();
		frame = gbufqueue[0];
		src = (RGB32 *)screen_getaddress();
		dest = gbuf + max_framesize * frame;
		if(converter) {
			converter(src, screen_width, screen_height, dest, vloopback_width, vloopback_height);
		}
		gbufstat[frame] = GBUFFER_DONE;
		gbufqueue[0] = gbufqueue[1];
		gbufqueue[1] = -1;
		gbuf_unlock();
		pthread_cond_signal(&gbufcond);
	}
	return 0;
}
