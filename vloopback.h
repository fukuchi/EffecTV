/*
 * EffecTV - Realtime Digital Video Effector
 * Copyright (C) 2001 FUKUCHI Kentarou
 *
 * vloopback.h: header for vloopback device manager
 *
 */

#ifndef __VLOOPBACK_H__
#define __VLOOPBACK_H__

extern int vloopback;

int vloopback_init(char *name);
void vloopback_quit();
int vloopback_push();

#endif /* __VLOOPBACK_H__ */
