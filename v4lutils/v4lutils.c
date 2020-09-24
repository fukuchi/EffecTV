/*
 * v4lutils - utility library for Video4Linux
 * Copyright (C) 2001-2002, 2020 FUKUCHI Kentaro
 *
 * v4lutils.c: utility functions
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <http://www.gnu.org/licenses/>.
 *
 */

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include <errno.h>
#include <linux/videodev2.h>
#include <libv4l2.h>
#include "v4lutils.h"

#define DEFAULT_DEVICE "/dev/video"


#define STRBUF_LENGTH 1024

static int v4l_debug = 0; /* 1 = print debug message */

static int v4lperror_level = V4L_PERROR_ALL;

/*
 * v4lperror - inhouse perror.
 *
 */
static void v4lperror(const char *str)
{
	if(v4lperror_level >= V4L_PERROR_ALL)
		perror(str);
}

/*
 * v4lopen - open the v4l device.
 *
 * name: device file
 * vd: v4l device object
 */
int v4lopen(char *name, v4ldevice *vd)
{
	uint32_t caps;
	char buf[STRBUF_LENGTH];

	if(name == NULL)
		name = DEFAULT_DEVICE;

	if(v4l_debug) fprintf(stderr, "v4lopen:open...\n");
	if((vd->fd = v4l2_open(name,O_RDWR)) < 0) {
		snprintf(buf, STRBUF_LENGTH, "v4lopen: failed to open %s", name);
		v4lperror(buf);
		return -1;
	}
	if(v4lgetcapability(vd))
		return -1;
	if(vd->capability.capabilities & V4L2_CAP_DEVICE_CAPS) {
		caps = vd->capability.device_caps;
	} else {
		caps = vd->capability.capabilities;
	}
	if(!(caps & V4L2_CAP_VIDEO_CAPTURE)) {
		if(caps & V4L2_CAP_META_CAPTURE) {
			fprintf(stderr, "This device is for metadata capture. Try another device file.\n");
		} else {
			fprintf(stderr, "This device does not support video capture.\n");
		}
		v4lclose(vd);
		return -1;
	}
	if(v4lenuminputs(vd)) {
		v4lclose(vd);
		return -1;
	}

	pthread_mutex_init(&vd->mutex, NULL);
	if(v4l_debug) fprintf(stderr, "v4lopen:quit\n");
	return 0;
}

/*
 * v4lclose - close v4l device
 *
 * vd: v4l device object
 */
int v4lclose(v4ldevice *vd)
{
	if(v4l_debug) fprintf(stderr, "v4lclose:close...\n");
	v4l2_close(vd->fd);
	if(v4l_debug) fprintf(stderr, "v4lclose:quit\n");
	return 0;
}

/*
 * v4lgetcapability - get the capability of v4l device
 *
 * vd: v4l device object
 */
int v4lgetcapability(v4ldevice *vd)
{
	if(v4l_debug) fprintf(stderr, "v4lgetcapability:VIDIOC_QUERYCAP...\n");
	if(v4l2_ioctl(vd->fd, VIDIOC_QUERYCAP, &(vd->capability)) < 0) {
		v4lperror("v4lgetcapability:VIDIOC_QUERYCAP");
		return -1;
	}
	if(v4l_debug) fprintf(stderr, "v4lgetcapability:quit\n");
	return 0;
}

/* 
 * v4lenuminputs - enumerate all video inputs
 */
int v4lenuminputs(v4ldevice *vd)
{
	int i;

	memset(vd->inputs, 0, sizeof(vd->inputs));
	for(i=0; i<V4L_MAXINPUTS; i++) {
		vd->inputs[i].index = i;
		if(v4l2_ioctl(vd->fd, VIDIOC_ENUMINPUT, &vd->inputs[i]) < 0) {
			if(errno == EINVAL) {
				break;
			}
			v4lperror("v4lenuminputs:VIDIOC_ENUMINPUT");
			return -1;
		}
	}
	vd->inputs_num = i;
	return 0;
}

/*
 * v4lsetinput - select the video source
 *
 * vd: v4l device object
 * ch: the channel number
 */
