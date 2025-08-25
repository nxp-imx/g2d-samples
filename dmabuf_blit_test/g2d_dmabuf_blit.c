/*
 * Copyright 2025 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/*
 * g2d_dmabuf_blit.c
 */

#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/time.h>

#include "g2d.h"
#include "g2dExt.h"

#define TEST_WIDTH 1920
#define TEST_HEIGHT 1080

int main(int argc, char *argv[]) {
  int test_loop = 16;
  int i, j, p, diff = 0;
  struct timeval tv1, tv2;
  void *handle = NULL;
  int g2d_hardware_avail = 0;
  int test_width = TEST_WIDTH;
  int test_height = TEST_HEIGHT;
  struct g2d_surface src, dst;
  struct g2d_buf *s_buf, *d_buf;
  struct g2d_surface_dmabuf src_dma, dst_dma;

  if (g2d_open(&handle)) {
    printf("g2d_open fail.\n");
    return -ENOTTY;
  }

  printf("Width %d, Height %d\n", test_width, test_height);

  s_buf = g2d_alloc(test_width * test_height * 4, 0);
  d_buf = g2d_alloc(test_width * test_height * 4, 0);

  src.left = 0;
  src.top = 0;
  src.right = test_width;
  src.bottom = test_height;
  src.stride = test_width;
  src.width = test_width;
  src.height = test_height;
  src.rot = G2D_ROTATION_0;
  src.format = G2D_RGBA8888;

  dst.left = 0;
  dst.top = 0;
  dst.right = test_width;
  dst.bottom = test_height;
  dst.stride = test_width;
  dst.width = test_width;
  dst.height = test_height;
  dst.rot = G2D_ROTATION_0;
  dst.format = G2D_RGBA8888;

  g2d_query_hardware(handle, G2D_HARDWARE_DPU_V2, &g2d_hardware_avail);
  if (g2d_hardware_avail == 1) {
    printf("---------------- g2d blit with dmabuf fd performance ----------------\n");
    src_dma.base = src;
    src_dma.plane_fd[0] = g2d_buf_export_fd(s_buf);
    src_dma.plane_offset[0] = 0;

    dst_dma.base = dst;
    dst_dma.plane_fd[0] = g2d_buf_export_fd(d_buf);
    dst_dma.plane_offset[0] = 0;

    memset(s_buf->buf_vaddr, 0xcc, test_width * test_height * 4);
    memset(d_buf->buf_vaddr, 0xaa, test_width * test_height * 4);

    gettimeofday(&tv1, NULL);

    for (i = 0; i < test_loop; i++) {
      g2d_blit_dmabuf(handle, &src_dma, &dst_dma);
    }

    g2d_finish(handle);

    gettimeofday(&tv2, NULL);
    diff = ((tv2.tv_sec - tv1.tv_sec) * 1000000 + (tv2.tv_usec - tv1.tv_usec)) /
         test_loop;
    printf("RGBA->RGBA with dmabuf fd time %dus, %dfps, %dMpixel/s ........\n", diff,
           1000000 / diff, test_width * test_height / diff);

    for (i = 0; i < test_height; i++) {
      for (j = 0; j < test_width; j++) {
        for (p = 0; p < 4; p++) {
          char *s = (char *)(((char *)s_buf->buf_vaddr) + (i * test_width + j) * 4 + p);
          char *d = (char *)(((char *)d_buf->buf_vaddr) + (i * test_width + j) * 4 + p);
          if (d[0] != s[0]) {
            printf("G2D_RGBA8888 to G2D_RGBA8888 with dmabuf fd is wrong at [%d,%d,%d] d = 0x%x (expect 0x%x)\n", i, j, p, d[0], s[0]);
          }
        }
      }
    }
  }

  g2d_free(s_buf);
  g2d_free(d_buf);

  g2d_close(handle);

  return 0;
}
