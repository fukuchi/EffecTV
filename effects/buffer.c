/*
 * EffecTV - Realtime Digital Video Effector
 * Copyright (C) 2001 FUKUCHI Kentarou
 *
 * buffer.c: shared buffer
 *
 */

#include <stdlib.h>
#include <string.h>
#include "../EffecTV.h"
#include "utils.h"

unsigned char *sharedbuffer;
int sharedbuffer_length;

static int tail;

int sharedbuffer_init(int scale)
{
	/* maximum size of the frame buffer is for double scaled screen x 2 */
	sharedbuffer_length = SCREEN_WIDTH*SCREEN_HEIGHT*scale*scale*2*4;

	sharedbuffer = (unsigned char *)malloc(sharedbuffer_length);
	if(sharedbuffer == NULL)
		return -1;
	else
		return 0;
}

/* The effects uses shared buffer must call this function at first in
 * *Register()
 */
void sharedbuffer_reset()
{
	tail = 0;
}

/* Allocates size bytes memory in shared buffer and returns a pointer to the
 * memory. NULL is returned when the rest memory is not enough for the request.
 */
unsigned char *sharedbuffer_alloc(int size)
{
	unsigned char *head;

	if(sharedbuffer_length-tail < size) {
		return NULL;
	}

	head = sharedbuffer + tail;
	tail += size;

	return head;
}
