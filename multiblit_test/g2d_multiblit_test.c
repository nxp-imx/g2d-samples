/*
 * Copyright 2021 NXP
 * Copyright 2013-2014 Freescale Semiconductor, Inc.
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/time.h>
#include "g2d.h"

/*----------------------------
 * g2d multi blit test
 *---------------------------*/

#define TEST_WIDTH  1920
#define TEST_HEIGHT 1080
#define TEST_BPP    32
#define TEST_FORMAT "RGBA"
#define TEST_LOOP 16

int main(void)
{
    int i,j,n,diff = 0;
    const int layers = 8;
    void *handle = NULL;
    int g2d_feature_available = 0;
    struct timeval tv1,tv2;
    char test_format[64];
    int test_width, test_height, test_bpp;
    struct g2d_buf *s_buf, *d_buf;
    struct g2d_buf *mul_s_buf[layers];

    struct g2d_surface src, dst;
    struct g2d_surface mul_src[layers], mul_dst[layers];

    struct g2d_surface_pair *sp[layers];
    for(n = 0; n < layers; n++)
    {
        sp[n] = (struct g2d_surface_pair *)malloc(sizeof(struct g2d_surface_pair) * layers);
    }

    //---------- g2d open -------------
    if(g2d_open(&handle))
    {
        printf("g2d_open fail.\n");
        return -ENOTTY;
    }

    //---------- g2d alloc -------------
    test_width = TEST_WIDTH;
    test_height = TEST_HEIGHT;
    test_bpp  = TEST_BPP;
    strcpy(test_format, TEST_FORMAT);

    test_width = (test_width + 15) & ~15;
    test_height = (test_height + 15) & ~15;
    printf("Width %d, Height %d, Format %s, Bpp %d\n",
            test_width, test_height, test_format, test_bpp);

    //stress test for g2d memory allocator
    for(i=0; i<16; i++)
    { 
        s_buf = g2d_alloc(1024*1024*((i % 16) + 1), 1);
        d_buf = g2d_alloc(1024*1024*((i % 16) + 1), 0);

        g2d_free(s_buf);
        g2d_free(d_buf);
    }

    s_buf = g2d_alloc(test_width * test_height * 4, 0);
    d_buf = g2d_alloc(test_width * test_height * 4, 0);

    for(n = 0; n < layers; n++)
    {
        mul_s_buf[n] = g2d_alloc(test_width * test_height * 4, 0);
    }


    //---------- g2d blit -------------
    printf("\n----- g2d blit -----\n");
    src.format = G2D_RGBA8888;
    dst.format = G2D_RGBA8888;

    src.planes[0] = s_buf->buf_paddr;
    src.planes[1] = s_buf->buf_paddr+test_width*test_height;
    src.planes[2] = s_buf->buf_paddr+test_width*test_height*2;
    src.left = 0;
    src.top = 0;
    src.right = test_width;
    src.bottom = test_height;
    src.stride = test_width;
    src.width  = test_width;
    src.height = test_height;
    src.rot    = G2D_ROTATION_0;

    dst.planes[0] = d_buf->buf_paddr;
    dst.left = 0;
    dst.top = 0;
    dst.right = test_width;
    dst.bottom = test_height;
    dst.stride = test_width;
    dst.width  = test_width;
    dst.height = test_height;
    dst.rot    = G2D_ROTATION_0;

    *((int *)((long)s_buf->buf_vaddr) ) = 0x1a2b3c4d;
    *((int *)((long)d_buf->buf_vaddr) ) = 0x0;
    gettimeofday(&tv1, NULL);

    for(i=0; i<TEST_LOOP; i++)
    {
        g2d_blit(handle, &src, &dst);
    }

    g2d_finish(handle);

    gettimeofday(&tv2, NULL);

    if(*((int *)s_buf->buf_vaddr) != *((int *)d_buf->buf_vaddr))
    {
        printf("g2d blit fail!!!\n");
    }

    diff = ((tv2.tv_sec-tv1.tv_sec)*1000000+(tv2.tv_usec-tv1.tv_usec)) / TEST_LOOP;
    printf("g2d blit time %dus, %dfps, %dMpixel/s ........\n",
            diff, 1000000/diff, test_width * test_height / diff);


    /*--- g2d blit with multiblit */
    printf("\n--- g2d blit with multiblit ---\n");
    g2d_query_feature(handle, G2D_MULTI_SOURCE_BLT, &g2d_feature_available);
    if(g2d_feature_available == 0)
    {
        printf("!!! g2d_feature 'G2D_MULTI_SOURCE_BLT' Not Supported for this hardware!!!\n");
        goto FAIL;
    }

    for(n = 0; n < layers; n++)
    {
        sp[n]->s = src;
        sp[n]->d = dst;
    }

    gettimeofday(&tv1, NULL);

    for(i=0; i<TEST_LOOP; i++)
    {
        g2d_multi_blit(handle, sp, 1);
    }

    g2d_finish(handle);

    gettimeofday(&tv2, NULL);
    diff = ((tv2.tv_sec-tv1.tv_sec)*1000000+(tv2.tv_usec-tv1.tv_usec)) / TEST_LOOP;
    printf("g2d multiblit 1 layers time %dus, %dfps, %dMpixel/s ........\n",
            diff, 1000000/diff, test_width * test_height / diff);

    gettimeofday(&tv1, NULL);
    for(i=0; i<TEST_LOOP; i++)
    {
        g2d_multi_blit(handle, sp, 4);
    }

    g2d_finish(handle);

    gettimeofday(&tv2, NULL);
    diff = ((tv2.tv_sec-tv1.tv_sec)*1000000+(tv2.tv_usec-tv1.tv_usec)) / TEST_LOOP;
    printf("g2d multiblit 4 layers time %dus, %dfps, %d(4 * %d)Mpixel/s ........\n", 
            diff, 1000000/diff, test_width * test_height / diff * 4, test_width * test_height / diff);

    gettimeofday(&tv1, NULL);
    for(i=0; i<TEST_LOOP; i++)
    {
        g2d_multi_blit(handle, sp, 8);
    }

    g2d_finish(handle);

    gettimeofday(&tv2, NULL);
    diff = ((tv2.tv_sec-tv1.tv_sec)*1000000+(tv2.tv_usec-tv1.tv_usec)) / TEST_LOOP;
    printf("g2d multiblit 8 layers time %dus, %dfps, %d(8 * %d)Mpixel/s ........\n", 
            diff, 1000000/diff, test_width * test_height / diff * 8, test_width * test_height / diff);

    if(*((int *)s_buf->buf_vaddr) != *((int *)d_buf->buf_vaddr))
    {
        printf("\ng2d multi blit fail!!!\n");
    }


    //-------------- ROTATION ------------------------
    printf("\n\n------ ROTATION -----\n");

    // 0 DEGREE
    //set test data in src buffer
    for(n = 0; n < layers; n++)
    {
        for(i=0; i<test_height; i++)
        {
            for(j=0; j<test_width; j++)
            {
                *(int *)(((long)mul_s_buf[n]->buf_vaddr) + (i*test_width+j)*4) = (i*test_width+j) + n*10;
            }
        }
    }

    //set garbage data in dst buffer
    memset(d_buf->buf_vaddr, 0xcd, test_width * test_height * 4);

    {
        sp[0]->s.left   = 0;
        sp[0]->s.top    = 0;
        sp[0]->s.right  = test_width/4;
        sp[0]->s.bottom = test_height/2;

        sp[1]->s.left   = test_width/4;
        sp[1]->s.top    = 0;
        sp[1]->s.right  = test_width/2;
        sp[1]->s.bottom = test_height/2;

        sp[2]->s.left   = test_width/2;
        sp[2]->s.top    = 0;
        sp[2]->s.right  = test_width/2 + test_width/4;
        sp[2]->s.bottom = test_height/2;

        sp[3]->s.left   = test_width - test_width/4;
        sp[3]->s.top    = 0;
        sp[3]->s.right  = test_width;
        sp[3]->s.bottom = test_height/2;

        for(n = 4; n < layers; n++)
        {
            sp[n]->s.left   = sp[n-4]->s.left;
            sp[n]->s.top    = test_height/2;
            sp[n]->s.right  = sp[n-4]->s.right;
            sp[n]->s.bottom = test_height;
        }
    }

    for(n = 0; n < layers; n++)
    {
        sp[n]->s.stride = test_width;
        sp[n]->s.width  = test_width;
        sp[n]->s.height = test_height;
        sp[n]->s.format = G2D_RGBA8888;
        sp[n]->s.rot    = G2D_ROTATION_0;
        sp[n]->s.planes[0] = mul_s_buf[n]->buf_paddr;
    }

    /* These data are the same for all dst surface. */
    for(n = 0; n < layers; n++)
    {
        sp[n]->d.stride = test_width;
        sp[n]->d.width = test_width;
        sp[n]->d.height = test_height;
        sp[n]->d.format = G2D_RGBA8888;
        sp[n]->d.rot = G2D_ROTATION_0;
        sp[n]->d.planes[0] = d_buf->buf_paddr;
    }

    /* These data are different. */
    for(n = 0; n < layers; n++)
    {
        sp[n]->d.left = 0;
        sp[n]->d.top = 0;
        sp[n]->d.right = test_width;
        sp[n]->d.bottom = test_height;
    }

    gettimeofday(&tv1, NULL);

    for(i=0; i<TEST_LOOP; i++)
    {
        g2d_multi_blit(handle, sp, layers);
    }

    g2d_finish(handle);

    gettimeofday(&tv2, NULL);
    diff = ((tv2.tv_sec-tv1.tv_sec)*1000000+(tv2.tv_usec-tv1.tv_usec)) / TEST_LOOP;

    for(i=0; i<test_height; i++)
    {
        for(j=0; j<test_width; j++)
        {
            int layer = ((i >= test_height/2)? 4 : 0) + j/(test_width/4);
            int correct_val = *(int *)(((long)mul_s_buf[layer]->buf_vaddr) + (i*test_width+j)*4);
            int rotated_val = *(int *)(((long)d_buf->buf_vaddr) + (i*test_width+j)*4);
            if(correct_val != rotated_val)
            {
                printf("[%d][%d]: 0 rotation value should be %d instead of %d(0x%x)\n", 
                        i, j, correct_val, rotated_val, rotated_val);
                printf("\n  0 DEGREE ROTATION fail!!!\n");
            }
        }
    }
    printf("  0 rotation 8 layers time %dus, %dfps, %d(8 * %d)Mpixel/s ........\n", 
            diff, 1000000/diff, test_width * test_height / diff * 8, test_width * test_height / diff);

    for(n = 0; n < layers; n++)
    {
        sp[n]->s.left = 0;
        sp[n]->s.top = 0;
        sp[n]->s.right = test_width;
        sp[n]->s.bottom = test_height;
    }

    gettimeofday(&tv1, NULL);
    for(i=0; i<TEST_LOOP; i++)
    {
        g2d_multi_blit(handle, sp, 4);
    }
    g2d_finish(handle);
    gettimeofday(&tv2, NULL);
    diff = ((tv2.tv_sec-tv1.tv_sec)*1000000+(tv2.tv_usec-tv1.tv_usec)) / TEST_LOOP;
    printf("  0 rotation 4 layers time %dus, %dfps, %d(4 * %d)Mpixel/s ........\n",
            diff, 1000000/diff, test_width * test_height / diff * 4, test_width * test_height / diff);

    gettimeofday(&tv1, NULL);
    for(i=0; i<TEST_LOOP; i++)
    {
        g2d_multi_blit(handle, sp, 1);
    }
    g2d_finish(handle);
    gettimeofday(&tv2, NULL);
    diff = ((tv2.tv_sec-tv1.tv_sec)*1000000+(tv2.tv_usec-tv1.tv_usec)) / TEST_LOOP;
    printf("  0 rotation 1 layers time %dus, %dfps, %dMpixel/s ........\n",
            diff, 1000000/diff, test_width * test_height / diff);

    /* -------- 90 DEGREE ------------*/
    for(i = 0; i < test_width; i++)
    {
        for(j = 0; j < test_height; j++)
        {
            *(int *)(((long)mul_s_buf[layers-1]->buf_vaddr) + (i*test_height+j)*4) = i*test_height+j + (layers-1)*10;
        }
    }

    for(n = 0; n < layers; n++)
    {
        sp[n]->s.left = 0;
        sp[n]->s.top  = 0;
        sp[n]->s.right = test_height;
        sp[n]->s.bottom = test_width;

        sp[n]->s.stride = test_height;
        sp[n]->s.width  = test_height;
        sp[n]->s.height = test_width;
        sp[n]->s.format = G2D_RGBA8888;
        sp[n]->s.rot    = G2D_ROTATION_90;
        sp[n]->s.planes[0] = mul_s_buf[n]->buf_paddr;
    }

    for(n = 0; n < layers; n++)
    {
        sp[n]->d.left = 0;
        sp[n]->d.top  = 0;
        sp[n]->d.right = test_width;
        sp[n]->d.bottom = test_height;

        sp[n]->d.stride = test_width;
        sp[n]->d.width  = test_width;
        sp[n]->d.height = test_height;
        sp[n]->d.format = G2D_RGBA8888;
        sp[n]->d.rot    = G2D_ROTATION_0;
        sp[n]->d.planes[0] = d_buf->buf_paddr;
    }

    gettimeofday(&tv1, NULL);

    for(i=0; i<TEST_LOOP; i++)
    {
        g2d_multi_blit(handle, sp, layers);
    }

    g2d_finish(handle);

    gettimeofday(&tv2, NULL);

    diff = ((tv2.tv_sec-tv1.tv_sec)*1000000+(tv2.tv_usec-tv1.tv_usec)) / TEST_LOOP;

    for(i=0; i<test_height; i++)
    {
        for(j=0; j<test_width; j++)
        {
            int correct_val = *(int *)(((long)mul_s_buf[layers-1]->buf_vaddr) + (j*test_height + test_height - i - 1)*4);
            int rotated_val = *(int *)(((long)d_buf->buf_vaddr) + (i*test_width+j)*4);
            if(correct_val != rotated_val)
            {
                printf("[%d][%d]: 90 rotation value should be %d instead of %d(0x%x)\n", 
                        i, j, correct_val, rotated_val, rotated_val);
                printf(" 90 DEGREE ROTATION fail!!!\n");
            }
        }
    }
    printf("\n 90 rotation 8 layers time %dus, %dfps, %d(8 * %d)Mpixel/s ........\n", 
            diff, 1000000/diff, test_width * test_height / diff * 8, test_width * test_height / diff);

    gettimeofday(&tv1, NULL);
    for(i=0; i<TEST_LOOP; i++)
    {
        g2d_multi_blit(handle, sp, 4);
    }
    g2d_finish(handle);
    gettimeofday(&tv2, NULL);
    diff = ((tv2.tv_sec-tv1.tv_sec)*1000000+(tv2.tv_usec-tv1.tv_usec)) / TEST_LOOP;
    printf(" 90 rotation 4 layers time %dus, %dfps, %d(4 * %d)Mpixel/s ........\n", 
            diff, 1000000/diff, test_width * test_height / diff * 4, test_width * test_height / diff);

    gettimeofday(&tv1, NULL);
    for(i=0; i<TEST_LOOP; i++)
    {
        g2d_multi_blit(handle, sp, 1);
    }
    g2d_finish(handle);
    gettimeofday(&tv2, NULL);
    diff = ((tv2.tv_sec-tv1.tv_sec)*1000000+(tv2.tv_usec-tv1.tv_usec)) / TEST_LOOP;
    printf(" 90 rotation 1 layers time %dus, %dfps, %dMpixel/s ........\n",
            diff, 1000000/diff, test_width * test_height / diff);

    /*--- 180 DEGREE ----*/
    test_width = 1920;
    test_height = 1080;
    for(i=0; i<test_height; i++)
    {
        for(j=0; j<test_width; j++)
        {
            *(int *)(((long)mul_s_buf[layers-1]->buf_vaddr) + (i*test_width+j)*4) = i*test_width+j + (layers-1)*10;
        }
    }

    for(n = 0; n < layers; n++)
    {
        sp[n]->s.left = 0;
        sp[n]->s.top  = 0;
        sp[n]->s.right = test_width;
        sp[n]->s.bottom = test_height;

        sp[n]->s.stride = test_width;
        sp[n]->s.width  = test_width;
        sp[n]->s.height = test_height;
        sp[n]->s.format = G2D_RGBA8888;
        sp[n]->s.rot    = G2D_ROTATION_180;
        sp[n]->s.planes[0] = mul_s_buf[n]->buf_paddr;
    }

    for(n = 0; n < layers; n++)
    {    
        sp[n]->d.left = 0;
        sp[n]->d.top  = 0;
        sp[n]->d.right = test_width;
        sp[n]->d.bottom = test_height;

        sp[n]->d.stride = test_width;
        sp[n]->d.width  = test_width;
        sp[n]->d.height = test_height;
        sp[n]->d.format = G2D_RGBA8888;
        sp[n]->d.rot    = G2D_ROTATION_0;
        sp[n]->d.planes[0] = d_buf->buf_paddr;
    }

    gettimeofday(&tv1, NULL);

    for(i=0; i<TEST_LOOP; i++)
    {
        g2d_multi_blit(handle, sp, layers);
    }

    g2d_finish(handle);

    gettimeofday(&tv2, NULL);
    diff = ((tv2.tv_sec-tv1.tv_sec)*1000000+(tv2.tv_usec-tv1.tv_usec)) / TEST_LOOP;


    for(i=0; i<test_height; i++)
    {
        for(j=0; j<test_width; j++)
        {
            int correct_val = *(int *)(((long)mul_s_buf[layers-1]->buf_vaddr) + ((test_height - i - 1)*test_width + (test_width - j -1))*4);
            int rotated_val = *(int *)(((long)d_buf->buf_vaddr) + (i*test_width+j)*4);
            if(rotated_val != correct_val)
            {
                printf("[%d][%d]: 180 rotation value should be %d instead of %d(0x%x)\n", 
                        i, j, correct_val, rotated_val, rotated_val);
                printf("180 DEGREE ROTATION fail!!!\n");
            }
        }
    }
    printf("\n180 rotation 8 layers time %dus, %dfps, %d(8 * %d)Mpixel/s ........\n", 
            diff, 1000000/diff, test_width * test_height / diff * 8, test_width * test_height / diff);

    gettimeofday(&tv1, NULL);
    for(i=0; i<TEST_LOOP; i++)
    {
        g2d_multi_blit(handle, sp, 4);
    }
    g2d_finish(handle);
    gettimeofday(&tv2, NULL);
    diff = ((tv2.tv_sec-tv1.tv_sec)*1000000+(tv2.tv_usec-tv1.tv_usec)) / TEST_LOOP;
    printf("180 rotation 4 layers time %dus, %dfps, %d(4 * %d)Mpixel/s ........\n",
            diff, 1000000/diff, test_width * test_height / diff * 4, test_width * test_height / diff);

    gettimeofday(&tv1, NULL);
    for(i=0; i<TEST_LOOP; i++)
    {
        g2d_multi_blit(handle, sp, 1);
    }
    g2d_finish(handle);
    gettimeofday(&tv2, NULL);
    diff = ((tv2.tv_sec-tv1.tv_sec)*1000000+(tv2.tv_usec-tv1.tv_usec)) / TEST_LOOP;
    printf("180 rotation 1 layers time %dus, %dfps, %dMpixel/s ........\n",
            diff, 1000000/diff, test_width * test_height / diff);

    /*--- 270 DEGREE ---*/
    test_width = 1920;
    test_height = 1080;
    memset(d_buf->buf_vaddr, 0xcd, test_width * test_height * 4);

    for(i=0; i<test_width; i++)
    {
        for(j=0; j<test_height; j++)
        {
            *(int *)(((long)mul_s_buf[layers-1]->buf_vaddr) + (i*test_height+j)*4) = i*test_height+j + (layers-1)*10;
        }
    }

    for(n = 0; n < layers; n++)
    {
        sp[n]->s.left = 0;
        sp[n]->s.top  = 0;
        sp[n]->s.right = test_height;
        sp[n]->s.bottom = test_width;

        sp[n]->s.stride = test_height;
        sp[n]->s.width  = test_height;
        sp[n]->s.height = test_width;
        sp[n]->s.format = G2D_RGBA8888;
        sp[n]->s.rot    = G2D_ROTATION_270;
        sp[n]->s.planes[0] = mul_s_buf[n]->buf_paddr;
    }

    sp[0]->d.left = 0;
    sp[0]->d.top  = 0;
    sp[0]->d.right = test_width;
    sp[0]->d.bottom = test_height;

    sp[0]->d.stride = test_width;
    sp[0]->d.width  = test_width;
    sp[0]->d.height = test_height;
    sp[0]->d.format = G2D_RGBA8888;
    sp[0]->d.rot    = G2D_ROTATION_0;
    sp[0]->d.planes[0] = d_buf->buf_paddr;

    for(n = 1; n < layers; n++)
    {
        sp[n]->d = sp[0]->d;
    }

    gettimeofday(&tv1, NULL);

    for(i=0; i<TEST_LOOP; i++)
    {
        g2d_multi_blit(handle, sp, layers);
    }

    g2d_finish(handle);

    gettimeofday(&tv2, NULL);
    diff = ((tv2.tv_sec-tv1.tv_sec)*1000000+(tv2.tv_usec-tv1.tv_usec)) / TEST_LOOP;

    for(i=0; i<test_height; i++)
    {
        for(j=0; j<test_width; j++)
        {
            int correct_val = *(int *)(((long)mul_s_buf[layers-1]->buf_vaddr) + ((test_width - j - 1)*test_height + i)*4);
            int rotated_val = *(int *)(((long)d_buf->buf_vaddr) + (i*test_width+j)*4);
            if(correct_val != rotated_val)
            {
                printf("[%d][%d]: 270 rotation value should be %d instead of %d(0x%x)\n", i, j, correct_val, rotated_val, rotated_val);
                printf("270 DEGREE ROTATION fail!!!\n");
            }
        }
    }
    printf("\n270 rotation 8 layers time %dus, %dfps, %d(8 * %d)Mpixel/s ........\n", 
            diff, 1000000/diff, test_width * test_height / diff * 8, test_width * test_height / diff);

    gettimeofday(&tv1, NULL);
    for(i=0; i<TEST_LOOP; i++)
    {
        g2d_multi_blit(handle, sp, 4);
    }
    g2d_finish(handle);
    gettimeofday(&tv2, NULL);
    diff = ((tv2.tv_sec-tv1.tv_sec)*1000000+(tv2.tv_usec-tv1.tv_usec)) / TEST_LOOP;
    printf("270 rotation 4 layers time %dus, %dfps, %d(4 * %d)Mpixel/s ........\n",
            diff, 1000000/diff, test_width * test_height / diff * 4, test_width * test_height / diff);

    gettimeofday(&tv1, NULL);
    for(i=0; i<TEST_LOOP; i++)
    {
        g2d_multi_blit(handle, sp, 1);
    }
    g2d_finish(handle);
    gettimeofday(&tv2, NULL);
    diff = ((tv2.tv_sec-tv1.tv_sec)*1000000+(tv2.tv_usec-tv1.tv_usec)) / TEST_LOOP;
    printf("270 rotation 1 layers time %dus, %dfps, %dMpixel/s ........\n", diff, 1000000/diff, test_width * test_height / diff);

    /*--- flip H ---*/
    test_width = 1920;
    test_height = 1080;
    for(i=0; i<test_height; i++)
    {
        for(j=0; j<test_width; j++)
        {
            *(int *)(((long)mul_s_buf[layers-1]->buf_vaddr) + (i*test_width+j)*4) = i*test_width+j + (layers-1)*10;
        }
    }

    for(n = 0; n < layers; n++)
    {
        sp[n]->s.left = 0;
        sp[n]->s.top  = 0;
        sp[n]->s.right = test_width;
        sp[n]->s.bottom = test_height;

        sp[n]->s.stride = test_width;
        sp[n]->s.width  = test_width;
        sp[n]->s.height = test_height;
        sp[n]->s.format = G2D_RGBA8888;
        sp[n]->s.rot    = G2D_FLIP_H;
        sp[n]->s.planes[0] = mul_s_buf[n]->buf_paddr;
    }

    for(n = 0; n < layers; n++)
    {
        sp[n]->d.left = 0;
        sp[n]->d.top  = 0;
        sp[n]->d.right = test_width;
        sp[n]->d.bottom = test_height;

        sp[n]->d.stride = test_width;
        sp[n]->d.width  = test_width;
        sp[n]->d.height = test_height;
        sp[n]->d.format = G2D_RGBA8888;
        sp[n]->d.rot    = G2D_ROTATION_0;
        sp[n]->d.planes[0] = d_buf->buf_paddr;
    }

    gettimeofday(&tv1, NULL);

    for(i=0; i<TEST_LOOP; i++)
    {
        g2d_multi_blit(handle, sp, layers);
    }

    g2d_finish(handle);

    gettimeofday(&tv2, NULL);
    diff = ((tv2.tv_sec-tv1.tv_sec)*1000000+(tv2.tv_usec-tv1.tv_usec)) / TEST_LOOP;

    for(i=0; i<test_height; i++)
    {
        for(j=0; j<test_width; j++)
        {
            int correct_val = *(int *)(((long)mul_s_buf[layers-1]->buf_vaddr) + (i*test_width + (test_width-j-1))*4);
            int rotated_val = *(int *)(((long)d_buf->buf_vaddr) + (i*test_width+j)*4);
            if(correct_val != rotated_val)
            {
                printf("[%d][%d]: flip-h value should be %d instead of %d(0x%x)\n", 
                        i, j, correct_val, rotated_val, rotated_val);
                printf("\nFLIP H fail!!!\n");
            }
        }
    }
    printf("\nflip h 8 layers time %dus, %dfps, %d(8 * %d)Mpixel/s ........\n", 
            diff, 1000000/diff, test_width * test_height / diff * 8, test_width * test_height / diff);

    /*--- flip v ---*/
    memset(d_buf->buf_vaddr, 0xcd, test_width * test_height * 4);

    for(n = 0; n < layers; n++)
    {
        sp[n]->s.left = 0;
        sp[n]->s.top  = 0;
        sp[n]->s.right = test_width;
        sp[n]->s.bottom = test_height;

        sp[n]->s.stride = test_width;
        sp[n]->s.width  = test_width;
        sp[n]->s.height = test_height;
        sp[n]->s.format = G2D_RGBA8888;
        sp[n]->s.rot    = G2D_FLIP_V;
        sp[n]->s.planes[0] = mul_s_buf[n]->buf_paddr;
    }

    sp[0]->d.left = 0;
    sp[0]->d.top  = 0;
    sp[0]->d.right = test_width;
    sp[0]->d.bottom = test_height;

    sp[0]->d.stride = test_width;
    sp[0]->d.width  = test_width;
    sp[0]->d.height = test_height;
    sp[0]->d.format = G2D_RGBA8888;
    sp[0]->d.rot    = G2D_ROTATION_0;
    sp[0]->d.planes[0] = d_buf->buf_paddr;

    for(n = 1; n < layers; n++)
    {
        sp[n]->d = sp[0]->d;
    }

    gettimeofday(&tv1, NULL);

    for(i=0; i<TEST_LOOP; i++)
    {
        g2d_multi_blit(handle, sp, layers);
    }

    g2d_finish(handle);

    gettimeofday(&tv2, NULL);
    diff = ((tv2.tv_sec-tv1.tv_sec)*1000000+(tv2.tv_usec-tv1.tv_usec)) / TEST_LOOP;

    for(i=0; i<test_height; i++)
    {
        for(j=0; j<test_width; j++)
        {
            int correct_val = *(int *)(((long)mul_s_buf[layers-1]->buf_vaddr) + ((test_height - i - 1)*test_width+j)*4);
            int rotated_val = *(int *)(((long)d_buf->buf_vaddr) + (i*test_width+j)*4);
            if(correct_val != rotated_val)
            {
                printf("[%d][%d]: flip-v value should be %d instead of %d(0x%x)\n", i, j, correct_val, rotated_val, rotated_val);
                printf("FLIP V fail!!!\n");
            }
        }
    }
    printf("flip v 8 layers time %dus, %dfps, %d(8 * %d)Mpixel/s ........\n", 
            diff, 1000000/diff, test_width * test_height / diff * 8, test_width * test_height / diff);


    /**/
    /*-------------------------------------*/
    /* --- test format conversion ---*/
    test_width = 1920;
    test_height = 1080;
    printf("\n\n--- TEST FORMAT TRANSFORMATION ---\n");

    for(i=0; i<test_height; i++)
    {
        for(j=0; j<test_width; j++)
        {
            char *p = (char *)(((long)s_buf->buf_vaddr) + (i*test_width+j)*4);
            p[0] = p[1] = p[2] = p[3] = (i*test_width+j) % 255;
        }
    }

    src.planes[0] = s_buf->buf_paddr;
    src.planes[1] = s_buf->buf_paddr+test_width*test_height;
    src.planes[2] = s_buf->buf_paddr+test_width*test_height*2;
    src.left = 0;
    src.top = 0;
    src.right = test_width;
    src.bottom = test_height;
    src.stride = test_width;
    src.width  = test_width;
    src.height = test_height;
    src.rot    = G2D_ROTATION_0;

    memset(d_buf->buf_vaddr, 0xcd, test_width * test_height * 4);
    dst.planes[0] = d_buf->buf_paddr;
    dst.left = 0;
    dst.top = 0;
    dst.right = test_width;
    dst.bottom = test_height;
    dst.stride = test_width;
    dst.width  = test_width;
    dst.height = test_height;
    dst.rot    = G2D_ROTATION_0;

    src.format = G2D_RGBA8888;
    dst.format = G2D_YUYV;

    for(n = 0; n < layers; n++)
    {
        sp[n]->s = src;
        sp[n]->d = dst;
    }

    gettimeofday(&tv1, NULL);

    for(i=0; i<TEST_LOOP; i++)
    {
        g2d_multi_blit(handle, sp, layers);
    }

    g2d_finish(handle);

    gettimeofday(&tv2, NULL);
    diff = ((tv2.tv_sec-tv1.tv_sec)*1000000+(tv2.tv_usec-tv1.tv_usec)) / TEST_LOOP;

    for(i=0; i<test_height/2; i++)
    {
        for(j=0; j<test_width; j++)
        {
            char *sp0 = (char *)(((long)s_buf->buf_vaddr) + (i*test_width+j)*4*2);
            char *sp1 = (char *)(((long)s_buf->buf_vaddr) + (i*test_width+j)*4*2 + 4);

            char Y0 = 0.257*sp0[0] + 0.504*sp0[1] + 0.098*sp0[2] + 16;
            char U0 = -0.148*sp0[0] - 0.291*sp0[1] + 0.439*sp0[2] + 128;
            char Y1 = 0.257*sp1[0] + 0.504*sp1[1] + 0.098*sp1[2] + 16;
            char V0 = 0.439*sp0[0] - 0.368*sp0[1] -0.071*sp0[2] + 128;

            char *p = (char *)(((long)d_buf->buf_vaddr) + (i*test_width+j)*4);

            if(abs(Y0 - p[0]) > 2 || abs(U0 - p[1]) > 2 || abs(Y1 - p[2]) > 2 || abs(V0 - p[3]) > 2)
            {
                printf("rgb to yuv fail!!!\n");
            }
        }
    }
    printf("rgb to yuv 8 layers time %dus, %dfps, %dMpixel/s ........\n", diff, 1000000/diff, test_width * test_height / diff);

    gettimeofday(&tv1, NULL);

    for(i=0; i<TEST_LOOP; i++)
    {
        g2d_multi_blit(handle, sp, 4);
    }
    g2d_finish(handle);

    gettimeofday(&tv2, NULL);
    diff = ((tv2.tv_sec-tv1.tv_sec)*1000000+(tv2.tv_usec-tv1.tv_usec)) / TEST_LOOP;
    printf("rgb to yuv 4 layers time %dus, %dfps, %dMpixel/s ........\n", diff, 1000000/diff, test_width * test_height / diff);

    gettimeofday(&tv1, NULL);

    for(i=0; i<TEST_LOOP; i++)
    {
        g2d_multi_blit(handle, sp, 1);
    }
    g2d_finish(handle);

    gettimeofday(&tv2, NULL);
    diff = ((tv2.tv_sec-tv1.tv_sec)*1000000+(tv2.tv_usec-tv1.tv_usec)) / TEST_LOOP;
    printf("rgb to yuv 1 layers time %dus, %dfps, %dMpixel/s ........\n", diff, 1000000/diff, test_width * test_height / diff);


    /**/
    /*--------------------------------------------------------------------*/
    /* --- alpha blending ---*/
    test_width = 1920;
    test_height = 1080;
    printf("\n\n--- TEST ALPHA BLENDING ---\n");
    printf("alpha blending mode :\n");
    printf("mode 1:  src: G2D_ZERO,G2D_ZERO,G2D_ZERO,G2D_ZERO,G2D_ZERO,G2D_ZERO,G2D_ZERO,G2D_ZERO dst: G2D_ONE\n");
    printf("mode 2:  src: G2D_ONE, G2D_ZERO,G2D_ZERO,G2D_ONE, G2D_ZERO,G2D_ZERO,G2D_ZERO,G2D_ONE  dst: G2D_ONE\n");
    printf("mode 3:  src: G2D_ONE, G2D_ONE, G2D_ONE, G2D_ONE, G2D_ONE, G2D_ONE, G2D_ONE, G2D_ONE  dst: G2D_ONE\n");
    printf("mode 4:  src: G2D_ZERO,G2D_ZERO,G2D_ZERO,G2D_ZERO,G2D_ZERO,G2D_ZERO,G2D_ZERO,G2D_ZERO dst: G2D_ONE_MINUS_SRC_ALPHA\n");
    printf("mode 5:  src: G2D_ONE, G2D_ONE, G2D_ONE, G2D_ONE, G2D_ONE, G2D_ONE, G2D_ONE, G2D_ONE  dst: G2D_ONE_MINUS_SRC_ALPHA\n");

    for(n = 0; n < layers; n++)
    {
        for(i = 0; i<test_height; i++)
        {
            for(j=0; j<test_width; j++)
            {
                char* p = (char *)(((long)mul_s_buf[n]->buf_vaddr) + (i*test_width+j)*4);
                p[0] = p[1] = p[2] = p[3] = 4*n % 255;
            }
        }
    }
    memset(d_buf->buf_vaddr, 0x64, test_width * test_height * 4);

    for(n = 0; n < layers; n++)
    {
        sp[n]->s.left = 0;
        sp[n]->s.top  = 0;
        sp[n]->s.right = test_width;
        sp[n]->s.bottom = test_height;

        sp[n]->s.stride = test_width;
        sp[n]->s.width  = test_width;
        sp[n]->s.height = test_height;
        sp[n]->s.format = G2D_RGBA8888;
        sp[n]->s.rot    = G2D_ROTATION_0;
        sp[n]->s.planes[0] = mul_s_buf[n]->buf_paddr;
    }

    sp[0]->d.left = 0;
    sp[0]->d.top  = 0;
    sp[0]->d.right = test_width;
    sp[0]->d.bottom = test_height;

    sp[0]->d.stride = test_width;
    sp[0]->d.width  = test_width;
    sp[0]->d.height = test_height;
    sp[0]->d.format = G2D_RGBA8888;
    sp[0]->d.rot    = G2D_ROTATION_0;
    sp[0]->d.planes[0] = d_buf->buf_paddr;

    for(n = 1; n < layers; n++)
    {
        sp[n]->d = sp[0]->d;
    }

    sp[0]->s.blendfunc = G2D_ZERO;
    sp[1]->s.blendfunc = G2D_ZERO;
    sp[2]->s.blendfunc = G2D_ZERO;
    sp[3]->s.blendfunc = G2D_ZERO;
    sp[4]->s.blendfunc = G2D_ZERO;
    sp[5]->s.blendfunc = G2D_ZERO;
    sp[6]->s.blendfunc = G2D_ZERO;
    sp[7]->s.blendfunc = G2D_ZERO;

    sp[0]->d.blendfunc = G2D_ONE;

    g2d_enable(handle,G2D_BLEND);

    gettimeofday(&tv1, NULL);

    for(i=0; i<TEST_LOOP; i++)
    {
        g2d_multi_blit(handle, sp, 8);
    }
    g2d_finish(handle);

    gettimeofday(&tv2, NULL);
    diff = ((tv2.tv_sec-tv1.tv_sec)*1000000+(tv2.tv_usec-tv1.tv_usec)) / TEST_LOOP;

    g2d_disable(handle, G2D_BLEND);

    for(i = 0; i<test_height; i++)
    {
        for(j=0; j<test_width; j++)
        {
            char* p = (char *)(((long)d_buf->buf_vaddr) + (i*test_width+j)*4);
            if(p[0] != 0x64 || p[1] != 0x64 || p[2] != 0x64 || p[3] != 0x64)
            {
                printf("alpha blending mode 1 fail!!!\n");
            }
        }
    }
    printf("\nmode 1, 8 layers time %dus, %dfps, %dMpixel/s ........\n", diff, 1000000/diff, test_width * test_height / diff);

    g2d_enable(handle,G2D_BLEND);
    gettimeofday(&tv1, NULL);
    for(i=0; i<TEST_LOOP; i++)
    {
        g2d_multi_blit(handle, sp, 4);
    }
    g2d_finish(handle);
    gettimeofday(&tv2, NULL);
    diff = ((tv2.tv_sec-tv1.tv_sec)*1000000+(tv2.tv_usec-tv1.tv_usec)) / TEST_LOOP;
    g2d_disable(handle, G2D_BLEND);
    printf("mode 1, 4 layers time %dus, %dfps, %dMpixel/s ........\n", diff, 1000000/diff, test_width * test_height / diff);

    g2d_enable(handle,G2D_BLEND);
    gettimeofday(&tv1, NULL);
    for(i=0; i<TEST_LOOP; i++)
    {
        g2d_multi_blit(handle, sp, 1);
    }
    g2d_finish(handle);
    gettimeofday(&tv2, NULL);
    diff = ((tv2.tv_sec-tv1.tv_sec)*1000000+(tv2.tv_usec-tv1.tv_usec)) / TEST_LOOP;
    g2d_disable(handle, G2D_BLEND);
    printf("mode 1, 1 layers time %dus, %dfps, %dMpixel/s ........\n", diff, 1000000/diff, test_width * test_height / diff);

    memset(d_buf->buf_vaddr, 0x64, test_width * test_height * 4);
    sp[0]->s.blendfunc = G2D_ONE;
    sp[1]->s.blendfunc = G2D_ZERO;
    sp[2]->s.blendfunc = G2D_ZERO;
    sp[3]->s.blendfunc = G2D_ONE;
    sp[4]->s.blendfunc = G2D_ZERO;
    sp[5]->s.blendfunc = G2D_ZERO;
    sp[6]->s.blendfunc = G2D_ZERO;
    sp[7]->s.blendfunc = G2D_ONE;

    sp[0]->d.blendfunc = G2D_ONE;

    g2d_enable(handle,G2D_BLEND);

    gettimeofday(&tv1, NULL);

    g2d_multi_blit(handle, sp, 8);

    g2d_finish(handle);

    gettimeofday(&tv2, NULL);
    diff = ((tv2.tv_sec-tv1.tv_sec)*1000000+(tv2.tv_usec-tv1.tv_usec)) / 1;

    g2d_disable(handle, G2D_BLEND);

    for(i = 0; i<test_height; i++)
    {
        for(j=0; j<test_width; j++)
        {
            char* sp0 = (char *)(((long)mul_s_buf[0]->buf_vaddr) + (i*test_width+j)*4);
            char* sp3 = (char *)(((long)mul_s_buf[3]->buf_vaddr) + (i*test_width+j)*4);
            char* sp7 = (char *)(((long)mul_s_buf[7]->buf_vaddr) + (i*test_width+j)*4);
            char* p = (char *)(((long)d_buf->buf_vaddr) + (i*test_width+j)*4);
            if( abs(sp0[0]+sp3[0]+sp7[0]+0x64 - p[0]) > 2
                    || abs(sp0[1]+sp3[1]+sp7[1]+0x64 - p[1]) > 2 
                    || abs(sp0[2]+sp3[2]+sp7[2]+0x64 - p[2]) > 2 
                    || abs(sp0[3]+sp3[3]+sp7[3]+0x64 - p[3]) > 2 )
            {
                printf("alpha blending mode 2 fail!!!\n");
            }
        }
    }
    printf("\nmode 2, 8 layers time %dus, %dfps, %dMpixel/s ........\n", diff, 1000000/diff, test_width * test_height / diff);

    g2d_enable(handle,G2D_BLEND);
    gettimeofday(&tv1, NULL);
    g2d_multi_blit(handle, sp, 4);
    g2d_finish(handle);
    gettimeofday(&tv2, NULL);
    diff = ((tv2.tv_sec-tv1.tv_sec)*1000000+(tv2.tv_usec-tv1.tv_usec)) / 1;
    g2d_disable(handle, G2D_BLEND);
    printf("mode 2, 4 layers time %dus, %dfps, %dMpixel/s ........\n", diff, 1000000/diff, test_width * test_height / diff);

    g2d_enable(handle,G2D_BLEND);
    gettimeofday(&tv1, NULL);
    g2d_multi_blit(handle, sp, 1);
    g2d_finish(handle);
    gettimeofday(&tv2, NULL);
    diff = ((tv2.tv_sec-tv1.tv_sec)*1000000+(tv2.tv_usec-tv1.tv_usec)) / 1;
    g2d_disable(handle, G2D_BLEND);
    printf("mode 2, 1 layers time %dus, %dfps, %dMpixel/s ........\n", diff, 1000000/diff, test_width * test_height / diff);

    memset(d_buf->buf_vaddr, 0x64, test_width * test_height * 4);
    sp[0]->s.blendfunc = G2D_ONE;
    sp[1]->s.blendfunc = G2D_ONE;
    sp[2]->s.blendfunc = G2D_ONE;
    sp[3]->s.blendfunc = G2D_ONE;
    sp[4]->s.blendfunc = G2D_ONE;
    sp[5]->s.blendfunc = G2D_ONE;
    sp[6]->s.blendfunc = G2D_ONE;
    sp[7]->s.blendfunc = G2D_ONE;

    sp[0]->d.blendfunc = G2D_ONE;

    g2d_enable(handle,G2D_BLEND);

    g2d_multi_blit(handle, sp, 8);
    g2d_finish(handle);

    g2d_disable(handle, G2D_BLEND);

    for(i = 0; i<test_height; i++)
    {
        for(j=0; j<test_width; j++)
        {
            char* sp0 = (char *)(((long)mul_s_buf[0]->buf_vaddr) + (i*test_width+j)*4);
            char* sp1 = (char *)(((long)mul_s_buf[1]->buf_vaddr) + (i*test_width+j)*4);
            char* sp2 = (char *)(((long)mul_s_buf[2]->buf_vaddr) + (i*test_width+j)*4);
            char* sp3 = (char *)(((long)mul_s_buf[3]->buf_vaddr) + (i*test_width+j)*4);
            char* sp4 = (char *)(((long)mul_s_buf[4]->buf_vaddr) + (i*test_width+j)*4);
            char* sp5 = (char *)(((long)mul_s_buf[5]->buf_vaddr) + (i*test_width+j)*4);
            char* sp6 = (char *)(((long)mul_s_buf[6]->buf_vaddr) + (i*test_width+j)*4);
            char* sp7 = (char *)(((long)mul_s_buf[7]->buf_vaddr) + (i*test_width+j)*4);
            char* p = (char *)(((long)d_buf->buf_vaddr) + (i*test_width+j)*4);
            if( abs(sp0[0]+sp1[0]+sp2[0]+sp3[0]+sp4[0]+sp5[0]+sp6[0]+sp7[0]+0x64 - p[0]) > 2 
                    || abs(sp0[1]+sp1[1]+sp2[1]+sp3[1]+sp4[1]+sp5[1]+sp6[1]+sp7[1]+0x64 - p[1]) > 2
                    || abs(sp0[2]+sp1[2]+sp2[2]+sp3[2]+sp4[2]+sp5[2]+sp6[2]+sp7[2]+0x64 - p[2]) > 2
                    || abs(sp0[3]+sp1[3]+sp2[3]+sp3[3]+sp4[3]+sp5[3]+sp6[3]+sp7[3]+0x64 - p[3]) > 2 )
            {
                printf("alpha blending mode 3 fail!!!\n");
            }
        }
    }

    memset(d_buf->buf_vaddr, 0x64, test_width * test_height * 4);
    sp[0]->s.blendfunc = G2D_ZERO;
    sp[1]->s.blendfunc = G2D_ZERO;
    sp[2]->s.blendfunc = G2D_ZERO;
    sp[3]->s.blendfunc = G2D_ZERO;
    sp[4]->s.blendfunc = G2D_ZERO;
    sp[5]->s.blendfunc = G2D_ZERO;
    sp[6]->s.blendfunc = G2D_ZERO;
    sp[7]->s.blendfunc = G2D_ZERO;

    sp[0]->d.blendfunc = G2D_ONE_MINUS_SRC_ALPHA;

    g2d_enable(handle,G2D_BLEND);

    g2d_multi_blit(handle, sp, 8);
    g2d_finish(handle);

    g2d_disable(handle, G2D_BLEND);


    for(i = 0; i<test_height; i++)
    {
        for(j=0; j<test_width; j++)
        {
            char* p = (char *)(((long)d_buf->buf_vaddr) + (i*test_width+j)*4);
            if( (abs(p[0] - 60) > 3) || (abs(p[1] - 60) > 3) || (abs(p[2] - 60) > 3) || (abs(p[3] - 60) > 3) )
            {
                printf("alpha blending mode 4 fail!!!\n");
            }
        }
    }

    memset(d_buf->buf_vaddr, 0x64, test_width * test_height * 4);
    sp[0]->s.blendfunc = G2D_ONE;
    sp[1]->s.blendfunc = G2D_ONE;
    sp[2]->s.blendfunc = G2D_ONE;
    sp[3]->s.blendfunc = G2D_ONE;
    sp[4]->s.blendfunc = G2D_ONE;
    sp[5]->s.blendfunc = G2D_ONE;
    sp[6]->s.blendfunc = G2D_ONE;
    sp[7]->s.blendfunc = G2D_ONE;

    sp[0]->d.blendfunc = G2D_ONE_MINUS_SRC_ALPHA;

    g2d_enable(handle,G2D_BLEND);

    gettimeofday(&tv1, NULL);

    g2d_multi_blit(handle, sp, 8);
    g2d_finish(handle);

    gettimeofday(&tv2, NULL);

    g2d_disable(handle, G2D_BLEND);

    diff = ((tv2.tv_sec-tv1.tv_sec)*1000000+(tv2.tv_usec-tv1.tv_usec)) / 1;

    for(i = 0; i<test_height; i++)
    {
        for(j=0; j<test_width; j++)
        {
            char* p = (char *)(((long)d_buf->buf_vaddr) + (i*test_width+j)*4);
            if( (abs(p[0] - 154) > 3) || (abs(p[1] - 154) > 3) || (abs(p[2] - 154) > 3) || (abs(p[3] - 154) > 3) )
            {
                printf("alpha blending mode 5 fail!!!\n");
            }
        }
    }
    printf("\nmode 5, 8 layers time %dus, %dfps, %dMpixel/s ........\n", diff, 1000000/diff, test_width * test_height / diff);

    g2d_enable(handle,G2D_BLEND);
    gettimeofday(&tv1, NULL);
    g2d_multi_blit(handle, sp, 4);
    g2d_finish(handle);
    gettimeofday(&tv2, NULL);
    g2d_disable(handle, G2D_BLEND);
    diff = ((tv2.tv_sec-tv1.tv_sec)*1000000+(tv2.tv_usec-tv1.tv_usec)) / 1;
    printf("mode 5, 4 layers time %dus, %dfps, %dMpixel/s ........\n", diff, 1000000/diff, test_width * test_height / diff);

    g2d_enable(handle,G2D_BLEND);
    gettimeofday(&tv1, NULL);
    g2d_multi_blit(handle, sp, 1);
    g2d_finish(handle);
    gettimeofday(&tv2, NULL);
    g2d_disable(handle, G2D_BLEND);
    diff = ((tv2.tv_sec-tv1.tv_sec)*1000000+(tv2.tv_usec-tv1.tv_usec)) / 1;
    printf("mode 5, 1 layers time %dus, %dfps, %dMpixel/s ........\n", diff, 1000000/diff, test_width * test_height / diff);

    /* Test global alpha */
    printf("\n\n--- TEST GLOBAL ALPHA ---\n");
    memset(mul_s_buf[0]->buf_vaddr, 0x20, test_width * test_height * 4);
    memset(d_buf->buf_vaddr, 0x64, test_width * test_height * 4);

    sp[0]->s.blendfunc = G2D_ONE;
    sp[0]->d.blendfunc = G2D_ONE;

    sp[0]->s.global_alpha = 0x80;
    sp[0]->d.global_alpha = 0xff;

    g2d_enable(handle,G2D_BLEND);
    g2d_enable(handle, G2D_GLOBAL_ALPHA);

    g2d_multi_blit(handle, sp, 1);
    g2d_finish(handle);

    g2d_disable(handle, G2D_GLOBAL_ALPHA);
    g2d_disable(handle, G2D_BLEND);

    for(i = 0; i<test_height; i++)
    {
        for(j=0; j<test_width; j++)
        {
            char* sp0 = (char *)(((long)mul_s_buf[0]->buf_vaddr) + (i*test_width+j)*4);
            char* p = (char *)(((long)d_buf->buf_vaddr) + (i*test_width+j)*4);
            if( abs(sp0[0]*0x80/0xff + 0x64 - p[0]) > 3 || abs(sp0[3]*0x80/0xff + 0x64 - p[3]) > 3 )
            {
                printf("global alpha fails!!!\n");
            }
        }
    }

    memset(mul_s_buf[0]->buf_vaddr, 0x20, test_width * test_height * 4);
    memset(d_buf->buf_vaddr, 0x64, test_width * test_height * 4);

    for(n = 0; n < layers; n++)
    {
        sp[n]->s.blendfunc = G2D_ONE;
        sp[n]->d.blendfunc = G2D_ONE_MINUS_SRC_ALPHA;

        sp[n]->s.global_alpha = 0x80;
        sp[n]->d.global_alpha = 0xff;
    }

    g2d_enable(handle,G2D_BLEND);
    g2d_enable(handle, G2D_GLOBAL_ALPHA);

    gettimeofday(&tv1, NULL);

    g2d_multi_blit(handle, sp, 1);

    g2d_finish(handle);

    gettimeofday(&tv2, NULL);
    diff = ((tv2.tv_sec-tv1.tv_sec)*1000000+(tv2.tv_usec-tv1.tv_usec)) / 1;

    g2d_disable(handle, G2D_GLOBAL_ALPHA);
    g2d_disable(handle, G2D_BLEND);

    for(i = 0; i<test_height; i++)
    {
        for(j=0; j<test_width; j++)
        {
            char* sp0 = (char *)(((long)mul_s_buf[0]->buf_vaddr) + (i*test_width+j)*4);
            char correct_val = ( sp0[0]*0x80/0xff ) + 0x64 - ( sp0[0]*0x80*0x64/0xff/0xff ); 
            char* p = (char *)(((long)d_buf->buf_vaddr) + (i*test_width+j)*4);
            if( abs(correct_val - p[0]) > 3 || abs(correct_val - p[3]) > 3) 
            {
                printf("global alpha fails!!!\n");
            }
        }
    }
    printf("global alpha 1 layer time %dus, %dfps, %dMpixel/s ........\n", diff, 1000000/diff, test_width * test_height / diff);

    g2d_enable(handle,G2D_BLEND);
    g2d_enable(handle, G2D_GLOBAL_ALPHA);
    gettimeofday(&tv1, NULL);
    g2d_multi_blit(handle, sp, 4);
    g2d_finish(handle);
    gettimeofday(&tv2, NULL);
    diff = ((tv2.tv_sec-tv1.tv_sec)*1000000+(tv2.tv_usec-tv1.tv_usec)) / 1;
    g2d_disable(handle, G2D_GLOBAL_ALPHA);
    g2d_disable(handle, G2D_BLEND);
    printf("global alpha 4 layer time %dus, %dfps, %dMpixel/s ........\n", diff, 1000000/diff, test_width * test_height / diff);

    g2d_enable(handle,G2D_BLEND);
    g2d_enable(handle, G2D_GLOBAL_ALPHA);
    gettimeofday(&tv1, NULL);
    g2d_multi_blit(handle, sp, 8);
    g2d_finish(handle);
    gettimeofday(&tv2, NULL);
    diff = ((tv2.tv_sec-tv1.tv_sec)*1000000+(tv2.tv_usec-tv1.tv_usec)) / 1;
    g2d_disable(handle, G2D_GLOBAL_ALPHA);
    g2d_disable(handle, G2D_BLEND);
    printf("global alpha 8 layer time %dus, %dfps, %dMpixel/s ........\n", diff, 1000000/diff, test_width * test_height / diff);

    //---------------------------
FAIL:
    for(n = 0; n < layers; n++)
        g2d_free(mul_s_buf[n]);
    g2d_free(s_buf);
    g2d_free(d_buf);

    g2d_close(handle);

    return 0;
}
