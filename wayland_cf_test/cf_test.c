/*
 * Copyright 2021 NXP
 * Copyright 2016 Freescale Semiconductor, Inc.
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/*
 * cf_test.c
 */

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "test_context.h"
#include <g2dExt.h>

#define CACHEABLE 0
#define FRAMES 30

static void g2d_fill_buffer(test_context *tc) {
  struct g2d_surfaceEx srcEx, dstEx;
  struct g2d_surface *src = &srcEx.base;
  struct g2d_surface *dst = &dstEx.base;
  struct g2d_buf *buf = NULL;
  void *g2dHandle;

  if (g2d_open(&g2dHandle) == -1 || g2dHandle == NULL) {
    fprintf(stderr, "Fail to open g2d device!\n");
    return;
  }

  // alloc physical contiguous memory for source image data
  buf = g2d_alloc(tc->src_sz, CACHEABLE);
  if (!buf) {
    fprintf(stderr, "Fail to allocate physical memory for image buffer!\n");
    goto OnError;
  }

  memcpy(buf->buf_vaddr, tc->src_buf, tc->src_sz);

#if CACHEABLE
  g2d_cache_op(buf, G2D_CACHE_FLUSH);
#endif

  /*
   NOTE: in this example, all the test image data meet with the alignment
   requirement. Thus, in your code, you need to pay attention on that.

   Pixel buffer address alignment requirement,
   RGB/BGR: pixel data in planes [0] with 16bytes alignment,
   NV12/NV16: Y in planes [0], UV in planes [1], with 64bytes alignment,
   I420: Y in planes [0], U in planes [1], V in planes [2], with 64 bytes
   alignment,
   YV12: Y in planes [0], V in planes [1], U in planes [2], with 64 bytes
   alignment,
   NV21/NV61: Y in planes [0], VU in planes [1], with 64 bytes alignment,
   YUYV/YVYU/UYVY/VYUY: in planes[0], buffer address is with 16 bytes alignment.
  */

  src->format = tc->src_color_format;
  switch (src->format) {
  case G2D_RGB565:
  case G2D_RGBA8888:
  case G2D_RGBX8888:
  case G2D_BGRA8888:
  case G2D_BGRX8888:
  case G2D_BGR565:
  case G2D_YUYV:
  case G2D_UYVY:
  default:
    src->planes[0] = buf->buf_paddr;
    break;
  case G2D_NV21:
    src->planes[0] = buf->buf_paddr;
    src->planes[1] = buf->buf_paddr + tc->src_width * tc->src_height;
    break;
  case G2D_NV12:
    src->planes[0] = buf->buf_paddr;
    src->planes[1] = buf->buf_paddr + tc->src_width * tc->src_height;
    break;
  case G2D_I420:
    src->planes[0] = buf->buf_paddr;
    src->planes[1] = buf->buf_paddr + tc->src_width * tc->src_height;
    src->planes[2] = src->planes[1] + tc->src_width * tc->src_height / 4;
    break;
  case G2D_NV16:
    src->planes[0] = buf->buf_paddr;
    src->planes[1] = buf->buf_paddr + tc->src_width * tc->src_height;
    break;
  case G2D_NV61:
    src->planes[0] = buf->buf_paddr;
    src->planes[1] = buf->buf_paddr + tc->src_width * tc->src_height;
    break;
  }

  srcEx.tiling = tc->src_tiling;
  dstEx.tiling = G2D_LINEAR;

  src->left = 0;
  src->top = 0;
  src->right = tc->src_width;
  src->bottom = tc->src_height;
  src->stride = tc->src_width;
  src->width = tc->src_width;
  src->height = tc->src_height;
  src->rot = G2D_ROTATION_0;

  dst->planes[0] = tc->dst_paddr;
  dst->left = tc->dst_left;
  dst->top = tc->dst_top;
  dst->right = tc->dst_left + tc->dst_width;
  dst->bottom = tc->dst_top + tc->dst_height;
  dst->stride = tc->dst_width;
  dst->width = tc->dst_width;
  dst->height = tc->dst_height;
  dst->rot = tc->dst_rotation;
  dst->format = tc->dst_color_format;

  if (tc->src_set_alpha) {
    assert(0);
    src->blendfunc = G2D_ONE;
    dst->blendfunc = G2D_ONE_MINUS_SRC_ALPHA;

    src->global_alpha = 0x80;
    dst->global_alpha = 0xff;

    g2d_enable(g2dHandle, G2D_BLEND);
    g2d_enable(g2dHandle, G2D_GLOBAL_ALPHA);
  }

  if (tc->src_set_blur) {
    g2d_enable(g2dHandle, G2D_BLUR);
  }

  g2d_blitEx(g2dHandle, &srcEx, &dstEx);
  g2d_finish(g2dHandle);

  if (tc->src_set_alpha) {
    g2d_disable(g2dHandle, G2D_GLOBAL_ALPHA);
    g2d_disable(g2dHandle, G2D_BLEND);
  }

  if (tc->src_set_blur) {
    g2d_disable(g2dHandle, G2D_BLUR);
  }

OnError:
  if (buf)
    g2d_free(buf);
  g2d_close(g2dHandle);
}

static bool read_file(const char *fname, void **out_buf, size_t *out_sz) {
  int fd = open(fname, O_RDONLY);
  if (fd < 0) {
    fprintf(stderr, "can't open %s: %s\n", fname, strerror(errno));
    return false;
  }
  struct stat statbuf;
  int rv = fstat(fd, &statbuf);
  if (rv) {
    fprintf(stderr, "fstat(%s) failed: %s\n", fname, strerror(errno));
    return false;
  }

  uint8_t *buf = malloc(statbuf.st_size);
  rv = read(fd, buf, statbuf.st_size);
  if (!rv) {
    fprintf(stderr, "can't read %s: %s\n", fname, strerror(errno));
    close(fd);
    return false;
  }

  close(fd);
  *out_sz = statbuf.st_size;
  *out_buf = buf;

  return true;
}

