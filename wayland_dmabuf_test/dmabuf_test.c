/*
 * Copyright 2021 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/*
 * dmabuf_test.c
 */

#include <assert.h>
#include <limits.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "Needle3Scaled.c"
#include "linux_dmabuf_wp.h"
#include "test_context.h"
#include <g2dExt.h>

struct client_buffer {
  struct wl_buffer *wlbuffer;
  int busy;

  struct g2d_buf *g2d_data;
  int dmabuf_fd;

  int width;
  int height;
  int bpp;
  unsigned long stride;
};
enum QMN_FORMAT {
  Q12_4,
  Q8_4,
  Q4_4,
  Q4,

  Q_X2_4,
  Q_X3_4,
};

struct qmn_format_specs {
  int i_bits;
  int f_bits;
};

struct qmn_format_specs something[] = {
    [Q12_4] = {12, 4},
    [Q8_4] = {8, 4},
    [Q4_4] = {4, 4},
    [Q4] = {0, 4},
};
#define SIGN_MASK 0x80000000UL

typedef int32_t fix16x16_t;
static const fix16x16_t fix16x16_overflow = 0x80000000;
static const fix16x16_t fix16x16_scaling_factor =
    0x00010000; /*!< fix16x16_t value of 1 (unit) */
static inline float fix16x16_to_float(fix16x16_t a) {
  return (float)a / fix16x16_scaling_factor;
}
static inline double fix16x16_to_dbl(fix16x16_t a) {
  return (double)a / fix16x16_scaling_factor;
}
static inline fix16x16_t int_to_fix16x16(int a) {
  return a * fix16x16_scaling_factor;
}
//#define FIXMATH_NO_ROUNDING 1

typedef int16_t fix12x4_t;
static const fix12x4_t fix12x4_scaling_factor =
    0x00000010; /*!< fix12x4_t value of 1 (unit) */
static inline float fix12x4_to_float(fix12x4_t a) {
  return (float)a / fix12x4_scaling_factor;
}
static inline double fix12x4_to_dbl(fix12x4_t a) {
  return (double)a / fix12x4_scaling_factor;
}
static inline fix12x4_t int_to_fix12x4(int a) {
  return a * fix12x4_scaling_factor;
}
static const fix12x4_t fix12x4_overflow = 0x8000;

