/*
 * EffecTV - Realtime Digital Video Effector
 * Copyright (C) 2001 FUKUCHI Kentarou
 *
 * buffer.c: shared buffer manager
 *
 */

#include <stdlib.h>
#include <string.h>
#include "../EffecTV.h"
#include "utils.h"

unsigned char *sharedbuffer = NULL;
int sharedbuffer_length;

static int tail;

int sharedbuffer_init()
{
	/* maximum size of the frame buffer is for screen size x 2 */
	sharedbuffer_length = screen_width * screen_height * sizeof(RGB32) * 2;

	sharedbuffer = (unsigned char *)malloc(sharedbuffer_length);
	if(sharedbuffer == NULL)
		return -1;
	else
		return 0;
}

/* The effects uses shared buffer must call this function at first in
 * each effect registrar.
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

	if(sharedbuffer_length - tail < size) {
		return NULL;
	}

	head = sharedbuffer + tail;
	tail += size;

	return head;
}