int v4lsetinput(v4ldevice *vd, int ch)
{
	if(v4l2_ioctl(vd->fd, VIDIOC_S_INPUT, &(ch)) < 0) {
		v4lperror("v4lsetchannel:VIDIOC_S_INPUT");
		return -1;
	}
	return 0;
}

/*
 * v4lgrabinit - set parameters for mmap interface
 *
 * vd: v4l device object
 * width: width of the buffer
 * height: height of the buffer
 */
int v4lgrabinit(v4ldevice *vd, int width, int height)
{
	struct v4l2_requestbuffers req;
	struct v4l2_buffer buf;
	int i;

	vd->fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	vd->fmt.fmt.pix.width  = width;
	vd->fmt.fmt.pix.height = height;
	vd->fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_BGR24;
	vd->fmt.fmt.pix.field = V4L2_FIELD_INTERLACED;

	if (v4l2_ioctl(vd->fd, VIDIOC_S_FMT, &vd->fmt) < 0) {
		v4lperror("v4lgrabinit:VIDIOC_S_FMT");
		return -1;
	}

	memset(&req, 0, sizeof(req));
	req.count = V4L_FRAMEBUFFERS;
	req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	req.memory = V4L2_MEMORY_MMAP;
	if(v4l2_ioctl(vd->fd, VIDIOC_REQBUFS, &req) < 0) {
		v4lperror("v4lgrabinit:VIDIOC_REQBUFS");
		return -1;
	}

	for(i = 0; i < req.count; i++) {
		memset(&buf, 0, sizeof(buf));
		buf.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_MMAP;
		buf.index  = i;

		if (v4l2_ioctl(vd->fd, VIDIOC_QUERYBUF, &buf) < 0) {
			v4lperror("v4lgrabinit: VIDIOC_QUERYBUF");
			return -1;
		}

		vd->frames[i].length = buf.length;
		vd->frames[i].data = v4l2_mmap(NULL, buf.length, PROT_READ|PROT_READ, MAP_SHARED, vd->fd, buf.m.offset);
		if(vd->frames[i].data == MAP_FAILED) {
			v4lperror("v4lgrabinit: v4l2_mmap");
			return -1;
		}
	}

	return 0;
}

/*
 * v4lgrabstart - activate mmap capturing
 *
 * vd: v4l device object
 */
int v4lgrabstart(v4ldevice *vd)
{
	int i;
	struct v4l2_buffer buf;
	enum v4l2_buf_type type;

	if(v4l_debug) fprintf(stderr, "v4lgrabstart: grab frame.\n");
	for (i = 0; i < V4L_FRAMEBUFFERS; ++i) {
		memset(&buf, 0, sizeof(buf));
		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_MMAP;
		buf.index = i;
		if(v4l2_ioctl(vd->fd, VIDIOC_QBUF, &buf) < 0) {
			v4lperror("v4lgrabstart:VIDIOC_QBUF");
			return -1;
		}
	}
	type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if (v4l2_ioctl(vd->fd, VIDIOC_STREAMON, &type) < 0) {
		v4lperror("v4lgrabstart:VIDIOC_STREAMON");
		return -1;
	}

	return 0;
}

/*
 * v4lgrabstop - stop mmap capturing
 *
 * vd: v4l device object
 */
int v4lgrabstop(v4ldevice *vd)
{
	enum v4l2_buf_type type;
	int i;

	type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if (v4l2_ioctl(vd->fd, VIDIOC_STREAMOFF, &type) < 0) {
		v4lperror("v4lgrabstop:VIDIOC_STREAMOFF");
		return -1;
	}
	for (i = 0; i < V4L_FRAMEBUFFERS; ++i) {
		v4l2_munmap(vd->frames[i].data, vd->frames[i].length);
	}

	return 0;
}

/*
 * v4lsync - wait until mmap capturing of the frame is finished
 *
 * vd: v4l device object
 */