fix16x16_t fix16x16_sub(fix16x16_t a, fix16x16_t b) {
  uint32_t _a = a, _b = b;
  uint32_t diff = _a - _b;

  // Overflow can only happen if sign of a != sign of b, and then
  // it causes sign of diff != sign of a.
  if (((_a ^ _b) & SIGN_MASK) && ((_a ^ diff) & SIGN_MASK))
    return fix16x16_overflow;

  return diff;
}
fix12x4_t fix12x4_sub(fix12x4_t a, fix12x4_t b) {
  uint32_t _a = a, _b = b;
  uint32_t diff = _a - _b;

  // Overflow can only happen if sign of a != sign of b, and then
  // it causes sign of diff != sign of a.
  if (((_a ^ _b) & SIGN_MASK) && ((_a ^ diff) & SIGN_MASK))
    return fix12x4_overflow;

  return diff;
}
fix12x4_t fix12x4_add(fix12x4_t a, fix12x4_t b) {
  // Use unsigned integers because overflow with signed integers is
  // an undefined operation (http://www.airs.com/blog/archives/120).
  uint32_t _a = a, _b = b;
  uint32_t sum = _a + _b;

  // Overflow can only happen if sign of a == sign of b, and then
  // it causes sign of sum != sign of a.
  if (!((_a ^ _b) & SIGN_MASK) && ((_a ^ sum) & SIGN_MASK))
    return fix12x4_overflow;
  fprintf(stderr, "SUM: %x+%x = %x\n", a, b, sum);
  return sum;
}
fix16x16_t fix16x16_add(fix16x16_t a, fix16x16_t b) {
  // Use unsigned integers because overflow with signed integers is
  // an undefined operation (http://www.airs.com/blog/archives/120).
  uint32_t _a = a, _b = b;
  uint32_t sum = _a + _b;

  // Overflow can only happen if sign of a == sign of b, and then
  // it causes sign of sum != sign of a.
  if (!((_a ^ _b) & SIGN_MASK) && ((_a ^ sum) & SIGN_MASK))
    return fix16x16_overflow;
  return sum;
}
fix12x4_t fix12x4_mul(fix12x4_t inArg0, fix12x4_t inArg1) {
  int64_t product = (int64_t)inArg0 * inArg1;

#ifndef FIXMATH_NO_OVERFLOW
  // The upper 17 bits should all be the same (the sign).
  uint32_t upper = (product >> (64 - 12));
#endif

  if (product < 0) {
#ifndef FIXMATH_NO_OVERFLOW
    if (~upper) {
      assert(0);
      return fix12x4_overflow;
    }
#endif

#ifndef FIXMATH_NO_ROUNDING
    // This adjustment is required in order to round -1/2 correctly
    product--;
#endif
  } else {
#ifndef FIXMATH_NO_OVERFLOW
    if (upper) {
      assert(0);
      return fix12x4_overflow;
    }
#endif
  }

#ifdef FIXMATH_NO_ROUNDING
  return product >> 4;
#else
  fix12x4_t result = product >> 4;
  result += (product & (1 << (4 - 1))) >> (4 - 1);
  return result;
#endif
}
fix16x16_t fix16x16_mul(fix16x16_t inArg0, fix16x16_t inArg1) {
  int64_t product = (int64_t)inArg0 * inArg1;

#ifndef FIXMATH_NO_OVERFLOW
  // The upper 17 bits should all be the same (the sign).
  uint32_t upper = (product >> 47);
#endif

  if (product < 0) {
#ifndef FIXMATH_NO_OVERFLOW
    if (~upper)
      return fix16x16_overflow;
#endif

#ifndef FIXMATH_NO_ROUNDING
    // This adjustment is required in order to round -1/2 correctly
    product--;
#endif
  } else {
#ifndef FIXMATH_NO_OVERFLOW
    if (upper)
      return fix16x16_overflow;
#endif
  }

#ifdef FIXMATH_NO_ROUNDING
  return product >> 16;
#else
  fix16x16_t result = product >> 16;
  result += (product & 0x8000) >> 15;

  return result;
#endif
}

static inline fix12x4_t float_to_fix12x4(float a) {
  float temp = a * fix12x4_scaling_factor;
#ifndef FIXMATH_NO_ROUNDING
  temp += (temp >= 0) ? 0.5f : -0.5f;
#endif
  return (fix12x4_t)temp;
}

static inline fix16x16_t float_to_fix16x16(float a) {
  float temp = a * fix16x16_scaling_factor;
#ifndef FIXMATH_NO_ROUNDING
  temp += (temp >= 0) ? 0.5f : -0.5f;
#endif
  return (fix16x16_t)temp;
}

////////////////////////////////////////////////
////////////////////////////////////////////////
////////////////////////////////////////////////
////////////////////////////////////////////////

struct test_data {
  void *g2d_handle;
  struct g2d_buf *src_layer;
  struct g2d_buf *coord_layer;
  struct coordinate_buffer *coord_buf;
  uint16_t *coord_x;
  uint16_t *coord_y;
};

struct coordinate_buffer {
  uint8_t *data;
  size_t size;
  size_t width;
  size_t height;
  enum QMN_FORMAT format;
};

struct coordinate_buffer *coordinate_buffer_new(int format, int width,
                                                int height) {
  struct coordinate_buffer *rv =
      (struct coordinate_buffer *)calloc(1, sizeof(struct coordinate_buffer));

  rv->width = width;
  rv->height = height;
  rv->format = format;

  return rv;
}

void coordinate_buffer_alloc(struct coordinate_buffer *buf) {

  int bits_per_coord =
      something[buf->format].i_bits + something[buf->format].f_bits;
  buf->size = (buf->width * buf->height * bits_per_coord * 2) / CHAR_BIT;
  buf->data = (uint8_t *)calloc(1, buf->size);
}

