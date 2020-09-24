/*
 * v4lutils - utility library for Video4Linux
 * Copyright (C) 2001-2002, 2020 FUKUCHI Kentaro
 *
 * v4lutils.h: header file
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

#ifndef __V4LUTILS_H__
#define __V4LUTILS_H__

#include <sys/types.h>
#include <linux/videodev2.h>
#include <pthread.h>

/*
 * Error message displaying level
 */
#define V4L_PERROR_NONE (0)
#define V4L_PERROR_ALL (1)

/*
 * Constant values
 */
#define V4L_MAXINPUTS (10)
#define V4L_FRAMEBUFFERS (2)

/*
 * Video4Linux Device Structure
 */
struct frame {
	unsigned char *data;
	int length;
};

struct _v4ldevice
{
	int fd;
	struct v4l2_format fmt;
	struct v4l2_capability capability;
	int inputs_num;
	struct v4l2_input inputs[V4L_MAXINPUTS];
	struct frame frames[V4L_FRAMEBUFFERS];
	pthread_mutex_t mutex;
	struct v4l2_buffer lastbuf;
};

typedef struct _v4ldevice v4ldevice;

extern int v4lopen(char *, v4ldevice *);
extern int v4lclose(v4ldevice *);
extern int v4lgetcapability(v4ldevice *);
extern int v4lenuminputs(v4ldevice *);
extern int v4lgrabinit(v4ldevice *vd, int width, int height);
extern int v4lgrabstart(v4ldevice *vd);
extern int v4lgrabstop(v4ldevice *vd);
extern int v4lsync(v4ldevice *vd);
extern unsigned char *v4lgetaddress(v4ldevice *);
extern int v4lnext(v4ldevice *vd);
extern int v4llock(v4ldevice *vd);
extern int v4ltrylock(v4ldevice *vd);
extern int v4lunlock(v4ldevice *vd);
extern void v4linfo(v4ldevice *);
extern void v4lseterrorlevel(int);
extern void v4ldebug(int);

#endif /* __V4LUTILS_H__ */