static void test_color_format(test_context *tc, const char *fname,
                              unsigned src_color_format) {
  bool rv = read_file(fname, &tc->src_buf, &tc->src_sz);
  if (!rv)
    return;

  tc->src_width = 1024;
  tc->src_height = 768;
  tc->src_color_format = src_color_format;
  tc->src_tiling = G2D_LINEAR;
  tc->dst_rotation = G2D_ROTATION_0;

  // Wayland WL_SHM_FORMAT_ARGB8888 color-format is stored
  // as little-endian, which translates to
  // G2D_BGRA8888, stored as big-endian.
  tc->dst_color_format = G2D_BGRA8888;
  g2d_fill_buffer(tc);
  free(tc->src_buf);
}

void paint_pixels(test_context *tc) {
  static int count = -1;
  count++;

  if (count < FRAMES * 1) {
    if (!(count % FRAMES))
      fprintf(stderr, "Testing %s.", "G2D_ABGR8888");
    else
      fprintf(stderr, ".");
    test_color_format(tc, "PM5544_MK10_ABGR8888.raw", G2D_ABGR8888);
    return;
  }
  if (count < FRAMES * 2) {
    if (!(count % FRAMES))
      fprintf(stderr, "\nTesting %s.", "G2D_ARGB8888");
    else
      fprintf(stderr, ".");
    test_color_format(tc, "PM5544_MK10_ARGB8888.raw", G2D_ARGB8888);
    return;
  }
  if (count < FRAMES * 3) {
    if (!(count % FRAMES))
      fprintf(stderr, "\nTesting %s.", "G2D_BGR565");
    else
      fprintf(stderr, ".");
    test_color_format(tc, "PM5544_MK10_BGR565.raw", G2D_BGR565);
    return;
  }
  if (count < FRAMES * 4) {
    if (!(count % FRAMES))
      fprintf(stderr, "\nTesting %s.", "G2D_BGRA8888");
    else
      fprintf(stderr, ".");
    test_color_format(tc, "PM5544_MK10_BGRA8888.raw", G2D_BGRA8888);
    return;
  }
  if (count < FRAMES * 5) {
    if (!(count % FRAMES))
      fprintf(stderr, "\nTesting %s.", "G2D_NV12");
    else
      fprintf(stderr, ".");
    test_color_format(tc, "PM5544_MK10_NV12.raw", G2D_NV12);
    return;
  }
  if (count < FRAMES * 6) {
    if (!(count % FRAMES))
      fprintf(stderr, "\nTesting %s.", "G2D_NV16");
    else
      fprintf(stderr, ".");
    test_color_format(tc, "PM5544_MK10_NV16.raw", G2D_NV16);
    return;
  }
  if (count < FRAMES * 7) {
    if (!(count % FRAMES))
      fprintf(stderr, "\nTesting %s.", "G2D_NV21");
    else
      fprintf(stderr, ".");
    test_color_format(tc, "PM5544_MK10_NV21.raw", G2D_NV21);
    return;
  }
  if (count < FRAMES * 8) {
    if (!(count % FRAMES))
      fprintf(stderr, "\nTesting %s.", "G2D_NV61");
    else
      fprintf(stderr, ".");
    test_color_format(tc, "PM5544_MK10_NV61.raw", G2D_NV61);
    return;
  }
  if (count < FRAMES * 9) {
    if (!(count % FRAMES))
      fprintf(stderr, "\nTesting %s.", "G2D_RGB565");
    else
      fprintf(stderr, ".");
    test_color_format(tc, "PM5544_MK10_RGB565.raw", G2D_RGB565);
    return;
  }
  if (count < FRAMES * 10) {
    if (!(count % FRAMES))
      fprintf(stderr, "\nTesting %s.", "G2D_RGBA8888");
    else
      fprintf(stderr, ".");
    test_color_format(tc, "PM5544_MK10_RGBA8888.raw", G2D_RGBA8888);
    return;
  }
  if (count < FRAMES * 11) {
    if (!(count % FRAMES))
      fprintf(stderr, "\nTesting %s.", "G2D_UYVY");
    else
      fprintf(stderr, ".");
    test_color_format(tc, "PM5544_MK10_UYVY422.raw", G2D_UYVY);
    return;
  }
  if (count < FRAMES * 12) {
    if (!(count % FRAMES))
      fprintf(stderr, "\nTesting %s.", "G2D_YUYV");
    else
      fprintf(stderr, ".");
    test_color_format(tc, "PM5544_MK10_YUYV422.raw", G2D_YUYV);
    return;
  }
#if 0
  if (count < FRAMES * 13) {
    if (!(count % FRAMES))
      fprintf(stderr, "\nTesting %s.", "G2D_I420");
    else
      fprintf(stderr, ".");
    test_color_format(tc, "PM5544_MK10_I420.raw", G2D_I420);
    return;
  }
  if (count < FRAMES * 14) {
    if (!(count % FRAMES))
      fprintf(stderr, "\nTesting %s.", "G2D_YV12");
    else
      fprintf(stderr, ".");
    test_color_format(tc, "PM5544_MK10_YV12.raw", G2D_YV12);
    return;
  }
#endif

  fprintf(stderr, "\nTest complete!\n");
  exit(0);
}
