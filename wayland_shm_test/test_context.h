/*
 * Copyright 2021 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/*
 * test_context.h
 */

#include <unistd.h>

typedef struct {
  struct g2d_buf *g2d_data;
  void *shm_data;
  int phy_data;
  size_t width;
  size_t height;

} test_context;

extern test_context *test_context_alloc(size_t width, size_t height);
extern void paint_pixels(test_context *tc);
