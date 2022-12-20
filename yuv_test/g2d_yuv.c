/*
 * Copyright 2021 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/*
 * g2d_yuv.c
 */

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <math.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

#include <g2dExt.h>

#define TRUE 1
#define FALSE 0
#define TEST_LOOP 16

#define CACHEABLE 0

#define Timespec_Double(t)                                                     \
  ((double)((t)->tv_sec) + (1.e-9 * (double)((t)->tv_nsec)))

#define Timespec_Sub(r, a, b)                                                  \
  do {                                                                         \
    if ((a)->tv_nsec < (b)->tv_nsec) {                                         \
      (r)->tv_nsec = 1000000000 + (a)->tv_nsec - (b)->tv_nsec;                 \
      (r)->tv_sec = (a)->tv_sec - (b)->tv_sec - 1;                             \
    } else {                                                                   \
      (r)->tv_nsec = (a)->tv_nsec - (b)->tv_nsec;                              \
      (r)->tv_sec = (a)->tv_sec - (b)->tv_sec;                                 \
    }                                                                          \
  } while (0)

static const struct option longOptions[] = {
    {"dest", optional_argument, NULL, 'd'},
    {"help", no_argument, NULL, 'h'},
    {"verbose", no_argument, NULL, 'v'},
    {"source", required_argument, NULL, 's'},
    {"format", required_argument, NULL, 'f'},
    {"infile", required_argument, NULL, 'i'},
    {"wh", required_argument, NULL, 'w'},
    {NULL, 0, NULL, 0}};

static void usage() {
  fprintf(stderr, "Usage: cmd [options] <filename>\n"
                  "\n"
                  "Options:\n"
                  "--format srcFmt-dstFmt\n"
                  "    source and dest format\n"
                  "    NV12:G2D_NV12 = 20\n"
                  "    YU12:G2D_I420 = 21\n"
                  "    YV12:G2D_YV12 = 22\n"
                  "    NV21:G2D_NV21 = 23\n"
                  "    YUYV:G2D_YUYV = 24\n"
                  "    YVYU:G2D_YVYU = 25\n"
                  "    UYVY:G2D_UYVY = 26\n"
                  "    VYUY:G2D_VYUY = 27\n"
                  "    NV16:G2D_NV16 = 28\n"
                  "    NV61:G2D_NV61 = 29\n"
                  "--source WIDTHxHEIGHT\n"
                  "    Set the source size, e.g. \"1280x720\" "
                  "--verbose\n"
                  "    Display interesting information on stdout.\n"
                  "--wh WIDTHxHEIGHT\n"
                  "    source width and height\n"
                  "--dest WIDTHxHEIGHT\n"
                  "    dest width and height\n"
                  "--help\n"
                  "    Show this message.\n"
                  "\n");
}

/*
 * Parses a string of the form "3840x2160".
 *
 * Returns 0 on success.
 */
static int parseWidthHeight(const char *widthHeight, int *pWidth,
                            int *pHeight) {
  char *end;

  if (2 != sscanf(widthHeight, "%dx%d", pWidth, pHeight)) {
    printf("widthxheight, both width and heigh are decimal\n");
    return -EINVAL;
  }

  return 0;
}

/*
 * Parses a string of the form "nv12-yu12".
 *
 * Returns true on success.
 */
static bool parseFormat(const char *fmtStr, int *pSrcFmt, int *pDstFmt) {
  char srcFmt[16], dstFmt[16];
  int ret = 0;

  ret = sscanf(fmtStr, "%4s-%4s", srcFmt, dstFmt);
  if (2 != ret) {
    printf("FAILED to parse covert format, srcFmt=%s, dstFmt=%s, ret=%d\n",
           srcFmt, dstFmt, ret);
    return false;
  }

  if (0 == strncmp(srcFmt, "nv12", 4))
    *pSrcFmt = G2D_NV12;
  else if (0 == strncmp(srcFmt, "yuyv", 4))
    *pSrcFmt = G2D_YUYV;
  else {
    printf("unknown srcFmt=%s\n", srcFmt);
    return false;
  }

  if (0 == strncmp(dstFmt, "nv12", 4))
    *pDstFmt = G2D_NV12;
  else if (0 == strncmp(dstFmt, "yuyv", 4))
    *pDstFmt = G2D_YUYV;
  else if (0 == strncmp(dstFmt, "yu12", 7))
    *pDstFmt = G2D_I420;
  else {
    printf("unknown dstFmt=%s\n", srcFmt);
    return false;
  }

  return true;
}
void *_g2d_handle;