int v4lsync(v4ldevice *vd)
{
	fd_set fds;
	struct timeval tv;
	int ret;

	if(v4l_debug) fprintf(stderr, "v4lsync: sync frame.\n");

	FD_ZERO(&fds);
	FD_SET(vd->fd, &fds);

	tv.tv_sec = 1;
	tv.tv_usec = 0;
	ret = select(vd->fd + 1, &fds, NULL, NULL, &tv);

	if(ret < 0) {
		v4lperror("v4lsync: select failed");
		return -1;
	} else if(ret == 0) {
		v4lperror("v4lsync: select timeout");
		return -1;
	}

	return 0;
}

/*
 * v4lgetaddress - returns a offset addres of buffer for mmap capturing
 *
 * vd: v4l device object
 */
unsigned char *v4lgetaddress(v4ldevice *vd)
{
	memset(&vd->lastbuf, 0, sizeof(vd->lastbuf));

	vd->lastbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	vd->lastbuf.memory = V4L2_MEMORY_MMAP;

	if (v4l2_ioctl(vd->fd, VIDIOC_DQBUF, &vd->lastbuf) < 0) {
		if(errno == EAGAIN) {
			return NULL;
		} else {
			v4lperror("v4lgetaddress: VIDIOC_DQBUF");
			return NULL;
		}
	}

	assert(vd->lastbuf.index < V4L_FRAMEBUFFERS);

	return vd->frames[vd->lastbuf.index].data;
}

/*
 * v4lnext - start the next mmap capturing
 *
 * vd: v4l device object
 */

int v4lnext(v4ldevice *vd)
{
	if(v4l2_ioctl(vd->fd, VIDIOC_QBUF, &vd->lastbuf) < 0) {
		v4lperror("v4lnext: VIDIOC_QBUF");
		return -1;
	}
	return 0;
}

/*
 * v4llock - lock the Video4Linux device object
 *
 * vd: v4l device object
 */
int v4llock(v4ldevice *vd)
{
	return pthread_mutex_lock(&vd->mutex);
}

/*
 * v4lunlock - unlock the Video4Linux device object
 *
 * vd: v4l device object
 */
int v4lunlock(v4ldevice *vd)
{
	return pthread_mutex_unlock(&vd->mutex);
}

/*
 * v4ltrylock - lock the Video4Linux device object (non-blocking mode)
 *
 * vd: v4l device object
 */
int v4ltrylock(v4ldevice *vd)
{
	return pthread_mutex_trylock(&vd->mutex);
}


/*
 * v4linfo - print v4l device object
 *
 * vd: v4l device object
 */
#define V4L2_CAPABILITY_CHECK(_flag_) if(vd->capability.capabilities & _flag_) printf(#_flag_  ",")

void v4linfo(v4ldevice *vd)
{
	int i;

	printf("Driver: %s\n", vd->capability.driver);
	printf("Card: %s\n", vd->capability.card);
	printf("Device type;\n");
	V4L2_CAPABILITY_CHECK(V4L2_CAP_VIDEO_CAPTURE);
	V4L2_CAPABILITY_CHECK(V4L2_CAP_VIDEO_OVERLAY);
	V4L2_CAPABILITY_CHECK(V4L2_CAP_TUNER);
	printf("Number of inputs: %d\n", vd->inputs_num);
	for(i=0; i<vd->inputs_num; i++) {
		printf("Input #%d:\n", i);
		printf("  Name: %s\n", vd->inputs[i].name);
		switch(vd->inputs[i].type) {
			case V4L2_INPUT_TYPE_TUNER:
				printf("  Type: tuner\n");
				break;
			case V4L2_INPUT_TYPE_CAMERA:
				printf("  Type: camera\n");
				break;
			default:
				printf("  Type: unknown\n");
				break;
		}
	}
}

/*
 * v4lseterrorlevel - enable/disable perror message output
 *
 * flag: V4L_PERROR_NONE or V4L_PERROR_ALL(default)
 */
void v4lseterrorlevel(int flag)
{
	v4lperror_level = flag;
}

/*
 * v4ldebug - enable/disable debug message output
 *
 * flag: 0 = disable / 1 = enable
 */
void v4ldebug(int flag)
{
	fprintf(stderr, "debug: %d\n",flag);
	v4l_debug = flag;
}