void coordinate_buffer_free(struct coordinate_buffer *buf) {
  free(buf->data);
  buf->size = 0;
}

////////////////////////////////////////////////
static inline void coordinate_buffer_set_xy(struct coordinate_buffer *buf,
                                            int x, int y, float sample_x,
                                            float sample_y) {
  fix12x4_t coord_x = float_to_fix12x4(sample_x);
  fix12x4_t coord_y = float_to_fix12x4(sample_y);

#warning the "data" pointer type is dependent on the Bits-per-coord
  uint32_t *data = (uint32_t *)buf->data;
  data[y * buf->width + x] = (coord_x << 16) | coord_y;
}
static inline void coordinate_buffer_set_fix12x4(struct coordinate_buffer *buf,
                                                 int x, int y,
                                                 fix12x4_t sample_x,
                                                 fix12x4_t sample_y) {
#warning the "data" pointer type is dependent on the Bits-per-coord
  uint32_t *data = (uint32_t *)buf->data;
  data[y * buf->width + x] = (sample_x << 16) | sample_y;
}

static void coordinate_buffer_generate_xy(struct coordinate_buffer *buf,
                                          int origin_x, int origin_y,
                                          uint16_t *out_x, uint16_t *out_y) {
  for (int img_y = 0; img_y < buf->height; ++img_y) {
    int cartesian_y = origin_y - img_y;
    for (int img_x = 0; img_x < buf->width; ++img_x) {
      int cartesian_x = img_x - origin_x;
      out_x[img_y * buf->width + img_x] = int_to_fix12x4(cartesian_x);
      out_y[img_y * buf->width + img_x] = int_to_fix12x4(cartesian_y);
    }
  }
}

#include <arm_neon.h>
void matrix_mul_fixed() {
  float degree = 60;

  float rad_phi = (degree * 3.14159f) / 180.0f;
  float cos_phi = cosf(rad_phi);

  fix12x4_t fix_cos = float_to_fix12x4(cos_phi);

  fix12x4_t x1[] = {0x00, 0x10, 0x20, 0x00, 0x10, 0x20, 0x00, 0x10, 0x20};

  uint16_t o1[8];

  int16x8_t x = vld1q_s16(x1);
  int16x8_t r = vmulq_n_s16(x, fix_cos);
  r = vshrq_n_s16(r, 4); // Q12.4 fixed-point format
  vst1q_s16(o1, r);

  for (int i = 0; i < 8; ++i) {
    fprintf(stderr, "%x ", o1[i]);
  }
  fprintf(stderr, "\n");

  //    vld1.16     {d16-d19}, [r1]          @ load sixteen elements of matrix 0
  //    vld1.16     {d0-d3}, [r2]            @ load sixteen elements of matrix 1

  //    vmull.s16   q12, d16, \col_d[0]         @ multiply col element 0 by
  //    matrix col 0 vmlal.s16   q12, d17, \col_d[1]         @ multiply-acc col
  //    element 1 by matrix col 1 vmlal.s16   q12, d18, \col_d[2]         @
  //    multiply-acc col element 2 by matrix col 2 vmlal.s16   q12, d19,
  //    \col_d[3]         @ multiply-acc col element 3 by matrix col 3
  //    vqrshrn.s32 \res_d, q12, #14            @ shift right and narrow
  //    accumulator into
}

/*
(x,y)
0,0 1,0 2,0 3,0
0,1 1,1 2,1 3,1
0,2 1,2 2,2 3,2
0,3 1,3 2,3 3,3

(x)
0,1,2,3 ... 639,640
0,1,2,3 ... 639,640
0,1,2,3 ... 639,640

(y)
  0,  0,  0,  0 ...   0,  0
  1,  1,  1,  1 ...   1,  1
    ..........
479,479,479,479 ... 479,479
480,480,480,480 ... 480,480
*/

