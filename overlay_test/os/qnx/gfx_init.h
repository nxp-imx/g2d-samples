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

#include <screen/screen.h>

typedef struct graphics_handler {
  screen_window_t screen_bg_win;
  screen_context_t screen_ctx;
  screen_buffer_t screen_buf;
} graphics_handler_t;

/* Include QNX specific header files */
#include <screen/screen.h>
#include <stdint.h>
#include <sys/time.h>

struct fb_bitfield {
  uint32_t offset;    /* beginning of bitfield        */
  uint32_t length;    /* length of bitfield           */
  uint32_t msb_right; /* != 0 : Most significant bit is right*/
};

typedef struct screeninfo {
  uint32_t xres; /* visible resolution		*/
  uint32_t yres;
  uint32_t xres_virtual; /* virtual resolution		*/

  uint32_t bits_per_pixel;  /* guess what			*/
                            /* >1 = FOURCC			*/
  struct fb_bitfield red;   /* bitfield in fb mem if true color, */
  struct fb_bitfield green; /* else only length is significant */
  struct fb_bitfield blue;
  struct fb_bitfield transp; /* transparency			*/
} screeninfo_t;

int init_graphics(graphics_handler_t *handler, screeninfo_t *screen_info,
                  int *buffer_phys, int *buffer_size);
void qnx_post_window(screeninfo_t *screen_info);
void deinit_graphics(graphics_handler_t *handler);

#ifdef __cplusplus
}
#endif

#endif
