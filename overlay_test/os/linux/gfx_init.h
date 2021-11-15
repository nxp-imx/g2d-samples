/*
 * Copyright 2021 NXP
 * Copyright 2013 Freescale Semiconductor, Inc.
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*
 * @file gfx_init.h
 *
 * @brief Graphics initialization header file
 *
 */

#ifndef __SCREEN_INIT_H__
#define __SCREEN_INIT_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Incude FB specific header files */
#include <linux/fb.h>

typedef struct graphics_handler {
  int fd_fb0;
} graphics_handler_t;

typedef struct fb_var_screeninfo screeninfo_t;

int init_graphics(graphics_handler_t *handler, screeninfo_t *screen_info,
                  int *fb0_phys, int *fb0_size);
void deinit_graphics(graphics_handler_t *handler);

#ifdef __cplusplus
}
#endif

#endif