static void coordinate_buffer_rotate_ccw_neon(struct coordinate_buffer *buf,
                                              int16_t *coord_x,
                                              int16_t *coord_y, float degree) {
  float rad_phi = (degree * 3.14159f) / 180.0f;
  fix12x4_t fix_cos = float_to_fix12x4(cos(rad_phi));

  uint16_t o1[8];

  for (int i = 0; i < buf->width * buf->height; i += 8) {
    int16x8_t x = vld1q_s16(&coord_x[i]);
    int16x8_t r = vmulq_n_s16(x, fix_cos);
    r = vshrq_n_s16(r, 4); // Q12.4 fixed-point format
    vst1q_s16(o1, r);
  }

  fprintf(stderr, "z");
}

static void coordinate_buffer_fixed16_rotate_ccw(struct coordinate_buffer *buf,
                                                 int _origin_x, int _origin_y,
                                                 float degree) {
  {
    float rad_phi = (degree * 3.14159f) / 180.0f;
    fix16x16_t cos_phi = float_to_fix16x16(cosf(rad_phi));
    fix16x16_t sin_phi = float_to_fix16x16(sinf(rad_phi));
    fix16x16_t origin_x = int_to_fix16x16(_origin_x);
    fix16x16_t origin_y = int_to_fix16x16(_origin_y);

    fix16x16_t cartesian_y = origin_y;
    for (int img_y = 0; img_y < buf->height; ++img_y) {
      fix16x16_t cartesian_x = -origin_x;
      for (int img_x = 0; img_x < buf->width; ++img_x) {
        fix16x16_t sample_x = fix16x16_add(fix16x16_mul(cartesian_x, cos_phi),
                                           fix16x16_mul(cartesian_y, sin_phi));
        fix16x16_t sample_y = fix16x16_sub(fix16x16_mul(cartesian_y, cos_phi),
                                           fix16x16_mul(cartesian_x, sin_phi));
        fix12x4_t coord_x = fix16x16_add(origin_x, sample_x) >> 12;
        fix12x4_t coord_y = fix16x16_sub(origin_y, sample_y) >> 12;
        coordinate_buffer_set_fix12x4(buf, img_x, img_y, coord_x, coord_y);
        cartesian_x = fix16x16_add(cartesian_x, fix16x16_scaling_factor);
      }
      cartesian_y = fix16x16_sub(cartesian_y, fix16x16_scaling_factor);
    }
  }
}

static void coordinate_buffer_rotate_ccw(struct coordinate_buffer *buf,
                                         int origin_x, int origin_y,
                                         float degree) {
  float rad_phi = degree * 3.14159f / 180.0f;
  float cos_phi = cosf(rad_phi);
  float sin_phi = sinf(rad_phi);

  float cartesian_y = origin_y;
  for (int img_y = 0; img_y < buf->height; ++img_y) {
    float cartesian_x = -origin_x;
    for (int img_x = 0; img_x < buf->width; ++img_x) {
      float smpl_x = cartesian_x * cos_phi + cartesian_y * sin_phi;
      float smpl_y = cartesian_y * cos_phi - cartesian_x * sin_phi;
      coordinate_buffer_set_xy(buf, img_x, img_y, origin_x + smpl_x,
                               origin_y - smpl_y);
      cartesian_x += 1.0f;
    }
    cartesian_y -= 1.0f;
  }
}

static void coordinate_buffer_atan_rotate_ccw(struct coordinate_buffer *buf,
                                              int origin_x, int origin_y,
                                              float degree) {
  float rad_phi = (degree * 3.14159f) / 180.0f;
  float cos_phi = cosf(rad_phi);
  float sin_phi = sinf(rad_phi);

  int cartesian_y = origin_y;
  for (int img_y = 0; img_y < buf->height; ++img_y) {
    int cartesian_x = -origin_x;
    for (int img_x = 0; img_x < buf->width; ++img_x) {
      float alpha = rad_phi + atanf((1.0f * cartesian_y) / cartesian_x);
      float smpl_x = cos(alpha);
      float smpl_y = sin(alpha);
      coordinate_buffer_set_xy(buf, img_x, img_y, origin_x + smpl_x,
                               origin_y - smpl_y);
      cartesian_x += 1;
    }
    cartesian_y -= 1;
  }
}

