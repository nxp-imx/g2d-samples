/*
 * Copyright 2021 NXP
 * Copyright 2013 Freescale Semiconductor, Inc.
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*
 * @file gfx_screen.c
 *
 * @brief Graphics initialization source file
 *
 */

#ifdef __cplusplus
extern "C" {
#endif

/* Standard include files */
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "gfx_init.h"

static graphics_handler_t *handler_ptr;

int init_graphics(graphics_handler_t *handler, screeninfo_t *screen_info,
                  int *buffer_phys, int *buffer_size) {
  int vis = 0, val = 0;
  int count = 0;
  int dims[2] = {0, 0};
  int rect[4] = {0, 0, 0, 0};
  long long buf_phys;
  screen_display_t *screen_disps, screen_disp;
  screen_buffer_t screen_buf;
  unsigned char *ptr = NULL;
  screen_window_t *screen_bg_win = &handler->screen_bg_win;
  screen_context_t *screen_ctx = &handler->screen_ctx;

  // Create screen context
  if (screen_create_context(screen_ctx, SCREEN_APPLICATION_CONTEXT) < 0) {
    printf("Unable to create context\n");
    return -1;
  }
  if (screen_get_context_property_iv(*screen_ctx, SCREEN_PROPERTY_DISPLAY_COUNT,
                                     &count) < 0) {
    printf("Unable to get context property\n");
    goto err0;
  }

  // Select display (by default the first display) and get resolution
  screen_disps = calloc(count, sizeof(screen_display_t));
  if (screen_get_context_property_pv(*screen_ctx, SCREEN_PROPERTY_DISPLAYS,
                                     (void **)screen_disps) < 0) {
    printf("Unable to get context property\n");
    goto err0;
  }

  screen_disp = screen_disps[0];
  free(screen_disps);
  if (screen_get_display_property_iv(screen_disp, SCREEN_PROPERTY_SIZE, dims) <
      0) {
    printf("Unable to get display property\n");
    goto err0;
  }

  // Create a window we will use to write the bitmaps to. By default, the window
  // is fullscreen.
  if (screen_create_window(screen_bg_win, *screen_ctx) < 0) {
    printf("Unable to create window\n");
  }
  vis = 0;
  if (screen_set_window_property_iv(*screen_bg_win, SCREEN_PROPERTY_VISIBLE,
                                    &vis) < 0) {
    printf("Unable to set window property - visible\n");
    goto err1;
  }
  val = SCREEN_USAGE_READ | SCREEN_USAGE_WRITE; // | SCREEN_USAGE_NATIVE;
  if (screen_set_window_property_iv(*screen_bg_win, SCREEN_PROPERTY_USAGE,
                                    &val) < 0) {
    printf("Unable to set window property - usage\n");
    goto err1;
  }

  val = SCREEN_FORMAT_RGBA8888;
  if (screen_set_window_property_iv(*screen_bg_win, SCREEN_PROPERTY_FORMAT,
                                    &val) < 0) {
    printf("Unable to set window property - format\n");
    goto err1;
  }

  val = SCREEN_TRANSPARENCY_NONE;
  if (screen_set_window_property_iv(*screen_bg_win,
                                    SCREEN_PROPERTY_TRANSPARENCY, &val)) {
    printf("Unable to set window property - transparency\n");
    goto err1;
  }

  if (screen_set_window_property_pv(*screen_bg_win, SCREEN_PROPERTY_DISPLAY,
                                    (void **)&screen_disp) < 0) {
    printf("Unable to set window property - display\n");
    goto err1;
  }

  // Create a window buffer
  if (screen_create_window_buffers(*screen_bg_win, 1) < 0) {
    printf("Unable to create buffer\n");
    goto err1;
  }
  if (screen_get_window_property_pv(*screen_bg_win,
                                    SCREEN_PROPERTY_RENDER_BUFFERS,
                                    (void **)&screen_buf) < 0) {
    printf("Unable to set buffer - render buffers: %s\n", strerror(errno));
    goto err1;
  }
  val = 1;
  /*
  if (screen_set_buffer_property_iv(screen_buf,
          SCREEN_PROPERTY_PHYSICALLY_CONTIGUOUS, &val) < 0) {
      printf("Unable to set buffer property - contiguous: %s\n",
              strerror( errno ));
  }
  */
  if (screen_get_buffer_property_llv(
          screen_buf, SCREEN_PROPERTY_PHYSICAL_ADDRESS, &buf_phys) < 0) {
    printf("Unable to get buffer property - paddr: %s\n", strerror(errno));
    goto err1;
  }
  *buffer_phys = (int)buf_phys;

  if (screen_get_buffer_property_pv(screen_buf, SCREEN_PROPERTY_POINTER,
                                    (void **)&ptr) < 0) {
    printf("Unable to get buffer property - pointer: %s\n", strerror(errno));
    goto err1;
  }

  vis = 1;
  screen_set_window_property_iv(*screen_bg_win, SCREEN_PROPERTY_VISIBLE, &vis);
  (*screen_info).xres = dims[0];
  (*screen_info).xres_virtual = dims[0];
  (*screen_info).yres = dims[1];

  (*screen_info).bits_per_pixel = 32;
  (*screen_info).red.offset = 0;

  *buffer_size = (*screen_info).xres * (*screen_info).yres *
                 (*screen_info).bits_per_pixel / 8;

  rect[2] = dims[0];
  rect[3] = dims[1];

  if (screen_post_window(*screen_bg_win, screen_buf, 1, rect,
                         SCREEN_WAIT_IDLE) < 0) {
    printf("Unable to post window\n");
    goto err1;
  }

  handler->screen_buf = screen_buf;
  handler_ptr = handler;

  return 0;

err1:
  screen_destroy_window(*screen_bg_win);
err0:
  screen_destroy_context(*screen_ctx);

  return -1;
}

void qnx_post_window(screeninfo_t *screen_info) {
  int rect[4] = {0, 0, 0, 0};
  rect[2] = screen_info->xres;
  rect[3] = screen_info->yres;
  if (screen_post_window(handler_ptr->screen_bg_win, handler_ptr->screen_buf, 1,
                         rect, SCREEN_WAIT_IDLE) < 0) {
    printf("Unable to post window\n");
  }
}

void deinit_graphics(graphics_handler_t *handler) {
  screen_window_t *screen_bg_win = &handler->screen_bg_win;
  screen_context_t *screen_ctx = &handler->screen_ctx;
  screen_destroy_window(*screen_bg_win);
  screen_destroy_context(*screen_ctx);
}
