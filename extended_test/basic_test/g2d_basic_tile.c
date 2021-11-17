/*
 * Copyright 2020 - 2021 NXP
 * Copyright 2013-2014 Freescale Semiconductor, Inc.
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/*
 * g2d_basic_tile.c
 */

#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/time.h>

#include <g2dExt.h>

#define TEST_WIDTH 1920
#define TEST_HEIGHT 1080
#define TEST_LOOP 16
#define SIZE_1K 1024
#define SIZE_1M (1024 * 1024)

/*
 * Parses a string of the form "nv12-yu12".
 *
 * Returns true on success.
 */
static int parseFormat(const char *fmtStr, int *pSrcFmt, int *pDstFmt) {
  char srcFmt[16], dstFmt[16];
  int ret = 0;

  ret = sscanf(fmtStr, "%4s-%4s", srcFmt, dstFmt);
  if (2 != ret) {
    printf("FAILED to parse covert format, srcFmt=%s, dstFmt=%s, ret=%d\n",
           srcFmt, dstFmt, ret);
    return -EINVAL;
  }

  if (!strncmp(srcFmt, "i420", 4)) {
    *pSrcFmt = G2D_I420;
  } else if (!strncmp(srcFmt, "rgba", 4)) {
    *pSrcFmt = G2D_RGBA8888;
  } else if (0 == strncmp(srcFmt, "nv12", 4)) {
    *pSrcFmt = G2D_NV12;
  } else {
    printf("unknown srcFmt=%s\n", srcFmt);
    return -EINVAL;
  }

  if (0 == strncmp(dstFmt, "nv12", 4)) {
    *pDstFmt = G2D_NV12;
  } else if (!strncmp(dstFmt, "rgba", 4)) {
    *pDstFmt = G2D_RGBA8888;
  } else if (!strncmp(dstFmt, "rgb565", 6)) {
    *pDstFmt = G2D_RGB565;
  } else {
    printf("unknown dstFmt=%s\n", dstFmt);
    return -EINVAL;
  }

  return 0;
}

static const struct option longOptions[] = {
    {"help", no_argument, NULL, 'h'},
    {"verbose", no_argument, NULL, 'v'},
    {"source", required_argument, NULL, 's'},
    {"format", required_argument, NULL, 'f'},
    {NULL, 0, NULL, 0}};

