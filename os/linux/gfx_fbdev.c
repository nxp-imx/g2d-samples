/*
 * Copyright 2021 NXP
 * Copyright 2013 Freescale Semiconductor, Inc.
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*
 * @file gfx_fbdev.c
 *
 * @brief Graphics initialization source file
 *
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <fcntl.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "gfx_init.h"

int init_graphics(graphics_handler_t *handler, screeninfo_t *screen_info,
                  int *fb0_phys, int *fb0_size) {
  struct fb_fix_screeninfo fb_info;
  int retval = 0;
  int *fd_fb0 = &handler->fd_fb0;

  if ((*fd_fb0 = open("/dev/fb0", O_RDWR, 0)) < 0) {
    printf("Unable to open fb0\n");
    return -1;
  }

  /* Get fix screen info. */
  retval = ioctl(*fd_fb0, FBIOGET_FSCREENINFO, &fb_info);
  if (retval < 0) {
    goto err2;
  }
  *fb0_phys = fb_info.smem_start;

  /* Get variable screen info. */
  retval = ioctl(*fd_fb0, FBIOGET_VSCREENINFO, screen_info);
  if (retval < 0) {
    goto err2;
  }

  /* Use xres_virtual to store real stride */
  (*screen_info).xres_virtual =
      fb_info.line_length / ((*screen_info).bits_per_pixel >> 3);

  *fb0_phys += ((*screen_info).xres_virtual * (*screen_info).yoffset *
                (*screen_info).bits_per_pixel / 8);

  *fb0_size = (*screen_info).xres_virtual * (*screen_info).yres_virtual *
              (*screen_info).bits_per_pixel / 8;

  return retval;

err2:
  close(*fd_fb0);

  return retval;
}

void deinit_graphics(graphics_handler_t *handler) {
  int *fd_fb0 = &handler->fd_fb0;
  close(*fd_fb0);
}
