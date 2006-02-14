/*
 * EffecTV - Realtime Digital Video Effector
 * Copyright (C) 2001-2006 FUKUCHI Kentaro
 *
 * vloopback.h: header for vloopback device manager
 *
 */

#ifndef __VLOOPBACK_H__
#define __VLOOPBACK_H__

extern int vloopback;

int vloopback_init(char *name);
void vloopback_quit(void);
int vloopback_push(void);

#endif /* __VLOOPBACK_H__ */