int main(int argc, char *argv[]) {
  int i, j, diff = 0;
  struct timeval tv1, tv2;
  int g2d_feature_available = 0;
  int test_width = 0, test_height = 0;
  void *handle = NULL;
  struct g2d_surface src, dst;
  struct g2d_buf *s_buf, *d_buf;
  void *v_buf1, *v_buf2;
  int srcFmt = G2D_RGBA8888;
  int dstFmt = G2D_RGBA8888;
  struct g2d_surfaceEx srcEx, dstEx;

  if (g2d_open(&handle)) {
    printf("g2d_open fail.\n");
    return -ENOTTY;
  }

  while (1) {
    int optionIndex;
    int ic = getopt_long(argc, argv, "hvs:f:1", longOptions, &optionIndex);
    if (ic == -1) {
      break;
    }

    switch (ic) {
    case 'v':
    case 'h':
      fprintf(stdout, "usage: %s -s widthxheight -f sourceformat-destformat",
              argv[0]);
      return 0;
      break;

    case 's':
      if (2 != sscanf(optarg, "%dx%d", &test_width, &test_height)) {
        fprintf(stderr, "Invalid size '%s', must be \"w x h\"\n", optarg);
        return -EINVAL;
      }
      printf("sourceformat-destformat: %s\n", optarg);
      break;

    case 'f':
      if (0 != parseFormat(optarg, &srcFmt, &dstFmt)) {
        fprintf(stderr,
                "Invalid format '%s', must be src-dst\n"
                "src and dst format in lower case, src refered below\n"
                "    i420:G2D_I420 \n"
                "    nv12:G2D_NV12 \n"
                "    rgba:G2D_RGBA8888 \n"
                "dst refered below\n"
                "    rgba:G2D_RGBA8888 \n"
                "    rgb565:G2D_RGB565 \n",
                optarg);
        return -EINVAL;
      }
      break;

    default:
      if (ic != '?') {
        fprintf(stderr, "unexpected value 0x%x\n", ic);
      }
      return -EINVAL;
    }
  }

  if (0 >= test_width)
    test_width = TEST_WIDTH;
  if (0 >= test_height)
    test_height = TEST_HEIGHT;

  test_width = (test_width + 15) & ~15;
  test_height = (test_height + 15) & ~15;

  printf("Width %d, Height %d\n", test_width, test_height);

  s_buf = g2d_alloc(test_width * test_height * 4, 0);
  d_buf = g2d_alloc(test_width * test_height * 4, 0);

  src.format = srcFmt;
  dst.format = dstFmt;

  printf("---------------- g2d blit super-tiling cropping ----------------\n");
  src.planes[0] = s_buf->buf_paddr;
  src.planes[1] = s_buf->buf_paddr + test_width * test_height;
  src.planes[2] = s_buf->buf_paddr + test_width * test_height * 2;
  src.left = 743;
  src.top = 352;
  src.right = src.left + 16;
  src.bottom = src.top + 1;
  src.stride = test_width;
  src.width = test_width;
  src.height = test_height;
  src.rot = G2D_ROTATION_0;

  dst.planes[0] = d_buf->buf_paddr;
  dst.planes[1] = d_buf->buf_paddr + test_width * test_height;
  dst.planes[2] = d_buf->buf_paddr + test_width * test_height * 2;
  dst.left = 743;
  dst.top = 352;
  dst.right = dst.left + 16;
  dst.bottom = dst.top + 1;
  dst.stride = test_width;
  dst.width = test_width;
  dst.height = test_height;
  dst.rot = G2D_ROTATION_0;

  srcEx.base = src;
  srcEx.tiling = G2D_SUPERTILED;
  dstEx.base = dst;
  dstEx.tiling = G2D_LINEAR;

  for (i = 0; i < TEST_LOOP * 100; i++) {
    g2d_blitEx(handle, &srcEx, &dstEx);
    srcEx.base.left = (743 + i) % 64;
    srcEx.base.top = (352 + i) % 64;
    srcEx.base.right = srcEx.base.left + 16;
    srcEx.base.bottom = srcEx.base.top + 1;
  }

  g2d_finish(handle);

  /****************************************** test g2d_blit
   * *********************************************************/
  printf(
      "---------------- g2d blit super-tiling performance ----------------\n");

  src.left = 0;
  src.top = 0;
  src.right = test_width;
  src.bottom = test_height;

  dst.left = 0;
  dst.top = 0;
  dst.right = test_width;
  dst.bottom = test_height;

  srcEx.base = src;
  dstEx.base = dst;

  gettimeofday(&tv1, NULL);

  for (i = 0; i < TEST_LOOP; i++) {
    g2d_blitEx(handle, &srcEx, &dstEx);
  }

  g2d_finish(handle);

  gettimeofday(&tv2, NULL);
  diff = ((tv2.tv_sec - tv1.tv_sec) * 1000000 + (tv2.tv_usec - tv1.tv_usec)) /
         TEST_LOOP;

  printf("g2d tiling blit time %dus, %dfps, %dMpixel/s ........\n", diff,
         1000000 / diff, test_width * test_height / diff);

#if G2D_OPENCL
  srcEx.base.format = G2D_NV12;
  dstEx.base.format = G2D_NV12;
  srcEx.tiling = G2D_AMPHION_TILED;
  srcEx.base.stride = (test_width + 511) & ~511;

  for (i = 0; i < test_height; i++) {
    for (j = 0; j < test_width; j++) {
      char *p = (char *)(((char *)s_buf->buf_vaddr) + (i * test_width + j) * 4);
      p[0] = p[1] = p[2] = p[3] = (i * test_width + j) % 255;

      p = (char *)(((char *)d_buf->buf_vaddr) + (i * test_width + j) * 4);
      p[0] = p[1] = p[2] = p[3] = ((i * test_width + j + 128) % 255);
    }
  }

  g2d_blitEx(handle, &srcEx, &dstEx);

  g2d_finish(handle);

  for (i = 0; i < test_height; i++) {
    for (j = 0; j < test_width; j++) {
      char *d = (char *)(((char *)d_buf->buf_vaddr) + (i * test_width + j));
      int vtile = i / 128;
      int htile = j / 8;
      int x_in_tile = j & 7;
      int y_in_tile = i & 127;
      int offset = (htile * 128 * 8) + vtile * srcEx.base.stride * 128 +
                   y_in_tile * 8 + x_in_tile;
      char *s = (char *)(((char *)s_buf->buf_vaddr) + offset);

      if (s[0] != d[0]) {
        printf("opencl amphion Y deting failed at (%d,%d) src (%d, offset %d), "
               "dst(%d)!\n",
               i, j, s[0], offset, d[0]);
        break;
      }
    }
  }

  for (i = 0; i < test_height / 2; i++) {
    for (j = 0; j < test_width; j++) {
      char *d = (char *)(((char *)d_buf->buf_vaddr) + test_width * test_height +
                         (i * test_width + j));
      int vtile = i / 128;
      int htile = j / 8;
      int x_in_tile = j & 7;
      int y_in_tile = i & 127;
      int offset = (htile * 128 * 8) + vtile * srcEx.base.stride * 128 +
                   y_in_tile * 8 + x_in_tile;
      char *s = (char *)(((char *)s_buf->buf_vaddr) + test_width * test_height +
                         offset);

      if (s[0] != d[0]) {
        printf("opencl amphion UV deting failed at (%d,%d) src (%d, offset "
               "%d), dst(%d)!\n",
               i, j, s[0], offset, d[0]);
        break;
      }
    }
  }

  printf("---------------- amphion tile2linear performance ----------------\n");
  gettimeofday(&tv1, NULL);

  for (i = 0; i < TEST_LOOP; i++) {
    g2d_blitEx(handle, &srcEx, &dstEx);
  }

  g2d_finish(handle);

  gettimeofday(&tv2, NULL);
  diff = ((tv2.tv_sec - tv1.tv_sec) * 1000000 + (tv2.tv_usec - tv1.tv_usec)) /
         TEST_LOOP;

  printf("g2d amphion tile2linear %dus, %dfps, %dMpixel/s ........\n", diff,
         1000000 / diff, test_width * test_height / diff);
#endif

  g2d_free(s_buf);
  g2d_free(d_buf);

  g2d_close(handle);

  return 0;
}