static void coordinate_buffer_shear_rotate_ccw(struct coordinate_buffer *buf,
                                               int origin_x, int origin_y,
                                               float degree) {
  float rad_phi = (degree * 3.14159f) / 180.0f;

  float a, b, g;
  float smpl_x, smpl_y;
  a = g = -tan(rad_phi / 2);
  b = sin(rad_phi);
  float tan_ = tan(rad_phi);

  for (int img_y = 0; img_y < buf->height; ++img_y) {
    int cartesian_y = origin_y - img_y;
    float hskew = a * cartesian_y;
    for (int img_x = 0; img_x < buf->width; ++img_x) {
      int cartesian_x = img_x - origin_x;
      smpl_x = cartesian_x - hskew;
      smpl_y = cartesian_y - b * smpl_x;
      smpl_x = smpl_x - a * smpl_y;

      coordinate_buffer_set_xy(buf, img_x, img_y, origin_x + smpl_x,
                               origin_y - smpl_y);
    }
  }
}

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
      0x00505050, // Black
  };

  unsigned columnWidth = width / 8;

  for (unsigned y = 0; y < height; y++) {
    for (unsigned x = 0; x < width; x++) {
      unsigned col_idx = x / columnWidth;
      frame[y * width + x] = BAR_COLOUR[col_idx];
    }
  }
}

