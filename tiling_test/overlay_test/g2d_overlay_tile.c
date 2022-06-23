/*
 * Copyright 2021 NXP
 * Copyright 2013-2014 Freescale Semiconductor, Inc.
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/*
 * g2d_overlay_tile.c
 */

#ifdef __cplusplus
extern "C" {
#endif

/*=======================================================================
                                        INCLUDE FILES
=======================================================================*/
/* Standard Include Files */
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <malloc.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include "gfx_init.h"
#include <g2dExt.h>

#define TFAIL -1
#define TPASS 0

#define CACHEABLE 0

int g_buf_size;
int g_buf_phys;

/**/
struct img_info {
  int img_left;
  int img_top;
  int img_right;
  int img_bottom;
  int img_width;
  int img_height;
  int img_rot;
  int img_size;
  int img_format;
  struct g2d_buf *img_ptr;
};

struct g2d_buf *createG2DTextureBuf(char *filename) {
  FILE *stream = NULL;
  int size = 0;
  struct g2d_buf *buf = NULL;
  int len = 0;

  stream = fopen(filename, "rb");
  if (!stream) {
    printf("Fail to open data file %s\n", filename);
    return NULL;
  }

  fseek(stream, 0, SEEK_END);
  size = ftell(stream);

  fseek(stream, 0L, SEEK_SET);

  // alloc physical contiguous memory for source image data
  buf = g2d_alloc(size, CACHEABLE);
  if (buf) {
    len = fread((void *)buf->buf_vaddr, 1, size, stream);
    if (len != size)
      printf("fread %s error\n", filename);

#if CACHEABLE
    g2d_cache_op(buf, G2D_CACHE_FLUSH);
#endif
  }

  fclose(stream);
  return buf;
}

void releaseG2DTextureBuf(struct g2d_buf *buf) { g2d_free(buf); }

/*
 * generate a color bar, NV12 format
 */
#define _YUV_COLOR_LIST_SIZE 5
static uint8_t y_color_list[_YUV_COLOR_LIST_SIZE] = {
    0xd2, 0xaa, 0x91, 0x6a, /*0x51,*/ 0x29,
};

static uint16_t uv_color_list[_YUV_COLOR_LIST_SIZE] = {
    0x9210, 0x10a6, 0x2236, 0xdeca, /*0xf05a,*/ 0x6ef0,
};

void fill_colorbar_y_uv(void *y_addr, void *uv_addr, int width, int height,
                        int colorbar_size) {
  uint32_t i = 0, j = 0;
  uint8_t *p_y = NULL;
  uint16_t *p_uv = NULL;

  colorbar_size = 64;

  p_y = (uint8_t *)y_addr;
  for (i = 0; i < height; i++) {
    for (j = 0; j < width; j++) {
      p_y[i * width + j] =
          y_color_list[(j / colorbar_size) % _YUV_COLOR_LIST_SIZE];
    }
  }
  p_uv = (uint16_t *)uv_addr;
  for (i = 0; i < height / 2; i++) {
    for (j = 0; j < width / 2; j++) {
      p_uv[i * width / 2 + j] =
          uv_color_list[(j / (colorbar_size / 2)) % _YUV_COLOR_LIST_SIZE];
    }
  }
}

void vpu_linear_to_tile_y_uv(void *src_addr, void *dst_addr, int width,
                             int height) {
  uint8_t *p_src_y = (uint8_t *)src_addr;
  uint8_t *p_dst_y = (uint8_t *)dst_addr;
  uint16_t *p_src_uv = (uint16_t *)(src_addr + width * height);
  uint16_t *p_dst_uv = (uint16_t *)(dst_addr + width * height);
  uint32_t offset_y = 0;
  uint32_t offset_uv = 0;
  uint32_t x = 0, y = 0;

  for (y = 0; y < height; y++) {
    for (x = 0; x < width; x++) {
      offset_y =
          (x >> 3) * 8 * 128 + (x & 7) + (y >> 7) * 128 * width + (y & 127) * 8;
      *(p_dst_y + offset_y) = *(p_src_y + y * width + x);
    }
  }

  for (y = 0; y < (height / 2); y++) {
    for (x = 0; x < (width / 2); x++) {
      offset_uv = (x >> 2) * 4 * 128 + (x & 3) + (y >> 7) * 128 * (width / 2) +
                  (y & 127) * 4;
      *(p_dst_uv + offset_uv) = *(p_src_uv + y * (width / 2) + x);
    }
  }
}

static void draw_image_to_framebuffer(
    void *handle, struct g2d_buf *buf, int img_width, int img_height,
    int img_size, int img_format, enum g2d_tiling tiling,
    screeninfo_t *screen_info, int left, int top, int dst_width,
    int dst_height, int set_alpha, int rotation, int set_blur) {
  int i;
  struct g2d_surfaceEx srcEx, dstEx;
  struct g2d_surface *src = &srcEx.base;
  struct g2d_surface *dst = &dstEx.base;

  if (((left + dst_width) > (int)screen_info->xres) ||
      ((top + dst_height) > (int)screen_info->yres)) {
    printf("Bad display image dimensions!\n");
    return;
  }

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
  NV21/NV61: Y in planes [0], VU in planes [1], with 64bytes alignment,
  YUYV/YVYU/UYVY/VYUY: in planes[0], buffer address is with 16bytes alignment.
  */

  src->format = img_format;
  switch (src->format) {
  case G2D_RGB565:
  case G2D_RGBA8888:
  case G2D_RGBX8888:
  case G2D_BGRA8888:
  case G2D_BGRX8888:
  case G2D_BGR565:
  case G2D_YUYV:
  case G2D_UYVY:
    src->planes[0] = buf->buf_paddr;
    break;
  case G2D_NV12:
    src->planes[0] = buf->buf_paddr;
    src->planes[1] = buf->buf_paddr + img_width * img_height;
    break;
  case G2D_I420:
    src->planes[0] = buf->buf_paddr;
    src->planes[1] = buf->buf_paddr + img_width * img_height;
    src->planes[2] = src->planes[1] + img_width * img_height / 4;
    break;
  case G2D_NV16:
    src->planes[0] = buf->buf_paddr;
    src->planes[1] = buf->buf_paddr + img_width * img_height;
    break;
  default:
    printf("Unsupport source image format in the example code\n");
    return;
  }

  srcEx.tiling = tiling;
  dstEx.tiling = G2D_LINEAR;

  src->left = 0;
  src->top = 0;
  src->right = img_width;
  src->bottom = img_height;
  src->stride = img_width;
  src->width = img_width;
  src->height = img_height;
  src->rot = G2D_ROTATION_0;

  dst->planes[0] = g_buf_phys;
  dst->left = left;
  dst->top = top;
  dst->right = left + dst_width;
  dst->bottom = top + dst_height;
  dst->stride = screen_info->xres;
  dst->width = screen_info->xres;
  dst->height = screen_info->yres;
  dst->rot = rotation;
  dst->format =
      screen_info->bits_per_pixel == 16
          ? G2D_RGB565
          : (screen_info->red.offset == 0 ? G2D_RGBA8888 : G2D_BGRA8888);

  if (set_alpha) {
    src->blendfunc = G2D_ONE;
    dst->blendfunc = G2D_ONE_MINUS_SRC_ALPHA;

    src->global_alpha = 0x80;
    dst->global_alpha = 0xff;

    g2d_enable(handle, G2D_BLEND);
    g2d_enable(handle, G2D_GLOBAL_ALPHA);
  }

  if (set_blur) {
    g2d_enable(handle, G2D_BLUR);
  }

  g2d_blitEx(handle, &srcEx, &dstEx);
  g2d_finish(handle);

  if (set_alpha) {
    g2d_disable(handle, G2D_GLOBAL_ALPHA);
    g2d_disable(handle, G2D_BLEND);
  }

  if (set_blur) {
    g2d_disable(handle, G2D_BLUR);
  }
}

void clear_screen_with_g2d(void *handle, screeninfo_t *screen_info,
                           int color) {
  struct g2d_surface dst;

  dst.planes[0] = g_buf_phys;
  dst.left = 0;
  dst.top = 0;
  dst.right = screen_info->xres;
  dst.bottom = screen_info->yres;
  dst.stride = screen_info->xres;
  dst.width = screen_info->xres;
  dst.height = screen_info->yres;
  dst.rot = G2D_ROTATION_0;
  dst.clrcolor = color;
  dst.format =
      screen_info->bits_per_pixel == 16
          ? G2D_RGB565
          : (screen_info->red.offset == 0 ? G2D_RGBA8888 : G2D_BGRA8888);

  g2d_clear(handle, &dst);
  g2d_finish(handle);
}

void Test_colorbar_vpu_tiled_to_linear(void *handle, int width, int height,
                                       enum g2d_tiling tiling,
                                       screeninfo_t *screen_info) {
  struct g2d_surfaceEx srcEx, dstEx;
  struct g2d_surface *src = &srcEx.base;
  struct g2d_surface *dst = &dstEx.base;
  struct g2d_buf *yuv_linear_buf = NULL;
  struct g2d_buf *yuv_vpu_tiled_buf = NULL;

  yuv_linear_buf = g2d_alloc(width * height * 2, 0);
  yuv_vpu_tiled_buf = g2d_alloc(width * height * 2, 0);

  if (!yuv_linear_buf || !yuv_vpu_tiled_buf) {
    printf("Fail to allocate physical memory !\n");
    goto OnError;
  }

  fill_colorbar_y_uv(yuv_linear_buf->buf_vaddr,
                     yuv_linear_buf->buf_vaddr + width * height, width, height,
                     64);

  if (tiling == G2D_AMPHION_TILED)
    vpu_linear_to_tile_y_uv(yuv_linear_buf->buf_vaddr,
                            yuv_vpu_tiled_buf->buf_vaddr, width, height);
  else if (tiling == G2D_LINEAR)
    memcpy(yuv_vpu_tiled_buf->buf_vaddr, yuv_linear_buf->buf_vaddr,
           width * height * 3 / 2);

  srcEx.tiling = tiling;
  dstEx.tiling = G2D_LINEAR;

  src->format = G2D_NV12;
  src->planes[0] = yuv_vpu_tiled_buf->buf_paddr;
  src->planes[1] = yuv_vpu_tiled_buf->buf_paddr + width * height;

  src->left = 0;
  src->top = 0;
  src->right = width;
  src->bottom = height;
  src->stride = width;
  src->width = width;
  src->height = height;
  src->rot = G2D_ROTATION_0;

  dst->planes[0] = g_buf_phys;
  dst->left = 0;
  dst->top = 0;
  dst->right = width;
  dst->bottom = height;
  dst->stride = screen_info->xres_virtual;
  dst->width = screen_info->xres_virtual;
  dst->height = screen_info->yres;
  dst->rot = G2D_ROTATION_0;
  dst->format =
      screen_info->bits_per_pixel == 16
          ? G2D_RGB565
          : (screen_info->red.offset == 0 ? G2D_RGBA8888 : G2D_BGRA8888);

  g2d_blitEx(handle, &srcEx, &dstEx);
  g2d_finish(handle);

OnError:
  if (yuv_linear_buf)
    g2d_free(yuv_linear_buf);
  if (yuv_vpu_tiled_buf)
    g2d_free(yuv_vpu_tiled_buf);
}

static void Test_image_vpu_tiled_to_linear(void *handle, struct g2d_buf *buf,
                                           int width, int height,
                                           enum g2d_tiling tiling,
                                           screeninfo_t *screen_info) {
  draw_image_to_framebuffer(handle, buf, width, height, width * height * 3 / 2,
                            G2D_NV12, tiling, screen_info, 0, 0, width, height,
                            0, G2D_ROTATION_0, 0);
}

static void Test_vpu_tiled_to_linear(void *handle, struct g2d_buf *linear_buf,
                                     struct g2d_buf *tiled_buf,
                                     screeninfo_t *screen_info) {
  printf("\nTest_vpu_tiled_to_linear\n");

  printf("Test_colorbar_vpu_linear_to_linear ...\n");
  Test_colorbar_vpu_tiled_to_linear(handle, 1024, 768, G2D_LINEAR, screen_info);

  graphics_update(screen_info);

  clear_screen_with_g2d(handle, screen_info, 0xffffffff);

  printf("Test_colorbar_vpu_tiled_to_linear ...\n");
  Test_colorbar_vpu_tiled_to_linear(handle, 1024, 768, G2D_AMPHION_TILED,
                                    screen_info);

  graphics_update(screen_info);

  clear_screen_with_g2d(handle, screen_info, 0xffffffff);

  printf("Test_image_vpu_linear_to_linear ...\n");
  Test_image_vpu_tiled_to_linear(handle, linear_buf, 1024, 768, G2D_LINEAR,
                                 screen_info);

  graphics_update(screen_info);

  clear_screen_with_g2d(handle, screen_info, 0xffffffff);

  printf("Test_image_vpu_tiled_to_linear ...\n");
  vpu_linear_to_tile_y_uv(linear_buf->buf_vaddr, tiled_buf->buf_vaddr, 1024,
                          768);
  Test_image_vpu_tiled_to_linear(handle, tiled_buf, 1024, 768,
                                 G2D_AMPHION_TILED, screen_info);

  graphics_update(screen_info);
}

static void Test_image_gpu_tiled_to_linear_with_crop(
    void *handle, struct g2d_buf *buf, int width, int height, int left, int top,
    int right, int bottom, enum g2d_format format, enum g2d_tiling tiling,
    int img_size, screeninfo_t *screen_info) {
  struct g2d_surfaceEx srcEx, dstEx;
  struct g2d_surface *src = &srcEx.base;
  struct g2d_surface *dst = &dstEx.base;

  srcEx.tiling = tiling;
  dstEx.tiling = G2D_LINEAR;

  src->format = format;
  src->planes[0] = buf->buf_paddr;

  src->left = 0;
  src->top = 0;
  src->right = width;
  src->bottom = height;
  src->stride = width;
  src->width = width;
  src->height = height;
  src->rot = G2D_ROTATION_0;

  dst->planes[0] = g_buf_phys;
  dst->left = 0;
  dst->top = 0;
  dst->right = width;
  dst->bottom = height;
  dst->stride = screen_info->xres_virtual;
  dst->width = screen_info->xres_virtual;
  dst->height = screen_info->yres;
  dst->rot = G2D_ROTATION_0;
  dst->format =
      screen_info->bits_per_pixel == 16
          ? G2D_RGB565
          : (screen_info->red.offset == 0 ? G2D_RGBA8888 : G2D_BGRA8888);

  g2d_set_clipping(handle, left, top, right, bottom);

  g2d_blitEx(handle, &srcEx, &dstEx);
  g2d_finish(handle);
}

static void Test_gpu_tiled_to_linear(void *handle, struct g2d_buf *buf,
                                     screeninfo_t *screen_info) {
  printf("\nTest_gpu_tiled_to_linear\n");

  Test_image_gpu_tiled_to_linear_with_crop(
      handle, buf, 1024, 768, 1024 / 2, 768 / 2, 1024, 768, G2D_RGB565,
      G2D_LINEAR, 1024 * 768 * 2, screen_info);
}

int main(int argc, char **argv) {
  int retval = TPASS;
  screeninfo_t screen_info;
  graphics_handler_t handler;
  struct timeval tv1, tv2;
  void *g2dHandle = NULL;
  struct g2d_buf *g2dDataBuf[8] = {NULL};
  int g2d_feature_available = 0;
  int src_file_available = 0;

  if (init_graphics(&handler, &screen_info, &g_buf_phys, &g_buf_size) != 0)
    return TFAIL;

  if (g2d_open(&g2dHandle) == -1 || g2dHandle == NULL) {
    printf("Fail to open g2d device!\n");
    goto deinit;
  }

  clear_screen_with_g2d(g2dHandle, &screen_info, 0xff000000);
  // TODO parse the para from filename or an xml which descript the blit.
  g2dDataBuf[0] = createG2DTextureBuf("1024x768-rgb565.rgb");
  if (!g2dDataBuf[0])
    goto no_file;

  g2dDataBuf[1] = createG2DTextureBuf("PM5544_MK10_NV12.raw");
  if (!g2dDataBuf[1])
    goto no_file;

  g2dDataBuf[2] = createG2DTextureBuf("PM5544_MK10_NV12.raw");
  if (!g2dDataBuf[2])
    goto no_file;

  src_file_available = 1;

  /* Test vpu tiled to linear */
  Test_vpu_tiled_to_linear(g2dHandle, g2dDataBuf[1], g2dDataBuf[2],
                           &screen_info);

  clear_screen_with_g2d(g2dHandle, &screen_info, 0xff000000);

  /* Test gpu tiled to linear */
  Test_gpu_tiled_to_linear(g2dHandle, g2dDataBuf[0], &screen_info);

  graphics_update(&screen_info);

no_file:
  if (!src_file_available) {
    printf("prepare the jpg file, and create with below cmd\n"
           "\tffmpeg -i 1024x768.jpg -pix_fmt rgb565le 1024x768-rgb565.rgb\n"
           "\tffmpeg -i 800x600.jpg -pix_fmt bgr565le 800x600-bgr565.rgb\n"
           "\tffmpeg -i 480x360.jpg -pix_fmt bgr565le 480x360-bgr565.rgb\n"
           "\tffmpeg -i 352x288.jpg -pix_fmt yuyv422 352x288-yuyv.yuv \n"
           "\tffmpeg -i 176x144.jpg -pix_fmt yuv420p 176x144-yuv420p.yuv\n"
           "\tgst-launch-1.0 videotestsrc num-buffers=1 ! \\\n"
           "\t\tvideo/x-raw,format=NV16,width=352,height=288 ! \\\n"
           "\t\tfilesink location=352x288-nv16.yuv\n");
    retval = -EINVAL;
  }

  for (int i = 0; i < sizeof(g2dDataBuf) / sizeof(g2dDataBuf[0]); i++) {
    if (g2dDataBuf[i]) {
      releaseG2DTextureBuf(g2dDataBuf[i]);
      g2dDataBuf[i] = NULL;
    }
  }

  if (!g2dHandle)
    g2d_close(g2dHandle);

deinit:
  deinit_graphics(&handler);

  return retval;
}