int OpenG2D(void);
int CreateG2DBuffer(struct g2d_buf **buf, int size);
void ReleaseG2DBuffer(struct g2d_buf *buf);
void CloseG2D(void);

int main(int argc, char **argv) {
  FILE *fpin, *fpout;
  int srcStride = 0, dstStride = 0;
  int i;
  static struct timespec t1 = {0, 0};
  static struct timespec t2 = {0, 0};
  struct timespec diff;
  double t;
  int srcWidth = 0, srcHeight = 0, dstWidth = 0, dstHeight = 0;
  char *inFile;
  char *outFile = "output.yuv";
  int srcFmt = G2D_YUYV;
  int dstFmt = G2D_YUYV;
  size_t size_r = 0;
  int ret = 0;

  while (true) {
    int optionIndex;
    int ic =
        getopt_long(argc, argv, "hvw:d:s:f:i:1", longOptions, &optionIndex);
    if (ic == -1) {
      break;
    }

    switch (ic) {
    case 'v':
    case 'h':
      usage();
      return 0;
      break;
    case 's':
      if (0 != parseWidthHeight(optarg, &srcWidth, &srcHeight)) {
        fprintf(stderr, "Invalid size '%s', must be w x h\n", optarg);
        return -EINVAL;
      }
      if (srcWidth == 0 || srcHeight == 0) {
        fprintf(stderr, "Invalid size %ux%u, w and h may not be zero\n",
                srcWidth, srcHeight);
        return -EINVAL;
      }
      break;
    case 'd':
      if (0 != parseWidthHeight(optarg, &dstWidth, &dstHeight)) {
        fprintf(stderr, "Invalid size '%s', must be w x h\n", optarg);
        return -EINVAL;
      }
      if (dstWidth == 0 || dstHeight == 0) {
        fprintf(stderr, "Invalid size %ux%u, w and h may not be zero\n",
                dstWidth, dstHeight);
        return -EINVAL;
      }
      break;
    case 'w':
      if (0 != parseWidthHeight(optarg, &srcStride, &dstStride)) {
        fprintf(stderr, "Invalid size '%s', must be w x h\n", optarg);
        return -EINVAL;
      }
      if (srcStride == 0 || dstStride == 0) {
        fprintf(stderr, "Invalid size %ux%u, w and h may not be zero\n",
                srcStride, dstStride);
        return -EINVAL;
      }
      break;

    case 'f':
      if (!parseFormat(optarg, &srcFmt, &dstFmt)) {
        fprintf(stderr,
                "Invalid format '%s', must be src-dst\n"
                "    src and dst format in lower case, refered below\n"
                "    nv12:G2D_NV12 \n"
                "    yu12:G2D_I420 \n"
                "    yv12:G2D_YV12 \n"
                "    nv21:G2D_NV21 \n"
                "    yuyv:G2D_YUYV \n"
                "    yvyu:G2D_YVYU \n"
                "    uyvy:G2D_UYVY \n"
                "    vyuy:G2D_VYUY \n"
                "    nv16:G2D_NV16 \n"
                "    nv61:G2D_NV61 \n",
                optarg);
        return -EINVAL;
      }
      break;
    case 'i':
      inFile = optarg;
      break;
    default:
      if (ic != '?') {
        fprintf(stderr, "unexpected value 0x%x\n", ic);
      }
      return -EINVAL;
    }
  }

  // opens data files
  fpin = fopen(inFile, "rb");
  if (!fpin) {
    printf("FAILED to open source file\n");
    return -EACCES;
  }

  fpout = fopen(outFile, "wb");
  if (!fpout) {
    printf("FAILED to open dest file %s\n", outFile);
    return -EACCES;
  }
  printf("\nOpening source file %s OK, dest file %s OK\n", inFile, outFile);

  if (0 == srcStride)
    srcStride = srcWidth;
  if (0 == dstWidth)
    dstWidth = srcWidth;
  if (0 == dstHeight)
    dstHeight = srcHeight;
  if (0 == dstStride)
    dstStride = dstWidth;

  struct g2d_buf *srcYBuf = NULL;
  struct g2d_buf *srcUBuf = NULL;
  struct g2d_buf *srcVBuf = NULL;
  struct g2d_buf *dstYBuf = NULL;
  struct g2d_buf *dstUBuf = NULL;
  struct g2d_buf *dstVBuf = NULL;

  ret = OpenG2D();
  if (0 != ret) {
    return ret;
  }

  if (G2D_YUYV == srcFmt) {
    CreateG2DBuffer(&srcYBuf, srcWidth * 2 * srcHeight);
    size_r = fread(srcYBuf->buf_vaddr, 1, srcWidth * 2 * srcHeight, fpin);
  } else if (G2D_NV12 == srcFmt) {
    CreateG2DBuffer(&srcYBuf, srcStride * srcHeight);
    size_r = fread(srcYBuf->buf_vaddr, 1, srcWidth * srcHeight, fpin);
    CreateG2DBuffer(&srcUBuf, srcStride * srcHeight / 2);
    size_r = fread(srcUBuf->buf_vaddr, 1, srcWidth * srcHeight / 2, fpin);
    fseek(fpin, srcStride * srcHeight, SEEK_SET);
  } else if (G2D_I420 == srcFmt) {
    CreateG2DBuffer(&srcYBuf, srcStride * srcHeight);
    size_r = fread(srcYBuf->buf_vaddr, 1, srcWidth * srcHeight, fpin);
    CreateG2DBuffer(&srcUBuf, srcStride * srcHeight / 4);
    size_r = fread(srcUBuf->buf_vaddr, 1, srcWidth * srcHeight / 4, fpin);
    CreateG2DBuffer(&srcVBuf, srcStride * srcHeight / 4);
    size_r = fread(srcVBuf->buf_vaddr, 1, srcWidth * srcHeight / 4, fpin);
    fseek(fpin, srcStride * srcHeight, SEEK_SET);
  }

  if (G2D_YUYV == dstFmt) {
    CreateG2DBuffer(&dstYBuf, dstWidth * 2 * dstHeight);
  } else if (G2D_NV12 == dstFmt) {
    CreateG2DBuffer(&dstYBuf, dstStride * dstHeight);
    CreateG2DBuffer(&dstUBuf, dstStride * dstHeight / 2);
  } else if (G2D_I420 == dstFmt) {
    CreateG2DBuffer(&dstYBuf, dstStride * dstHeight);
    CreateG2DBuffer(&dstUBuf, dstStride * dstHeight / 4);
    CreateG2DBuffer(&dstVBuf, dstStride * dstHeight / 4);
  }

  printf("inFile=%s, src, wxh=%dx%d, stride=%d, srcFmt=%d\n", inFile, srcWidth,
         srcHeight, srcStride, srcFmt);
  printf("outFile=%s, dst, wxh=%dx%d, stride=%d, dstFmt=%d\n", outFile,
         dstWidth, dstHeight, dstStride, dstFmt);
  ////////////////////----------------------------------------
  struct g2d_surfaceEx *srcEx;
  struct g2d_surfaceEx *dstEx;

  srcEx = (struct g2d_surfaceEx *)malloc(sizeof(struct g2d_surfaceEx));
  dstEx = (struct g2d_surfaceEx *)malloc(sizeof(struct g2d_surfaceEx));
  memset(srcEx, 0, sizeof(struct g2d_surfaceEx));
  memset(dstEx, 0, sizeof(struct g2d_surfaceEx));

  struct g2d_surface *src = (struct g2d_surface *)srcEx;
  struct g2d_surface *dst = (struct g2d_surface *)dstEx;

  src->right = srcWidth;
  src->bottom = srcHeight;
  src->width = srcWidth;
  src->height = srcHeight;
  src->format = srcFmt;
  if (G2D_YUYV == src->format) {
    src->stride = srcStride;
    printf("change yuyv stride to %d\n", src->stride);
    src->planes[0] = srcYBuf->buf_paddr;
  } else if (G2D_NV12 == src->format) {
    src->stride = srcWidth;
    src->planes[0] = srcYBuf->buf_paddr;
    src->planes[1] = srcUBuf->buf_paddr;
  }

  dst->right = dstWidth;
  dst->bottom = dstHeight;
  dst->width = dstWidth;
  dst->height = dstHeight;
  dst->stride = dstStride;
  dst->format = dstFmt;

  if (G2D_YUYV == dst->format) {
    dst->planes[0] = dstYBuf->buf_paddr;
  } else if (G2D_NV12 == dst->format) {
    dst->planes[0] = dstYBuf->buf_paddr;
    dst->planes[1] = dstUBuf->buf_paddr;
  } else if (G2D_I420 == dst->format) {
    dst->planes[0] = dstYBuf->buf_paddr;
    dst->planes[1] = dstUBuf->buf_paddr;
    dst->planes[2] = dstVBuf->buf_paddr;
  }

  printf("\nLinear conversion start\n");
  clock_gettime(CLOCK_REALTIME, &t1);
  for (i = 0; i < TEST_LOOP; i++) {
    g2d_blit(_g2d_handle, src, dst); // for real converstion
  }
  g2d_finish(_g2d_handle);
  clock_gettime(CLOCK_REALTIME, &t2);
  Timespec_Sub(&diff, &t2, &t1);
  t = 1000 * Timespec_Double(&diff);

  printf("\nLinear conversion done %f ms  \n", t / TEST_LOOP);

  if (G2D_YUYV == dstFmt) {
    fwrite(dstYBuf->buf_vaddr, 1, dstStride * dstHeight, fpout);
  } else if (G2D_NV12 == dstFmt) {
    fwrite(dstYBuf->buf_vaddr, 1, dstStride * dstHeight, fpout);
    fwrite(dstUBuf->buf_vaddr, 1, dstStride * dstHeight / 2, fpout);
  } else if (G2D_I420 == dstFmt) {
    fwrite(dstYBuf->buf_vaddr, 1, dstStride * dstHeight, fpout);
    fwrite(dstUBuf->buf_vaddr, 1, dstStride * dstHeight / 4, fpout);
    fwrite(dstVBuf->buf_vaddr, 1, dstStride * dstHeight / 4, fpout);
  }

  printf("\nClosing opened files...");
  fclose(fpout);
  fclose(fpin);
  printf("OK");

  printf("\nFinishing OpenCL...");

  printf("\nClosing G2D Device...");

  printf("free srcY...\n");
  ReleaseG2DBuffer(srcYBuf);
  printf("free srcU...\n");
  ReleaseG2DBuffer(srcUBuf);
  printf("free srcV..\n");
  ReleaseG2DBuffer(srcVBuf);

  printf("free dstY...\n");
  ReleaseG2DBuffer(dstYBuf);
  printf("free dstU...\n");
  ReleaseG2DBuffer(dstUBuf);
  printf("free dstV...\n");
  ReleaseG2DBuffer(dstVBuf);

  CloseG2D();
  free(srcEx);
  free(dstEx);
  printf("OK\n\n");

  return 0;
}

int OpenG2D(void) {
  if (g2d_open(&_g2d_handle) == -1 || _g2d_handle == NULL) {
    printf("Fail to open g2d device!\n");
    return -ENOTTY;
  }

  return 0;
}

void CloseG2D(void) {
  g2d_close(_g2d_handle);

  return;
}

int CreateG2DBuffer(struct g2d_buf **buf, int size) {
  *buf = g2d_alloc(size, CACHEABLE);
  if (!*buf) {
    printf("Fail to allocate physical memory !\n");
    return -ENOMEM;
  }

  return 0;
}

void ReleaseG2DBuffer(struct g2d_buf *buf) {
  if (!buf) {
    return;
  }

  g2d_free(buf);
}
