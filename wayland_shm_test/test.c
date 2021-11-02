#include "test_context.h"
#include <stdint.h>
#include <stdio.h>

#include <g2dExt.h>

static void ebu_color_bands(uint32_t *frame, unsigned int width,
                            unsigned int height) {
  const uint32_t BAR_COLOUR[8] = {
      0x00FFFFFF, // 100% White
      0x00FFFF00, // Yellow
      0x0000FFFF, // Cyan
      0x0000FF00, // Green
      0x00FF00FF, // Magenta
      0x00FF0000, // Red
      0x000000FF, // Blue
      0x00000000, // Black
  };

  unsigned columnWidth = width / 8;

  for (unsigned y = 0; y < height; y++) {
    for (unsigned x = 0; x < width; x++) {
      unsigned col_idx = x / columnWidth;
      frame[y * width + x] = BAR_COLOUR[col_idx];
    }
  }
}
static void g2d_fill_buffer(test_context *tc) {
  struct g2d_surfaceEx srcEx, dstEx;
  struct g2d_surface *src = &srcEx.base;
  struct g2d_surface *dst = &dstEx.base;
  struct g2d_buf *buf = NULL;

  void *g2dHandle;

  if (g2d_open(&g2dHandle) == -1 || g2dHandle == NULL) {
    printf("Fail to open g2d device!\n");
    return;
  }

  unsigned matrix[] = {
      0xFF, 0x0, 0x0,  0x0, 0x0, 0xFF, 0x0, 0x0,
      0x0,  0x0, 0xFF, 0x0, 0x0, 0x0,  0x0, 0xFF,
  };
  g2d_set_csc_matrix(g2dHandle, matrix);

  buf = g2d_alloc(tc->width * tc->height * 4, 0);
  src->planes[0] = (int)buf->buf_paddr;
  src->left = 0;
  src->top = 0;
  src->right = tc->width;
  src->width = tc->width;
  src->stride = tc->width;
  src->bottom = tc->height / 3;
  src->height = tc->height / 3;
  src->rot = G2D_ROTATION_0;
  src->format = G2D_RGBA8888;
  src->blendfunc = G2D_ONE;
  srcEx.tiling = G2D_LINEAR;

  ebu_color_bands(buf->buf_vaddr, tc->width, tc->height / 3);

  dst->planes[0] = tc->phy_data;
  dst->left = 0;
  dst->top = 0;
  dst->right = tc->width;
  dst->stride = tc->width;
  dst->width = tc->width;
  dst->bottom = tc->height / 2;
  dst->height = tc->height / 2;
  dst->rot = G2D_ROTATION_0;
  dst->clrcolor = 0xFF00FF00;
  dst->format = G2D_RGBA8888;
  dst->blendfunc = G2D_ONE_MINUS_SRC_ALPHA;
  dstEx.tiling = G2D_LINEAR;

  g2d_blitEx(g2dHandle, &srcEx, &dstEx);
  g2d_finish(g2dHandle);

  g2d_free(buf);
  g2d_close(g2dHandle);
}

void paint_pixels(test_context *tc) {
  int n;
  uint32_t *pixel = tc->shm_data;
  static uint32_t pixel_value = 0x0; // black

  for (n = 0; n < tc->width * tc->height; n++) {
    *pixel++ = pixel_value;
  }

  // increase each RGB component by one
  pixel_value += 0x10101;

  // if it's reached 0xffffff (white) reset to zero
  if (pixel_value > 0xffffff) {
    pixel_value = 0x0;
  }
  g2d_fill_buffer(tc);
}