static void g2d_fill_buffer(test_context *tc,
                            struct client_buffer *client_buffer) {
  struct g2d_surfaceEx srcEx;
  struct g2d_surfaceEx secEx;
  struct g2d_surfaceEx dstEx;
  struct g2d_surface *src = &srcEx.base;
  struct g2d_surface *sec = &secEx.base;
  struct g2d_surface *dst = &dstEx.base;
  ////
  struct test_data *td = (struct test_data *)tc->user_data;
  struct g2d_buf *ebu_buf = td->src_layer;
  struct g2d_buf *coord_layer = td->coord_layer;
  struct coordinate_buffer *coord_buf = td->coord_buf;

  void *g2dHandle = td->g2d_handle;

  unsigned matrix[] = {
      0xFF, 0x0, 0x0,  0x0, 0x0, 0xFF, 0x0, 0x0,
      0x0,  0x0, 0xFF, 0x0, 0x0, 0x0,  0x0, 0xFF,
  };
  g2d_set_csc_matrix(g2dHandle, matrix);
  //  g2d_enable(g2dHandle, G2D_ARB_WARP);

  ////SRC
  src->planes[0] = (int)ebu_buf->buf_paddr;
  src->left = 0;
  src->top = 0;
  src->right = 240;  // tc->width;
  src->width = 240;  // tc->width;
  src->stride = 240; // tc->width;
  src->bottom = 240; // tc->height;
  src->height = 240; // tc->height;

  src->rot = G2D_ROTATION_0;
  src->format = G2D_BGRA8888;
  //    RGBA

  src->blendfunc = G2D_ONE;
  srcEx.tiling = G2D_LINEAR;

  ////SEC
  static float rot_degree = 1.0f;
  coord_buf->data = coord_layer->buf_vaddr;
  // coordinate_buffer_rotate_ccw_neon(coord_buf, td->coord_x, td->coord_y,
  // rot_degree);
  //    coordinate_buffer_rotate_ccw(coord_buf, 240/2, 240/2, rot_degree);
  //    coordinate_buffer_atan_rotate_ccw(coord_buf, 240/2, 240/2, rot_degree);
  //    coordinate_buffer_fixed16_rotate_ccw(coord_buf, 240/2, 240/2,
  //    rot_degree);
  coordinate_buffer_shear_rotate_ccw(coord_buf, 240 / 2, 240 / 2, rot_degree);

  rot_degree -= 10.0f;

  sec->planes[0] = (int)coord_layer->buf_paddr;
  sec->left = 0;
  sec->top = 0;
  sec->right = 240;  // tc->width;
  sec->width = 240;  // tc->width;
  sec->stride = 240; // tc->width;
  sec->bottom = 240; // tc->height;
  sec->height = 240; // tc->height;

  sec->rot = G2D_ROTATION_0;
  sec->format = G2D_RGBX8888;
  secEx.tiling = G2D_LINEAR;

  ////DST
  dst->planes[0] = client_buffer->g2d_data->buf_paddr;
  dst->left = 0;
  dst->top = 0;
  dst->right = tc->window_width;
  dst->stride = tc->window_width;
  dst->width = tc->window_width;
  dst->bottom = tc->window_height;
  dst->height = tc->window_height;
  dst->rot = G2D_ROTATION_0;
  dst->clrcolor = 0xFF00FF00;
  dst->format = G2D_RGBX8888;
  dst->blendfunc = G2D_ONE_MINUS_SRC_ALPHA;
  dstEx.tiling = G2D_LINEAR;

  g2d_blitEx(g2dHandle, &srcEx, &dstEx);
  //  g2d_two_blit(g2dHandle, src, sec, dst);
  g2d_finish(g2dHandle);
}
////////////////////////////////////////////////
void test_setup(test_context *tc) {
  struct test_data *td =
      (struct test_data *)calloc(1, sizeof(struct test_data));

  td->coord_buf = coordinate_buffer_new(Q12_4, 240 // tc->width
                                        ,
                                        240 // tc->height
  );

  if (g2d_open(&td->g2d_handle) == -1 || td->g2d_handle == NULL) {
    fprintf(stderr, "Fail to open g2d device!\n");
    return;
  }

  td->coord_layer = g2d_alloc(tc->window_width * tc->window_height * 4, 0);
  if (td->coord_layer == NULL) {
    fprintf(stderr, "unable to allocate Secondary client_buffer");
    return;
  }

  td->src_layer = g2d_alloc(tc->window_width * tc->window_height * 4, 0);
  if (td->src_layer == NULL) {
    fprintf(stderr, "unable to allocate Source client_buffer");
    return;
  }

  tc->user_data = (void *)td;

  struct g2d_buf *ebu_buf = td->src_layer;
  //    ebu_color_bands(ebu_buf->buf_vaddr, tc->width, tc->height);
  memcpy(ebu_buf->buf_vaddr, gimp_image.pixel_data, 240 * 240 * 4);

  //    td->coord_x = calloc(1, tc->width*tc->height*2);
  //    td->coord_y = calloc(1, tc->width*tc->height*2);
  // 1.2 MB
  //+ 1.2 the source image
  //+ 1.2 the coordinate layer
  //+ 1.2 the dest
  //    coordinate_buffer_generate_xy(td->coord_buf, tc->width/2,tc->height/2,
  //    td->coord_x, td->coord_y);
  fprintf(stderr, "test setup done\n");
}

void test_teardown(test_context *tc) {
  struct test_data *td = (struct test_data *)tc->user_data;

  free(td->coord_buf);
  free(td->coord_x);
  free(td->coord_y);
  g2d_free(td->coord_layer);
  g2d_free(td->src_layer);

  g2d_close(td->g2d_handle);

  free(tc->user_data);
  tc->user_data = NULL;
}

static int frames = 0;
static time_t time_prev = 0;

void test_paint_pixels(test_context *tc, struct client_buffer *client_buffer) {
  /*
  int n;
  uint32_t *pixel = tc->shm_data;
  static uint32_t pixel_value = 0x0; // black

  for (n =0; n < tc->width*tc->height; n++) {
      *pixel++ = pixel_value;
  }

  // increase each RGB component by one
  pixel_value += 0x10101;

  // if it's reached 0xffffff (white) reset to zero
  if (pixel_value > 0xffffff) {
      pixel_value = 0x0;
  }
  */
  g2d_fill_buffer(tc, client_buffer);
  ++frames;
  int diff = time(NULL) - time_prev;
  if (diff > 1) {
    fprintf(stderr, "%f: %d frames in %d seconds\n", 1.0f * frames / diff,
            frames, diff);
    frames = 0;
    time_prev = time(NULL);
  }
}
