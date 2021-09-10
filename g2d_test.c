/*
 *  Copyright (C) 2013-2014 Freescale Semiconductor, Inc.
 *  All Rights Reserved.
 *
 *  The following programs are the sole property of Freescale Semiconductor Inc.,
 *  and contain its proprietary and confidential information.
 *
 */
/*
 *	g2d_test.c
 *	The sampe test code is implemented for g2d APIs.
 *	History :
 *	Date(y.m.d)        Author            Version        Description
 *	2012-10-22         Li Xianzhong      0.1            Created
 *	2013-02-22         Li Xianzhong      0.2            g2d_copy API test is added
 *	2013-03-21         Li Xianzhong      0.4            g2d clear/rotation/flip tests are added
 *	2013-04-09         Li Xianzhong      0.5            g2d alpha blending conformance & performance tests are added
 *	2013-05-17         Li Xianzhong      0.6            g2d blit & performance test with vg core
 *	2013-12-23         Li Xianzhong      0.7            g2d blend dim conformance & performance test
 *	2014-03-20         Li Xianzhong      0.8            g2d pre-multipied & de-mutlipy out alpha test
*/

#include "g2d.h"
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define TEST_WIDTH  1920
#define TEST_HEIGHT 1080
#define TEST_BPP    32
#define TEST_FORMAT "RGBA"
#define TEST_LOOP 16

int main(int argc, char*argv[])
{
   int i,j,diff = 0;
   struct timeval tv1,tv2;
   int status = -EINVAL;
   char test_format[64];
   int test_width, test_height, test_bpp;
   void *handle = NULL;
   struct g2d_surface src,dst;
   struct g2d_buf *s_buf, *d_buf;
   void *v_buf1, *v_buf2;

   if(g2d_open(&handle))
   {
        printf("g2d_open fail.\n");
        return -1;
   }

    test_width = TEST_WIDTH;
    test_height = TEST_HEIGHT;
    test_bpp  = TEST_BPP;
    strcpy(test_format, TEST_FORMAT);

    if(argc > 4)
    {
        test_width = atoi(argv[1]);
        test_height = atoi(argv[2]);
        strcpy(test_format, argv[3]);
        test_bpp = atoi(argv[4]);
    }

    test_width = (test_width + 15) & ~15;
    test_height = (test_height + 15) & ~15;

    printf("Width %d, Height %d, Format %s, Bpp %d\n", test_width, test_height, test_format, test_bpp);

    //stress test for g2d memory allocator
    for(i=0; i<128; i++)
    {
        s_buf = g2d_alloc(1024*1024*((i % 4) + 1), 1);
        g2d_free(s_buf);
        d_buf = g2d_alloc(1024*1024*((i % 16) + 1), 0);
        g2d_free(d_buf);
    }

    s_buf = g2d_alloc(test_width * test_height * 4, 0);
    d_buf = g2d_alloc(test_width * test_height * 4, 0);

    if(!strcmp(test_format, "I420"))
    {
        src.format = G2D_I420;
    }
    else if(!strcmp(test_format, "NV12"))
    {
        src.format = G2D_NV12;
    }
    else if(!strcmp(test_format, "RGBA"))
    {
        src.format = G2D_RGBA8888;
    }

    if(TEST_BPP == 32)
    {
         dst.format = G2D_RGBA8888;
    }
    else
    {
         dst.format = G2D_RGB565;
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

    dst.planes[0] = d_buf->buf_paddr;
    dst.planes[1] = d_buf->buf_paddr+test_width*test_height;
    dst.planes[2] = d_buf->buf_paddr+test_width*test_height*2;
    dst.left = 0;
    dst.top = 0;
    dst.right = test_width;
    dst.bottom = test_height;
    dst.stride = test_width;
    dst.width  = test_width;
    dst.height = test_height;
    dst.rot    = G2D_ROTATION_0;

    g2d_query_feature(handle, G2D_DST_YUV, &status);
    if (status == 1)
    {
        printf("---------------- test dst YUV feature ----------------\n");

        src.format = G2D_RGBA8888;
        dst.format = G2D_YUYV;

        memset(s_buf->buf_vaddr, 0xcc, test_width*test_height*4);
        memset(d_buf->buf_vaddr, 0x0, test_width*test_height*4);

        gettimeofday(&tv1, NULL);

        for(i=0; i<TEST_LOOP; i++)
        {
            g2d_blit(handle, &src, &dst);
        }

        g2d_finish(handle);

        gettimeofday(&tv2, NULL);
        diff = ((tv2.tv_sec-tv1.tv_sec)*1000000+(tv2.tv_usec-tv1.tv_usec)) / TEST_LOOP;

        printf("RGBA to YUY2 time %dus, %dfps, %dMpixel/s ........\n", diff, 1000000/diff, test_width * test_height / diff);

        src.format = G2D_YUYV;
        dst.format = G2D_NV16;

        for(i=0; i<test_height; i++)
        {
            for(j=0; j<test_width; j++)
            {
                char* p = (char *)(((char*)s_buf->buf_vaddr) + (i*test_width+j)*2);
                p[0] = (i*test_width+j) % 255;
                p[1] = (p[0] + 128) % 255;
            }
        }

        memset(d_buf->buf_vaddr, 0x0, test_width*test_height*4);

        g2d_blit(handle, &src, &dst);

        g2d_finish(handle);

        //test Y and UV
        for(i=0; i<test_height; i++)
        {
            for(j=0; j<test_width; j++)
            {
                char* s = (char *)(((char*)s_buf->buf_vaddr) + (i*test_width+j)*2);
                char* y = (char *)(((char*)d_buf->buf_vaddr) + (i*test_width+j));
                char* uv = y + test_width * test_height;

	        if(y[0] != s[0] || uv[0] != s[1])
                {
                    printf("YUY2 to NV16 is wrong at [%d,%d] Y = 0x%x (expect 0x%x), UV = 0x%x (expect 0x%x)\n",i,j, y[0], s[0], uv[0], s[1]);
                }
            }
        }

        gettimeofday(&tv1, NULL);

        for(i=0; i<TEST_LOOP; i++)
        {
            g2d_blit(handle, &src, &dst);
        }

        g2d_finish(handle);

        gettimeofday(&tv2, NULL);
        diff = ((tv2.tv_sec-tv1.tv_sec)*1000000+(tv2.tv_usec-tv1.tv_usec)) / TEST_LOOP;

        printf("YUY2 to NV16 time %dus, %dfps, %dMpixel/s ........\n", diff, 1000000/diff, test_width * test_height / diff);
    }

    src.format = G2D_RGBA8888;
    dst.format = G2D_RGBA8888;

/****************************************** test g2d_blit *********************************************************/
    printf("---------------- g2d blit performance ----------------\n");
    gettimeofday(&tv1, NULL);

    for(i=0; i<TEST_LOOP; i++)
    {
        g2d_blit(handle, &src, &dst);
    }

    g2d_finish(handle);

    gettimeofday(&tv2, NULL);
    diff = ((tv2.tv_sec-tv1.tv_sec)*1000000+(tv2.tv_usec-tv1.tv_usec)) / TEST_LOOP;

    printf("g2d blit time %dus, %dfps, %dMpixel/s ........\n", diff, 1000000/diff, test_width * test_height / diff);

   /******************************test alpha blending with Porter-Duff modes ***************************************/
    //Clear: alpha blending mode G2D_ZERO, G2D_ZERO
    //set test data in src buffer
    for(i=0; i<test_height; i++)
    {
         for(j=0; j<test_width; j++)
         {
             char* p = (char *)(((char*)s_buf->buf_vaddr) + (i*test_width+j)*4);
             p[0] = p[1] = p[2] = p[3] = (i*test_width+j) % 255;

             p = (char *)(((char*)d_buf->buf_vaddr) + (i*test_width+j)*4);
             p[0] = p[1] = p[2] = p[3] = ((i*test_width+j + 128) % 255);
         }
    }

    src.blendfunc = G2D_ZERO;
    dst.blendfunc = G2D_ZERO;

    g2d_enable(handle,G2D_BLEND);
    g2d_blit(handle, &src, &dst);
    g2d_disable(handle, G2D_BLEND);

    g2d_finish(handle);

    for(i=0; i<test_height; i++)
    {
         for(j=0; j<test_width; j++)
         {
             char* p = (char *)(((char*)d_buf->buf_vaddr) + (i*test_width+j)*4);

             if(p[0] != 0 || p[0] != p[1] || p[0] != p[2] || p[0] != p[3])
             {
                printf("2d blended r/g/b/a (%d/%d/%d/%d) are not zero in clear mode!\n", p[0], p[1], p[2], p[3]);
             }
         }
    }

    //SRC: alpha blending mode G2D_ONE, G2D_ZERO
    //set test data in src buffer
    for(i=0; i<test_height; i++)
    {
         for(j=0; j<test_width; j++)
         {
             char* p = (char *)(((char*)s_buf->buf_vaddr) + (i*test_width+j)*4);
             p[0] = p[1] = p[2] = p[3] = (i*test_width+j) % 255;

             p = (char *)(((char*)d_buf->buf_vaddr) + (i*test_width+j)*4);
             p[0] = p[1] = p[2] = p[3] = ((i*test_width+j + 128) % 255);
         }
    }

    src.blendfunc = G2D_ONE;
    dst.blendfunc = G2D_ZERO;

    g2d_enable(handle,G2D_BLEND);
    g2d_blit(handle, &src, &dst);
    g2d_disable(handle, G2D_BLEND);

    g2d_finish(handle);

    for(i=0; i<test_height; i++)
    {
         for(j=0; j<test_width; j++)
         {
             unsigned char Cs,As,Co,Ao;
             unsigned char* p = (unsigned char *)(((char*)d_buf->buf_vaddr) + (i*test_width+j)*4);

             if(p[0] != p[1] || p[0] != p[2])
             {
                printf("2d blended r/g/b values(%d/%d/%d) are not same in SRC mode!\n", p[0], p[1], p[2]);
             }

             Co = Ao = Cs = As = (i*test_width+j) % 255;

             if(Co != p[0] || Ao != p[3])
             {
                printf("2d blended color(%d) or alpha(%d) is incorrect in SRC mode, Co %d, Ao %d\n", p[0],p[3],Co,Ao);
             }
         }
    }

    //DST: alpha blending mode G2D_ZERO, G2D_ONE
    //set test data in src buffer
    for(i=0; i<test_height; i++)
    {
         for(j=0; j<test_width; j++)
         {
             char* p = (char *)(((char*)s_buf->buf_vaddr) + (i*test_width+j)*4);
             p[0] = p[1] = p[2] = p[3] = (i*test_width+j) % 255;

             p = (char *)(((char*)d_buf->buf_vaddr) + (i*test_width+j)*4);
             p[0] = p[1] = p[2] = p[3] = ((i*test_width+j + 128) % 255);
         }
    }

    src.blendfunc = G2D_ZERO;
    dst.blendfunc = G2D_ONE;

    g2d_enable(handle,G2D_BLEND);
    g2d_blit(handle, &src, &dst);
    g2d_disable(handle, G2D_BLEND);

    g2d_finish(handle);

    for(i=0; i<test_height; i++)
    {
         for(j=0; j<test_width; j++)
         {
             unsigned char Cd,Ad,Co,Ao;
             unsigned char* p = (unsigned char *)(((char*)d_buf->buf_vaddr) + (i*test_width+j)*4);

             if(p[0] != p[1] || p[0] != p[2])
             {
                printf("2d blended r/g/b values(%d/%d/%d) are not same in DST mode!\n", p[0], p[1], p[2]);
             }

             Co = Ao = Cd = Ad = ((i*test_width+j + 128) % 255);

             if(Co != p[0] || Ao != p[3])
             {
                printf("2d blended color(%d) or alpha(%d) is incorrect in DST mode, Co %d, Ao %d\n", p[0],p[3],Co,Ao);
             }
         }
    }

    //SRC OVER: alpha blending mode G2D_ONE, G2D_ONE_MINUS_SRC_ALPHA
    //set test data in src buffer
    for(i=0; i<test_height; i++)
    {
         for(j=0; j<test_width; j++)
         {
             char* p = (char *)(((char*)s_buf->buf_vaddr) + (i*test_width+j)*4);
             p[0] = p[1] = p[2] = p[3] = (i*test_width+j) % 255;

             p = (char *)(((char*)d_buf->buf_vaddr) + (i*test_width+j)*4);
             p[0] = p[1] = p[2] = p[3] = ((i*test_width+j + 128) % 255);
         }
    }

    src.blendfunc = G2D_ONE;
    dst.blendfunc = G2D_ONE_MINUS_SRC_ALPHA;

    g2d_enable(handle,G2D_BLEND);
    g2d_blit(handle, &src, &dst);
    g2d_disable(handle, G2D_BLEND);

    g2d_finish(handle);

    for(i=0; i<test_height; i++)
    {
         for(j=0; j<test_width; j++)
         {
             unsigned int  iCo, iAo;
             unsigned char Cs,As,Cd,Ad,Co,Ao;
             unsigned char* p = (unsigned char *)(((char*)d_buf->buf_vaddr) + (i*test_width+j)*4);

             if(p[0] != p[1] || p[0] != p[2])
             {
                printf("2d blended r/g/b values(%d/%d/%d) are not same in SRC OVER mode!\n", p[0], p[1], p[2]);
             }

             Cs = As = (i*test_width+j) % 255;
             Cd = Ad = ((i*test_width+j + 128) % 255);

             iCo = ((unsigned int)Cs * 255 + (unsigned int)Cd * (255 - As)) / 255;
             iAo = ((unsigned int)As * 255 + (unsigned int)Ad * (255 - As)) / 255;

             if(iCo > 255) Co = 255;
             else Co = (unsigned char)iCo;

             if(iAo > 255) Ao = 255;
             else Ao = (unsigned char)iAo;

             //compare the result with +/-1 threshold
             if(abs(Co - p[0]) > 2 || abs(Ao - p[3]) > 2)
             {
                printf("2d blended color(%d) or alpha(%d) is incorrect in SRC OVER mode, Cs %d, As %d, Cd %d, Ad %d, Co %d, Ao %d\n", p[0],p[3],Cs,As,Cd,Ad,Co,Ao);
             }
         }
    }

    //DST OVER: alpha blending mode G2D_ONE_MINUS_DST_ALPHA, G2D_ONE
    //set test data in src buffer
    for(i=0; i<test_height; i++)
    {
         for(j=0; j<test_width; j++)
         {
             char* p = (char *)(((char*)s_buf->buf_vaddr) + (i*test_width+j)*4);
             p[0] = p[1] = p[2] = p[3] = (i*test_width+j) % 255;

             p = (char *)(((char*)d_buf->buf_vaddr) + (i*test_width+j)*4);
             p[0] = p[1] = p[2] = p[3] = ((i*test_width+j + 128) % 255);
         }
    }

    src.blendfunc = G2D_ONE_MINUS_DST_ALPHA;
    dst.blendfunc = G2D_ONE;

    g2d_enable(handle,G2D_BLEND);
    g2d_blit(handle, &src, &dst);
    g2d_disable(handle, G2D_BLEND);

    g2d_finish(handle);

    for(i=0; i<test_height; i++)
    {
         for(j=0; j<test_width; j++)
         {
             unsigned int  iCo, iAo;
             unsigned char Cs,As,Cd,Ad,Co,Ao;
             unsigned char* p = (unsigned char *)(((char*)d_buf->buf_vaddr) + (i*test_width+j)*4);

             if(p[0] != p[1] || p[0] != p[2])
             {
                printf("2d blended r/g/b values(%d/%d/%d) are not same in DST OVER mode!\n", p[0], p[1], p[2]);
             }

             Cs = As = (i*test_width+j) % 255;
             Cd = Ad = ((i*test_width+j + 128) % 255);

             iCo = ((unsigned int)Cs * (255 - Ad) + (unsigned int)Cd * 255) / 255;
             iAo = ((unsigned int)As * (255 - Ad) + (unsigned int)Ad * 255) / 255;

             if(iCo > 255) Co = 255;
             else Co = (unsigned char)iCo;

             if(iAo > 255) Ao = 255;
             else Ao = (unsigned char)iAo;

             //compare the result with +/-1 threshold
             if(abs(Co - p[0]) > 2 || abs(Ao - p[3]) > 2)
             {
                printf("2d blended color(%d) or alpha(%d) is incorrect in DST OVER mode, Cs %d, As %d, Cd %d, Ad %d, Co %d, Ao %d\n", p[0],p[3],Cs,As,Cd,Ad,Co,Ao);
             }
         }
    }

    //SRC IN: alpha blending mode G2D_DST_ALPHA, G2D_ZERO
    //set test data in src buffer
    for(i=0; i<test_height; i++)
    {
         for(j=0; j<test_width; j++)
         {
             char* p = (char *)(((char*)s_buf->buf_vaddr) + (i*test_width+j)*4);
             p[0] = p[1] = p[2] = p[3] = (i*test_width+j) % 255;

             p = (char *)(((char*)d_buf->buf_vaddr) + (i*test_width+j)*4);
             p[0] = p[1] = p[2] = p[3] = ((i*test_width+j + 128) % 255);
         }
    }

    src.blendfunc = G2D_DST_ALPHA;
    dst.blendfunc = G2D_ZERO;

    g2d_enable(handle,G2D_BLEND);
    g2d_blit(handle, &src, &dst);
    g2d_disable(handle, G2D_BLEND);

    g2d_finish(handle);

    for(i=0; i<test_height; i++)
    {
         for(j=0; j<test_width; j++)
         {
             unsigned char Cs,As,Ad,Co,Ao;
             char* p = (char *)(((char*)d_buf->buf_vaddr) + (i*test_width+j)*4);

             if(p[0] != p[1] || p[0] != p[2])
             {
                printf("2d blended r/g/b values(%d/%d/%d) are not same in SRC IN mode!\n", p[0], p[1], p[2]);
             }

             Cs = As = (i*test_width+j) % 255;
             Ad = ((i*test_width+j + 128) % 255);

             Co = (unsigned char)(((unsigned int)Cs * Ad) / 255);
             Ao = (unsigned char)(((unsigned int)As * Ad) / 255);

             //compare the result with +/-1 threshold
             if(abs(Co - p[0]) > 2 || abs(Ao - p[3]) > 2)
             {
                printf("2d blended color(%d) or alpha(%d) is incorrect in SRC IN mode, Cs %d, As %d, Ad %d, Co %d, Ao %d\n", p[0],p[3],Cs,As,Ad,Co,Ao);
             }
         }
    }

    //DST IN: alpha blending mode G2D_ZERO, G2D_SRC_ALPHA
    //set test data in src buffer
    for(i=0; i<test_height; i++)
    {
         for(j=0; j<test_width; j++)
         {
             char* p = (char *)(((char*)s_buf->buf_vaddr) + (i*test_width+j)*4);
             p[0] = p[1] = p[2] = p[3] = (i*test_width+j) % 255;

             p = (char *)(((char*)d_buf->buf_vaddr) + (i*test_width+j)*4);
             p[0] = p[1] = p[2] = p[3] = ((i*test_width+j + 128) % 255);
         }
    }

    src.blendfunc = G2D_ZERO;
    dst.blendfunc = G2D_SRC_ALPHA;

    g2d_enable(handle,G2D_BLEND);
    g2d_blit(handle, &src, &dst);
    g2d_disable(handle, G2D_BLEND);

    g2d_finish(handle);

    for(i=0; i<test_height; i++)
    {
         for(j=0; j<test_width; j++)
         {
             unsigned char As,Cd,Ad,Co,Ao;
             char* p = (char *)(((char*)d_buf->buf_vaddr) + (i*test_width+j)*4);

             if(p[0] != p[1] || p[0] != p[2])
             {
                printf("2d blended r/g/b values(%d/%d/%d) are not same in DST IN mode!\n", p[0], p[1], p[2]);
             }

             As = (i*test_width+j) % 255;
             Cd = Ad = ((i*test_width+j + 128) % 255);

             Co = (unsigned char)(((unsigned int)Cd * As) / 255);
             Ao = (unsigned char)(((unsigned int)Ad * As) / 255);

             //compare the result with +/-1 threshold
             if(abs(Co - p[0]) > 2 || abs(Ao - p[3]) > 2)
             {
                printf("2d blended color(%d) or alpha(%d) is incorrect in DST IN mode, As %d, Cd %d, Ad %d, Co %d, Ao %d\n", p[0],p[3],As,Cd,Ad,Co,Ao);
             }
         }
    }

    //SRC OUT: alpha blending mode G2D_ONE_MINUS_DST_ALPHA, G2D_ZERO
    //set test data in src buffer
    for(i=0; i<test_height; i++)
    {
         for(j=0; j<test_width; j++)
         {
             char* p = (char *)(((char*)s_buf->buf_vaddr) + (i*test_width+j)*4);
             p[0] = p[1] = p[2] = p[3] = (i*test_width+j) % 255;

             p = (char *)(((char*)d_buf->buf_vaddr) + (i*test_width+j)*4);
             p[0] = p[1] = p[2] = p[3] = ((i*test_width+j + 128) % 255);
         }
    }

    src.blendfunc = G2D_ONE_MINUS_DST_ALPHA;
    dst.blendfunc = G2D_ZERO;

    g2d_enable(handle,G2D_BLEND);
    g2d_blit(handle, &src, &dst);
    g2d_disable(handle, G2D_BLEND);

    g2d_finish(handle);

    for(i=0; i<test_height; i++)
    {
         for(j=0; j<test_width; j++)
         {
             unsigned char Cs,As,Ad,Co,Ao;
             unsigned char* p = (char *)(((char*)d_buf->buf_vaddr) + (i*test_width+j)*4);

             if(p[0] != p[1] || p[0] != p[2])
             {
                printf("2d blended r/g/b values(%d/%d/%d) are not same in SRC OUT mode!\n", p[0], p[1], p[2]);
             }

             Cs = As = (i*test_width+j) % 255;
             Ad = ((i*test_width+j + 128) % 255);

             Co = (unsigned char)(((unsigned int)Cs * (255 - Ad)) / 255);
             Ao = (unsigned char)(((unsigned int)As * (255 - Ad)) / 255);

             //compare the result with +/-1 threshold
             if(abs(Co - p[0]) > 2 || abs(Ao - p[3]) > 2)
             {
                printf("2d blended color(%d) or alpha(%d) is incorrect in SRC OUT mode, Cs %d, As %d, Ad %d, Co %d, Ao %d\n", p[0],p[3],Cs,As,Ad,Co,Ao);
             }
         }
    }

    //DST OUT: alpha blending mode G2D_ZERO, G2D_ONE_MINUS_SRC_ALPHA
    //set test data in src buffer
    for(i=0; i<test_height; i++)
    {
         for(j=0; j<test_width; j++)
         {
             char* p = (char *)(((char*)s_buf->buf_vaddr) + (i*test_width+j)*4);
             p[0] = p[1] = p[2] = p[3] = (i*test_width+j) % 255;

             p = (char *)(((char*)d_buf->buf_vaddr) + (i*test_width+j)*4);
             p[0] = p[1] = p[2] = p[3] = ((i*test_width+j + 128) % 255);
         }
    }

    src.blendfunc = G2D_ZERO;
    dst.blendfunc = G2D_ONE_MINUS_SRC_ALPHA;

    g2d_enable(handle,G2D_BLEND);
    g2d_blit(handle, &src, &dst);
    g2d_disable(handle, G2D_BLEND);

    g2d_finish(handle);

    for(i=0; i<test_height; i++)
    {
         for(j=0; j<test_width; j++)
         {
             unsigned char As,Cd,Ad,Co,Ao;
             unsigned char* p = (unsigned char *)(((char*)d_buf->buf_vaddr) + (i*test_width+j)*4);

             if(p[0] != p[1] || p[0] != p[2])
             {
                printf("2d blended r/g/b values(%d/%d/%d) are not same in DST OUT mode!\n", p[0], p[1], p[2]);
             }

             As = (i*test_width+j) % 255;
             Cd = Ad = ((i*test_width+j + 128) % 255);

             Co = (unsigned char)(((unsigned int)Cd * (255 - As)) / 255);
             Ao = (unsigned char)(((unsigned int)Ad * (255 - As)) / 255);

             //compare the result with +/-1 threshold
             if(abs(Co - p[0]) > 2 || abs(Ao - p[3]) > 2)
             {
                printf("2d blended color(%d) or alpha(%d) is incorrect in DST OUT mode, As %d, Cd %d, Ad %d, Co %d, Ao %d\n", p[0],p[3],As,Cd,Ad,Co,Ao);
             }
         }
    }

    //SRC ATOP: alpha blending mode G2D_DST_ALPHA, G2D_ONE_MINUS_SRC_ALPHA
    //set test data in src buffer
    for(i=0; i<test_height; i++)
    {
         for(j=0; j<test_width; j++)
         {
             char* p = (char *)(((char*)s_buf->buf_vaddr) + (i*test_width+j)*4);
             p[0] = p[1] = p[2] = p[3] = (i*test_width+j) % 255;

             p = (char *)(((char*)d_buf->buf_vaddr) + (i*test_width+j)*4);
             p[0] = p[1] = p[2] = p[3] = ((i*test_width+j + 128) % 255);
         }
    }

    src.blendfunc = G2D_DST_ALPHA;
    dst.blendfunc = G2D_ONE_MINUS_SRC_ALPHA;

    g2d_enable(handle,G2D_BLEND);
    g2d_blit(handle, &src, &dst);
    g2d_disable(handle, G2D_BLEND);

    g2d_finish(handle);

    for(i=0; i<test_height; i++)
    {
         for(j=0; j<test_width; j++)
         {
             unsigned int  iCo, iAo;
             unsigned char Cs,As,Cd,Ad,Co,Ao;
             unsigned char* p = (unsigned char *)(((char*)d_buf->buf_vaddr) + (i*test_width+j)*4);

             if(p[0] != p[1] || p[0] != p[2])
             {
                printf("2d blended r/g/b values(%d/%d/%d) are not same in SRC ATOP mode!\n", p[0], p[1], p[2]);
             }

             Cs = As = (i*test_width+j) % 255;
             Cd = Ad = ((i*test_width+j + 128) % 255);

             iCo = ((unsigned int)Cs * Ad + (unsigned int)Cd * (255 - As)) / 255;
             iAo = ((unsigned int)As * Ad + (unsigned int)Ad * (255 - As)) / 255;

             if(iCo > 255) Co = 255;
             else Co = (unsigned char)iCo;

             if(iAo > 255) Ao = 255;
             else Ao = (unsigned char)iAo;

             //compare the result with +/-1 threshold
             if(abs(Co - p[0]) > 2 || abs(Ao - p[3]) > 2)
             {
                printf("2d blended color(%d) or alpha(%d) is incorrect in SRC ATOP mode, Cs %d, As %d, Cd %d, Ad %d, Co %d, Ao %d\n", p[0],p[3],Cs,As,Cd,Ad,Co,Ao);
             }
         }
    }

    //DST ATOP: alpha blending mode G2D_ONE_MINUS_DST_ALPHA, G2D_SRC_ALPHA
    //set test data in src buffer
    for(i=0; i<test_height; i++)
    {
         for(j=0; j<test_width; j++)
         {
             char* p = (char *)(((char*)s_buf->buf_vaddr) + (i*test_width+j)*4);
             p[0] = p[1] = p[2] = p[3] = (i*test_width+j) % 255;

             p = (char *)(((char*)d_buf->buf_vaddr) + (i*test_width+j)*4);
             p[0] = p[1] = p[2] = p[3] = ((i*test_width+j + 128) % 255);
         }
    }

    src.blendfunc = G2D_ONE_MINUS_DST_ALPHA;
    dst.blendfunc = G2D_SRC_ALPHA;

    g2d_enable(handle,G2D_BLEND);
    g2d_blit(handle, &src, &dst);
    g2d_disable(handle, G2D_BLEND);

    g2d_finish(handle);

    for(i=0; i<test_height; i++)
    {
         for(j=0; j<test_width; j++)
         {
             unsigned int  iCo, iAo;
             unsigned char Cs,As,Cd,Ad,Co,Ao;
             unsigned char* p = (unsigned char *)(((char*)d_buf->buf_vaddr) + (i*test_width+j)*4);

             if(p[0] != p[1] || p[0] != p[2])
             {
                printf("2d blended r/g/b values(%d/%d/%d) are not same in DST ATOP mode!\n", p[0], p[1], p[2]);
             }

             Cs = As = (i*test_width+j) % 255;
             Cd = Ad = ((i*test_width+j + 128) % 255);

             iCo = ((unsigned int)Cs * (255 - Ad) + (unsigned int)Cd * As) / 255;
             iAo = ((unsigned int)As * (255 - Ad) + (unsigned int)Ad * As) / 255;

             if(iCo > 255) Co = 255;
             else Co = (unsigned char)iCo;

             if(iAo > 255) Ao = 255;
             else Ao = (unsigned char)iAo;

             //compare the result with +/-1 threshold
             if(abs(Co - p[0]) > 2 || abs(Ao - p[3]) > 2)
             {
                printf("2d blended color(%d) or alpha(%d) is incorrect in DST ATOP mode, Cs %d, As %d, Cd %d, Ad %d, Co %d, Ao %d\n", p[0],p[3],Cs,As,Cd,Ad,Co,Ao);
             }
         }
    }

    //XOR: test alpha blending mode G2D_ONE_MINUS_DST_ALPHA, G2D_ONE_MINUS_SRC_ALPHA
    //set test data in src buffer
    for(i=0; i<test_height; i++)
    {
         for(j=0; j<test_width; j++)
         {
             char* p = (char *)(((char*)s_buf->buf_vaddr) + (i*test_width+j)*4);
             p[0] = p[1] = p[2] = p[3] = (i*test_width+j) % 255;

             p = (char *)(((char*)d_buf->buf_vaddr) + (i*test_width+j)*4);
             p[0] = p[1] = p[2] = p[3] = ((i*test_width+j + 128) % 255);
         }
    }

    src.blendfunc = G2D_ONE_MINUS_DST_ALPHA;
    dst.blendfunc = G2D_ONE_MINUS_SRC_ALPHA;

    g2d_enable(handle,G2D_BLEND);
    g2d_blit(handle, &src, &dst);
    g2d_disable(handle, G2D_BLEND);

    g2d_finish(handle);

    for(i=0; i<test_height; i++)
    {
         for(j=0; j<test_width; j++)
         {
             unsigned int  iCo, iAo;
             unsigned char Cs,As,Cd,Ad,Co,Ao;
             unsigned char* p = (unsigned char *)(((char*)d_buf->buf_vaddr) + (i*test_width+j)*4);

             if(p[0] != p[1] || p[0] != p[2])
             {
                printf("2d blended r/g/b values(%d/%d/%d) are not same in XOR mode!\n", p[0], p[1], p[2]);
             }

             Cs = As = (i*test_width+j) % 255;
             Cd = Ad = ((i*test_width+j + 128) % 255);

             iCo = ((unsigned int)Cs * (255 - Ad) + (unsigned int)Cd * (255 -As)) / 255;
             iAo = ((unsigned int)As * (255 - Ad) + (unsigned int)Ad * (255 -As)) / 255;

             if(iCo > 255) Co = 255;
             else Co = (unsigned char)iCo;

             if(iAo > 255) Ao = 255;
             else Ao = (unsigned char)iAo;

             //compare the result with +/-1 threshold
             if(abs(Co - p[0]) > 2 || abs(Ao - p[3]) > 2)
             {
                printf("2d blended color(%d) or alpha(%d) is incorrect in XOR mode, Cs %d, As %d, Cd %d, Ad %d, Co %d, Ao %d\n", p[0],p[3],Cs,As,Cd,Ad,Co,Ao);
             }
         }
    }

    //Global Alpha: alpha blending mode G2D_ZERO, G2D_SRC_ALPHA
    //set test data in src buffer
    for(i=0; i<test_height; i++)
    {
         for(j=0; j<test_width; j++)
         {
             char* p = (char *)(((char*)s_buf->buf_vaddr) + (i*test_width+j)*4);
             p[0] = p[1] = p[2] = p[3] = (i*test_width+j) % 255;

             p = (char *)(((char*)d_buf->buf_vaddr) + (i*test_width+j)*4);
             p[0] = p[1] = p[2] = p[3] = ((i*test_width+j + 128) % 255);
         }
    }

    src.blendfunc = G2D_ZERO;
    dst.blendfunc = G2D_SRC_ALPHA;

    src.global_alpha = 0xab;
    dst.global_alpha = 0xff;

    g2d_enable(handle,G2D_BLEND);
    g2d_enable(handle,G2D_GLOBAL_ALPHA);

    g2d_blit(handle, &src, &dst);

    g2d_disable(handle,G2D_GLOBAL_ALPHA);
    g2d_disable(handle, G2D_BLEND);

    g2d_finish(handle);

    for(i=0; i<test_height; i++)
    {
         for(j=0; j<test_width; j++)
         {
             unsigned char Cs,As,Cd,Ad,Co_on_imx6,Ao_on_imx6,Co_on_imx8,Ao_on_imx8;
             unsigned char* p = (unsigned char *)(((char*)d_buf->buf_vaddr) + (i*test_width+j)*4);

             if(p[0] != p[1] || p[0] != p[2])
             {
                printf("2d blended r/g/b values(%d/%d/%d) are not same in SRC IN mode!\n", p[0], p[1], p[2]);
             }

             Cs = As = (i*test_width+j) % 255;
             Cd = Ad = ((i*test_width+j + 128) % 255);

             /* different behaviour of global alpha between imx6 and imx8 */
             Co_on_imx6 = (unsigned char)(((unsigned int)Cd * As * src.global_alpha) / (255*255));
             Ao_on_imx6 = (unsigned char)(((unsigned int)Ad * As * src.global_alpha) / (255*255));

             Co_on_imx8 = (unsigned char)(((unsigned int)Cd * src.global_alpha) / 255);
             Ao_on_imx8 = (unsigned char)(((unsigned int)Ad * src.global_alpha) / 255);

             //compare the result with +/-1 threshold
             if((abs(Co_on_imx6 - p[0]) > 2 || abs(Ao_on_imx6 - p[3]) > 2) && (abs(Co_on_imx8 - p[0]) > 2 || abs(Ao_on_imx8 - p[3]) > 2))
             {
                printf("2d blended color(%d) or alpha(%d) is incorrect in DST IN mode, Cs %d, As %d, Ad %d, Co %d, Ao %d\n", 
                        p[0],p[3],Cs,As,Ad,Co_on_imx6,Ao_on_imx6);
             }
         }
    }

    //Global Alpha: alpha blending mode G2D_ONE, G2D_ONE_MINUS_SRC_ALPHA
    //set test data in src buffer
    for(i=0; i<test_height; i++)
    {
         for(j=0; j<test_width; j++)
         {
             char* p = (char *)(((char*)s_buf->buf_vaddr) + (i*test_width+j)*4);
             p[0] = p[1] = p[2] = p[3] = (i*test_width+j + 64) % 255;

             p = (char *)(((char*)d_buf->buf_vaddr) + (i*test_width+j)*4);
             p[0] = p[1] = p[2] = p[3] = ((i*test_width+j + 128) % 255);
         }
    }

    src.blendfunc = G2D_ONE;
    dst.blendfunc = G2D_ONE_MINUS_SRC_ALPHA;

    src.global_alpha = 0x69;
    dst.global_alpha = 0xff;

    g2d_enable(handle,G2D_BLEND);
    g2d_enable(handle,G2D_GLOBAL_ALPHA);

    g2d_blit(handle, &src, &dst);

    g2d_disable(handle,G2D_GLOBAL_ALPHA);
    g2d_disable(handle,G2D_BLEND);

    g2d_finish(handle);

    for(i=0; i<test_height; i++)
    {
         for(j=0; j<test_width; j++)
         {
             unsigned int k, iCo_on_imx6, iAo_on_imx6, iCo_on_imx8, iAo_on_imx8;
             unsigned char Cs,As,Cd,Ad,Co_on_imx6,Ao_on_imx6,Co_on_imx8,Ao_on_imx8;

             unsigned char* p = (unsigned char *)(((char*)d_buf->buf_vaddr) + (i*test_width+j)*4);

             if(p[0] != p[1] || p[0] != p[2])
             {
                printf("2d blended r/g/b values(%d/%d/%d) are not same in SRC IN mode!\n", p[0], p[1], p[2]);
             }

             Cs = As = ((i*test_width+j + 64) % 255);
             Cd = Ad = ((i*test_width+j + 128) % 255);

             /* different behaviour of global alpha between imx6 and imx8 */
             iCo_on_imx6 = ((unsigned int)Cs * src.global_alpha + (unsigned int)Cd * (255 - (As * src.global_alpha / 255))) / 255;
             iAo_on_imx6 = ((unsigned int)(As * src.global_alpha / 255) * 255 + (unsigned int)Ad * (255 - (As * src.global_alpha / 255))) / 255;

             iCo_on_imx8 = ((unsigned int)Cs * src.global_alpha + (unsigned int)Cd * (255 - src.global_alpha)) / 255;
             iAo_on_imx8 = ((unsigned int)As * src.global_alpha + (unsigned int)Ad * (255 - src.global_alpha)) / 255;

             Co_on_imx6 = (iCo_on_imx6 > 255) ? 255 : (unsigned char)iCo_on_imx6;
             Ao_on_imx6 = (iAo_on_imx6 > 255) ? 255 : (unsigned char)iAo_on_imx6;
             Co_on_imx8 = (iCo_on_imx8 > 255) ? 255 : (unsigned char)iCo_on_imx8;
             Ao_on_imx8 = (iAo_on_imx8 > 255) ? 255 : (unsigned char)iAo_on_imx8;

             //compare the result with +/-1 threshold
             if((abs(Co_on_imx6 - p[0]) > 2 || abs(Ao_on_imx6 - p[3]) > 2) && (abs(Co_on_imx8 - p[0]) > 2 || abs(Ao_on_imx8 - p[3]) > 2))
             {
                printf("2d blended color(%d) or alpha(%d) is incorrect Cs %d, As %d, Cd %d, Ad %d, Co %d, Ao %d, global alpha=%d\n",
                            p[0],p[3],Cs,As,Cd,Ad,Co_on_imx6,Ao_on_imx6, src.global_alpha);
             }
         }
    }

    //Pre-multipied & de-muliply test: alpha blending mode G2D_ONE, G2D_ONE_MINUS_SRC_ALPHA
    //set test data in src buffer
    for(i=0; i<test_height; i++)
    {
         for(j=0; j<test_width; j++)
         {
             char* p = (char *)(((char*)s_buf->buf_vaddr) + (i*test_width+j)*4);
             p[0] = p[1] = p[2] = p[3] = (i*test_width+j) % 255;

             p = (char *)(((char*)d_buf->buf_vaddr) + (i*test_width+j)*4);
             p[0] = p[1] = p[2] = p[3] = ((i*test_width+j + 128) % 255);
         }
    }

    src.blendfunc = G2D_ONE | G2D_PRE_MULTIPLIED_ALPHA;
    dst.blendfunc = G2D_ONE_MINUS_SRC_ALPHA | G2D_PRE_MULTIPLIED_ALPHA;

    g2d_enable(handle,G2D_BLEND);
    g2d_blit(handle, &src, &dst);
    g2d_disable(handle, G2D_BLEND);

    g2d_finish(handle);

    for(i=0; i<test_height; i++)
    {
         for(j=0; j<test_width; j++)
         {
             unsigned int  iCo, iAo;
             unsigned char Cs,As,Cd,Ad,Co,Ao;
             unsigned char* p = (unsigned char *)(((char*)d_buf->buf_vaddr) + (i*test_width+j)*4);

             if(p[0] != p[1] || p[0] != p[2])
             {
                printf("2d blended r/g/b values(%d/%d/%d) are not same in premultiped & demuliply SRC OVER mode!\n", p[0], p[1], p[2]);
             }

             Cs = As = (i*test_width+j) % 255;
             Cd = Ad = ((i*test_width+j + 128) % 255);

             iCo = ((unsigned int)Cs * As * 255 + (unsigned int)Cd * Ad * (255 - As)) / (255 * 255);
             iAo = ((unsigned int)As * 255 + (unsigned int)Ad * (255 - As)) / 255;


             if(iCo > 255) Co = 255;
             else Co = (unsigned char)iCo;

             if(iAo > 255) Ao = 255;
             else Ao = (unsigned char)iAo;

             //compare the result with +/-2 threshold
             if(abs(Co - p[0]) > 4 || abs(Ao - p[3]) > 1)
             {
                printf("2d blended color(%d) or alpha(%d) is incorrect in premultiped & demuliply SRC OVER mode, Cs %d, As %d, Cd %d, Ad %d, Co %d, Ao %d\n", p[0],p[3],Cs,As,Cd,Ad,Co,Ao);
             }
         }
    }

    gettimeofday(&tv1, NULL);

    for(i=0; i<TEST_LOOP; i++)
    {
       g2d_enable(handle,G2D_BLEND);
       g2d_enable(handle,G2D_GLOBAL_ALPHA);

       g2d_blit(handle, &src, &dst);

       g2d_disable(handle,G2D_GLOBAL_ALPHA);
       g2d_disable(handle, G2D_BLEND);
    }

    g2d_finish(handle);

    gettimeofday(&tv2, NULL);
    diff = ((tv2.tv_sec-tv1.tv_sec)*1000000+(tv2.tv_usec-tv1.tv_usec)) / TEST_LOOP;

    printf("g2d blending time %dus, %dfps, %dMpixel/s ........\n", diff, 1000000/diff, test_width * test_height / diff);

    //BLIT dim: alpha blending mode G2D_ONE, G2D_ONE_MINUS_SRC_AlPHA
    //set test data in dst buffer
    for(i=0; i<test_height; i++)
    {
         for(j=0; j<test_width; j++)
         {
             char *p = (char *)(((char*)d_buf->buf_vaddr) + (i*test_width+j)*4);
             p[0] = p[1] = p[2] = p[3] = ((i*test_width+j + 128) % 255);
         }
    }

    src.planes[0] = 0x0;

    src.blendfunc = G2D_ONE;
    dst.blendfunc = G2D_ONE_MINUS_SRC_ALPHA;

    src.clrcolor     = 0xabcdabcd;
    src.global_alpha = 0x69;
    dst.global_alpha = 0xff;

    g2d_enable(handle,G2D_BLEND_DIM);
    g2d_enable(handle,G2D_GLOBAL_ALPHA);

    g2d_blit(handle, &src, &dst);

    g2d_disable(handle,G2D_GLOBAL_ALPHA);
    g2d_disable(handle, G2D_BLEND_DIM);

    g2d_finish(handle);

    for(i=0; i<test_height; i++)
    {
         for(j=0; j<test_width; j++)
         {
             unsigned int k, iCo_on_imx6, iAo_on_imx6, iCo_on_imx8, iAo_on_imx8;
             unsigned char Cs,As,Cd,Ad,Co_on_imx6,Ao_on_imx6,Co_on_imx8,Ao_on_imx8;

             unsigned char* p = (unsigned char *)(((char*)d_buf->buf_vaddr) + (i*test_width+j)*4);

             As = (src.clrcolor >> 24) & 0xff;
             Cd = Ad = ((i*test_width+j + 128) % 255);

             for(k = 0; k < 3; k++)
             {
                 Cs = (src.clrcolor >> (k * 8)) & 0xff;

                 /* different behaviour of global alpha between imx6 and imx8 */
                 iCo_on_imx6 = ((unsigned int)Cs * 255 + (unsigned int)Cd * (255 - (As * src.global_alpha / 255))) / 255;
                 iAo_on_imx6 = ((unsigned int)(As * src.global_alpha / 255) * 255 + (unsigned int)Ad * (255 - (As * src.global_alpha / 255))) / 255;

                 iCo_on_imx8 = ((unsigned int)Cs * 255 + (unsigned int)Cd * (255 - src.global_alpha)) / 255;
                 iAo_on_imx8 = ((unsigned int)As * src.global_alpha + (unsigned int)Ad * (255 - src.global_alpha)) / 255;

                 Co_on_imx6 = (iCo_on_imx6 > 255) ? 255 : (unsigned char)iCo_on_imx6;
                 Ao_on_imx6 = (iAo_on_imx6 > 255) ? 255 : (unsigned char)iAo_on_imx6;
                 Co_on_imx8 = (iCo_on_imx8 > 255) ? 255 : (unsigned char)iCo_on_imx8;
                 Ao_on_imx8 = (iAo_on_imx8 > 255) ? 255 : (unsigned char)iAo_on_imx8;

                 //compare the result with +/-1 threshold
                 if((abs(Co_on_imx6 - p[k]) > 2 || abs(Ao_on_imx6 - p[3]) > 2) && (abs(Co_on_imx8 - p[k]) > 2 || abs(Ao_on_imx8 - p[3]) > 2))
                 {
                    printf("2d blended dim color(%d) or alpha(%d) is incorrect k=%d, Cs %d, As %d, Cd %d, Ad %d, Co %d, Ao %d, global alpha=%d\n",
                                p[k],p[3],k, Cs,As,Cd,Ad,Co_on_imx6,Ao_on_imx6, src.global_alpha);
                 }
             }
         }
    }

    gettimeofday(&tv1, NULL);

    for(i=0; i<TEST_LOOP; i++)
    {
       g2d_enable(handle,G2D_BLEND_DIM);
       g2d_enable(handle,G2D_GLOBAL_ALPHA);

       g2d_blit(handle, &src, &dst);

       g2d_disable(handle,G2D_GLOBAL_ALPHA);
       g2d_disable(handle, G2D_BLEND_DIM);
    }

    g2d_finish(handle);

    gettimeofday(&tv2, NULL);
    diff = ((tv2.tv_sec-tv1.tv_sec)*1000000+(tv2.tv_usec-tv1.tv_usec)) / TEST_LOOP;

    printf("g2d blend-dim time %dus, %dfps, %dMpixel/s ........\n", diff, 1000000/diff, test_width * test_height / diff);

/****************************************** test g2d_clear *********************************************************/
    //set garbage data in dst buffer
    memset(d_buf->buf_vaddr, 0xcd, test_width * test_height * 4);

    dst.clrcolor = 0xffeeddcc;

    g2d_clear(handle, &dst);

    g2d_finish(handle);

    //check if the generated color is correct
    for(i=0; i<test_width * test_height; i++)
    {
        int clrcolor = *(int *)(((char*)d_buf->buf_vaddr) + i*4);
        if(clrcolor != dst.clrcolor)
        {
            printf("[%d] Clear color 0x%x, Error color 0x%x\n", i, dst.clrcolor, clrcolor);
        }
    }

    printf("---------------- g2d clear performance ----------------\n");
    gettimeofday(&tv1, NULL);

    for(i=0; i<TEST_LOOP; i++)
    {
        g2d_clear(handle, &dst);
    }

    g2d_finish(handle);

    gettimeofday(&tv2, NULL);
    diff = ((tv2.tv_sec-tv1.tv_sec)*1000000+(tv2.tv_usec-tv1.tv_usec)) / TEST_LOOP;

    printf("g2d clear time %dus, %dfps, %dMpixel/s ........\n", diff, 1000000/diff, test_width * test_height / diff);

/****************************************** test g2d rotation *********************************************************/
#if 0//the disabled code is for debug purpose only
    memset(s_buf->buf_vaddr, 0x0, test_width * test_height * 4);
    FILE *fd = fopen("./antutu_mx6dl_1G_dual_core.fb0", "rb");
    fread(s_buf->buf_vaddr, 1280*720*4, 1, fd);
    fclose(fd);

    src.left   = 0;
    src.top    = 0;
    src.right  = 1280;
    src.bottom = 720;
    src.stride = 1280;
    src.format = G2D_RGBA8888;
    src.rot    = G2D_ROTATION_0;

    dst.right  = 1280;
    dst.bottom = 720;
    dst.stride = 1280;
    dst.format = G2D_RGBA8888;
    dst.rot    = G2D_FLIP_H;

    g2d_blit(handle, &src, &dst);

    fd = fopen("./antutu_mx6dl_1G_dual_core_flip_h.fb0", "wb");
    fwrite(d_buf->buf_vaddr, 1280*720*4, 1, fd);
    fclose(fd);
#endif

    //set test data in src buffer
    for(i=0; i<test_height; i++)
    {
         for(j=0; j<test_width; j++)
         {
             *(int *)(((char*)s_buf->buf_vaddr) + (i*test_width+j)*4) = i*test_width+j;
         }
    }

    //1. 90 degree rotation test
    //set garbage data in dst buffer
    memset(d_buf->buf_vaddr, 0xcd, test_width * test_height * 4);

    src.left   = 0;
    src.top    = 0;
    src.right  = test_width;
    src.bottom = test_height;
    src.stride = test_width;
    src.width  = test_width;
    src.height = test_height;
    src.format = G2D_RGBA8888;
    src.rot    = G2D_ROTATION_0;
    src.planes[0] = s_buf->buf_paddr;

    src.left   = 0;
    src.top    = 0;
    dst.right  = test_height;
    dst.bottom = test_width;
    dst.stride = test_height;
    dst.width  = test_height;
    dst.height = test_width;
    dst.format = G2D_RGBA8888;
    dst.rot    = G2D_ROTATION_90;

    g2d_blit(handle, &src, &dst);

    g2d_finish(handle);

    //check if the dst buffer is rotated by 90
    for(i=0; i<test_width; i++)
    {
         for(j=0; j<test_height; j++)
         {
             int correct_val = (test_height - 1 - j) * test_width + i;
             int rotated_val = *(int *)(((char*)d_buf->buf_vaddr) + (i*test_height+j)*4);
             if(rotated_val != correct_val)
             {
                  printf("[%d][%d]: 90 rotation value should be %d instead of %d(0x%x)\n", i, j, correct_val, rotated_val, rotated_val);
             }
         }
    }

    printf("---------------- g2d rotation performance ----------------\n");
    gettimeofday(&tv1, NULL);

    for(i=0; i<TEST_LOOP; i++)
    {
        g2d_blit(handle, &src, &dst);
    }

    g2d_finish(handle);

    gettimeofday(&tv2, NULL);
    diff = ((tv2.tv_sec-tv1.tv_sec)*1000000+(tv2.tv_usec-tv1.tv_usec)) / TEST_LOOP;

    printf("90 rotation time %dus, %dfps, %dMpixel/s ........\n", diff, 1000000/diff, test_width * test_height / diff);

    //2. 180 degree rotation test
    //set garbage data in dst buffer
    memset(d_buf->buf_vaddr, 0xcd, test_width * test_height * 4);

    src.left   = 0;
    src.top    = 0;
    src.right  = test_width;
    src.bottom = test_height;
    src.stride = test_width;
    src.format = G2D_RGBA8888;
    src.rot    = G2D_ROTATION_0;

    src.left   = 0;
    src.top    = 0;
    dst.right  = test_width;
    dst.bottom = test_height;
    dst.stride = test_width;
    dst.width  = test_width;
    dst.height = test_height;
    dst.format = G2D_RGBA8888;
    dst.rot    = G2D_ROTATION_180;

    g2d_blit(handle, &src, &dst);

    g2d_finish(handle);

    //check if the dst buffer is rotated by 180
    for(i=0; i<test_height; i++)
    {
         for(j=0; j<test_width; j++)
         {
             int correct_val = (test_height -1 - i) * test_width + (test_width -1 - j);
             int rotated_val = *(int *)(((char*)d_buf->buf_vaddr) + (i*test_width+j)*4);
             if(rotated_val != correct_val)
             {
                  printf("[%d][%d]: 180 rotation value should be %d instead of %d(0x%x)\n", i, j, correct_val, rotated_val, rotated_val);
             }
         }
    }

    gettimeofday(&tv1, NULL);

    for(i=0; i<TEST_LOOP; i++)
    {
        g2d_blit(handle, &src, &dst);
    }

    g2d_finish(handle);

    gettimeofday(&tv2, NULL);
    diff = ((tv2.tv_sec-tv1.tv_sec)*1000000+(tv2.tv_usec-tv1.tv_usec)) / TEST_LOOP;

    printf("180 rotation time %dus, %dfps, %dMpixel/s ........\n", diff, 1000000/diff, test_width * test_height / diff);

    //3. 270 degree rotation test
    //set garbage data in dst buffer
    memset(d_buf->buf_vaddr, 0xcd, test_width * test_height * 4);

    src.left   = 0;
    src.top    = 0;
    src.right  = test_width;
    src.bottom = test_height;
    src.stride = test_width;
    src.format = G2D_RGBA8888;
    src.rot    = G2D_ROTATION_0;

    src.left   = 0;
    src.top    = 0;
    dst.right  = test_height;
    dst.bottom = test_width;
    dst.stride = test_height;
    dst.width  = test_height;
    dst.height = test_width;
    dst.format = G2D_RGBA8888;
    dst.rot    = G2D_ROTATION_270;

    g2d_blit(handle, &src, &dst);

    g2d_finish(handle);

    //check if the dst buffer is rotated by 270
    for(i=0; i<test_width; i++)
    {
         for(j=0; j<test_height; j++)
         {
             int correct_val = test_width * j + (test_width -1 - i);
             int rotated_val = *(int *)(((char*)d_buf->buf_vaddr) + (i*test_height+j)*4);
             if(rotated_val != correct_val)
             {
                  printf("[%d][%d]: 270 rotation value should be %d instead of %d(0x%x)\n", i, j, correct_val, rotated_val, rotated_val);
             }
         }
    }

    gettimeofday(&tv1, NULL);

    for(i=0; i<TEST_LOOP; i++)
    {
        g2d_blit(handle, &src, &dst);
    }

    g2d_finish(handle);

    gettimeofday(&tv2, NULL);
    diff = ((tv2.tv_sec-tv1.tv_sec)*1000000+(tv2.tv_usec-tv1.tv_usec)) / TEST_LOOP;

    printf("270 rotation time %dus, %dfps, %dMpixel/s ........\n", diff, 1000000/diff, test_width * test_height / diff);

    //4. flip h test
    //set garbage data in dst buffer
    memset(d_buf->buf_vaddr, 0xcd, test_width * test_height * 4);

    src.left   = 0;
    src.top    = 0;
    src.right  = test_width;
    src.bottom = test_height;
    src.stride = test_width;
    src.format = G2D_RGBA8888;
    src.rot    = G2D_ROTATION_0;

    src.left   = 0;
    src.top    = 0;
    dst.right  = test_width;
    dst.bottom = test_height;
    dst.stride = test_width;
    dst.width  = test_width;
    dst.height = test_height;
    dst.format = G2D_RGBA8888;
    dst.rot    = G2D_FLIP_H;

    g2d_blit(handle, &src, &dst);

    g2d_finish(handle);

    //check if the dst buffer is h-flipped
    for(i=0; i<test_height; i++)
    {
         for(j=0; j<test_width; j++)
         {
             int correct_val = test_width*i +(test_width -1 -j);
             int rotated_val = *(int *)(((char*)d_buf->buf_vaddr) + (i*test_width+j)*4);
             if(rotated_val != correct_val)
             {
                  printf("[%d][%d]: flip-h value should be %d instead of %d(0x%x)\n", i, j, correct_val, rotated_val, rotated_val);
             }
         }
    }

    gettimeofday(&tv1, NULL);

    for(i=0; i<TEST_LOOP; i++)
    {
        g2d_blit(handle, &src, &dst);
    }

    g2d_finish(handle);

    gettimeofday(&tv2, NULL);
    diff = ((tv2.tv_sec-tv1.tv_sec)*1000000+(tv2.tv_usec-tv1.tv_usec)) / TEST_LOOP;

    printf("g2d flip-h time %dus, %dfps, %dMpixel/s ........\n", diff, 1000000/diff, test_width * test_height / diff);

    //5. flip v test
    //set garbage data in dst buffer
    memset(d_buf->buf_vaddr, 0xcd, test_width * test_height * 4);

    src.left   = 0;
    src.top    = 0;
    src.right  = test_width;
    src.bottom = test_height;
    src.stride = test_width;
    src.format = G2D_RGBA8888;
    src.rot    = G2D_ROTATION_0;

    src.left   = 0;
    src.top    = 0;
    dst.right  = test_width;
    dst.bottom = test_height;
    dst.stride = test_width;
    dst.format = G2D_RGBA8888;
    dst.rot    = G2D_FLIP_V;

    g2d_blit(handle, &src, &dst);

    g2d_finish(handle);

    //check if the dst buffer is v-flipped
    for(i=0; i<test_height; i++)
    {
         for(j=0; j<test_width; j++)
         {
             int correct_val = test_width*(test_height -1 -i) +j;
             int rotated_val = *(int *)(((char*)d_buf->buf_vaddr) + (i*test_width+j)*4);
             if(rotated_val != correct_val)
             {
                  printf("[%d][%d]: flip-v value should be %d instead of %d(0x%x)\n", i, j, correct_val, rotated_val, rotated_val);
             }
         }
    }

    gettimeofday(&tv1, NULL);

    for(i=0; i<TEST_LOOP; i++)
    {
        g2d_blit(handle, &src, &dst);
    }

    g2d_finish(handle);

    gettimeofday(&tv2, NULL);
    diff = ((tv2.tv_sec-tv1.tv_sec)*1000000+(tv2.tv_usec-tv1.tv_usec)) / TEST_LOOP;

    printf("g2d flip-v time %dus, %dfps, %dMpixel/s ........\n", diff, 1000000/diff, test_width * test_height / diff);

/****************************************** test g2d resize performance *********************************************************/
    printf("---------------- g2d resize test performance ----------------\n");

    src.left = 0;
    src.top = 0;
    src.right = (test_width > 1280) ? 1280 : test_width >> 1;
    src.bottom = (test_height > 720) ? 720 : test_height >> 1;
    src.stride = (test_width > 1280) ? 1280 : test_width >> 1;
    src.width  = (test_width > 1280) ? 1280 : test_width >> 1;
    src.height = (test_height > 720) ? 720 : test_height >> 1;
    src.rot    = G2D_ROTATION_0;
    src.format = G2D_RGBA8888;

    dst.left = 0;
    dst.top = 0;
    dst.right = test_width;
    dst.bottom = test_height;
    dst.stride = test_width;
    dst.width  = test_width;
    dst.height = test_height;
    dst.rot    = G2D_ROTATION_0;
    dst.format = G2D_RGBA8888;

    printf("g2d resize test from %dx%d to %dx%d: \n", src.width, src.height, dst.width, dst.height);

    gettimeofday(&tv1, NULL);

    for(i=0; i<TEST_LOOP; i++)
    {
        g2d_blit(handle, &src, &dst);
    }

    g2d_finish(handle);

    gettimeofday(&tv2, NULL);
    diff = ((tv2.tv_sec-tv1.tv_sec)*1000000+(tv2.tv_usec-tv1.tv_usec)) / TEST_LOOP;

    printf("resize format from rgba8888 to rgba8888, time %dus, %dfps, %dMpixel/s ........\n", diff, 1000000/diff, test_width * test_height / diff);

    src.format = G2D_NV12;

    gettimeofday(&tv1, NULL);

    for(i=0; i<TEST_LOOP; i++)
    {
        g2d_blit(handle, &src, &dst);
    }

    g2d_finish(handle);

    gettimeofday(&tv2, NULL);
    diff = ((tv2.tv_sec-tv1.tv_sec)*1000000+(tv2.tv_usec-tv1.tv_usec)) / TEST_LOOP;

    printf("resize format from nv12 to rgba8888, time %dus, %dfps, %dMpixel/s ........\n", diff, 1000000/diff, test_width * test_height / diff);

    src.left = 0;
    src.top = 0;
    src.right = test_width;
    src.bottom = test_height;
    src.stride = test_width;
    src.width  = test_width;
    src.height = test_height;
    src.rot    = G2D_ROTATION_0;
    src.format = G2D_RGBA8888;

    dst.left = 0;
    dst.top = 0;
    dst.right = (test_width > 1280) ? 1280 : test_width >> 1;
    dst.bottom = (test_height > 720) ? 720 : test_height >> 1;
    dst.stride = (test_width > 1280) ? 1280 : test_width >> 1;
    dst.width  = (test_width > 1280) ? 1280 : test_width >> 1;
    dst.height = (test_height > 720) ? 720 : test_height >> 1;
    dst.rot    = G2D_ROTATION_0;
    dst.format = G2D_RGBA8888;

    printf("g2d resize test from %dx%d to %dx%d: \n", src.width, src.height, dst.width, dst.height);

    gettimeofday(&tv1, NULL);

    for(i=0; i<TEST_LOOP; i++)
    {
        g2d_blit(handle, &src, &dst);
    }

    g2d_finish(handle);

    gettimeofday(&tv2, NULL);
    diff = ((tv2.tv_sec-tv1.tv_sec)*1000000+(tv2.tv_usec-tv1.tv_usec)) / TEST_LOOP;

    printf("resize format from rgba8888 to rgba8888, time %dus, %dfps, %dMpixel/s ........\n", diff, 1000000/diff, test_width * test_height / diff);

    src.format = G2D_NV12;

    gettimeofday(&tv1, NULL);

    for(i=0; i<TEST_LOOP; i++)
    {
        g2d_blit(handle, &src, &dst);
    }

    g2d_finish(handle);

    gettimeofday(&tv2, NULL);
    diff = ((tv2.tv_sec-tv1.tv_sec)*1000000+(tv2.tv_usec-tv1.tv_usec)) / TEST_LOOP;

    printf("resize format from nv12 to rgba8888, time %dus, %dfps, %dMpixel/s ........\n", diff, 1000000/diff, test_width * test_height / diff);

    src.left = 10;
    src.top = 10;
    src.right = test_width - 10;
    src.bottom = test_height - 10;
    src.stride = test_width;
    src.width  = test_width;
    src.height = test_height;
    src.rot    = G2D_ROTATION_0;
    src.format = G2D_RGBA8888;

    dst.left = 0;
    dst.top = 0;
    dst.right = test_width;
    dst.bottom = test_height;
    dst.stride = test_width;
    dst.width  = test_width;
    dst.height = test_height;
    dst.rot    = G2D_ROTATION_0;
    dst.format = G2D_RGBA8888;

    gettimeofday(&tv1, NULL);

    for(i=0; i<TEST_LOOP; i++)
    {
        g2d_blit(handle, &src, &dst);
    }

    g2d_finish(handle);

    gettimeofday(&tv2, NULL);
    diff = ((tv2.tv_sec-tv1.tv_sec)*1000000+(tv2.tv_usec-tv1.tv_usec)) / TEST_LOOP;

    printf("crop from (%d,%d,%d,%d) to %dx%d, time %dus, %dfps, %dMpixel/s ........\n",
           src.left, src.top, src.right, src.bottom, dst.width, dst.height,
           diff, 1000000/diff, test_width * test_height / diff);

/****************************************** test g2d_copy *********************************************************/
    //set test data in src buffer
    memset(s_buf->buf_vaddr, 0xab, test_width * test_height * 4);

    //set garbage data in dst buffer
    memset(d_buf->buf_vaddr, 0xcd, test_width * test_height * 4);

    g2d_copy(handle, d_buf, s_buf, test_width * test_height * 4);

    g2d_finish(handle);

    if(memcmp(s_buf->buf_vaddr, d_buf->buf_vaddr, test_width * test_height * 4))
    {
        printf("g2d_copy: dst buffer is not copied from src buffer correctly !\n");
    }

    printf("---------------- g2d copy & cache performance ----------------\n");
    gettimeofday(&tv1, NULL);

    for(i=0; i<TEST_LOOP; i++)
    {
        g2d_copy(handle, d_buf, s_buf, test_width * test_height * 4);
    }

    g2d_finish(handle);

    gettimeofday(&tv2, NULL);
    diff = ((tv2.tv_sec-tv1.tv_sec)*1000000+(tv2.tv_usec-tv1.tv_usec)) / TEST_LOOP;

    printf("g2d copy non-cacheable time %dus, %dfps, %dMpixel/s ........\n", diff, 1000000/diff, test_width * test_height / diff);

    gettimeofday(&tv1, NULL);

    for(i=0; i<TEST_LOOP; i++)
    {
        memcpy(d_buf->buf_vaddr, s_buf->buf_vaddr, test_width * test_height * 4);
    }

    gettimeofday(&tv2, NULL);
    diff = ((tv2.tv_sec-tv1.tv_sec)*1000000+(tv2.tv_usec-tv1.tv_usec)) / TEST_LOOP;

    printf("cpu copy non-cacheable time %dus, %dfps, %dMpixel/s ........\n", diff, 1000000/diff, test_width * test_height / diff);

    v_buf1 = malloc(test_width * test_height * 4);
    v_buf2 = malloc(test_width * test_height * 4);

    //initialize source buffer
    memset(v_buf1, 0, test_width * test_height * 4);

    gettimeofday(&tv1, NULL);

    for(i=0; i<TEST_LOOP; i++)
    {
        memcpy(v_buf2, v_buf1, test_width * test_height * 4);
    }

    gettimeofday(&tv2, NULL);
    diff = ((tv2.tv_sec-tv1.tv_sec)*1000000+(tv2.tv_usec-tv1.tv_usec)) / TEST_LOOP;

    printf("cpu copy user cacheable time %dus, %dfps, %dMpixel/s ........\n", diff, 1000000/diff, test_width * test_height / diff);

    memset(v_buf1, 0, test_width * test_height * 4);

    gettimeofday(&tv1, NULL);

    for(i=0; i<TEST_LOOP; i++)
    {
        memcpy(d_buf->buf_vaddr, v_buf1, test_width * test_height * 4);
    }

    gettimeofday(&tv2, NULL);
    diff = ((tv2.tv_sec-tv1.tv_sec)*1000000+(tv2.tv_usec-tv1.tv_usec)) / TEST_LOOP;

    printf("cpu copy user cacheable to non-cacheable time %dus, %dfps, %dMpixel/s ........\n", diff, 1000000/diff, test_width * test_height / diff);

    memset(s_buf->buf_vaddr, 0, test_width * test_height * 4);

    gettimeofday(&tv1, NULL);

    for(i=0; i<TEST_LOOP; i++)
    {
        memcpy(v_buf2, s_buf->buf_vaddr, test_width * test_height * 4);
    }

    gettimeofday(&tv2, NULL);
    diff = ((tv2.tv_sec-tv1.tv_sec)*1000000+(tv2.tv_usec-tv1.tv_usec)) / TEST_LOOP;

    printf("cpu copy user non-cacheable to cacheable time %dus, %dfps, %dMpixel/s ........\n", diff, 1000000/diff, test_width * test_height / diff);

    gettimeofday(&tv1, NULL);

    free(v_buf1);
    free(v_buf2);
    g2d_free(s_buf);
    g2d_free(d_buf);

    s_buf = g2d_alloc(test_width * test_height * 4, 1);
    d_buf = g2d_alloc(test_width * test_height * 4, 1);

    gettimeofday(&tv1, NULL);

    for(i=0; i<TEST_LOOP; i++)
    {
        memcpy(d_buf->buf_vaddr, s_buf->buf_vaddr, test_width * test_height * 4);
    }

    gettimeofday(&tv2, NULL);
    diff = ((tv2.tv_sec-tv1.tv_sec)*1000000+(tv2.tv_usec-tv1.tv_usec)) / TEST_LOOP;

    printf("cpu copy gpu cacheable time %dus, %dfps, %dMpixel/s ........\n", diff, 1000000/diff, test_width * test_height / diff);

/****************************************** test g2d_cache_op *********************************************************/
    //set test data in src buffer
    memset(s_buf->buf_vaddr, 0xab, test_width * test_height * 4);

    //set garbage data in dst buffer
    memset(d_buf->buf_vaddr, 0xcd, test_width * test_height * 4);

    g2d_cache_op(s_buf, G2D_CACHE_FLUSH);
    g2d_cache_op(d_buf, G2D_CACHE_FLUSH);

    g2d_copy(handle, d_buf, s_buf, test_width * test_height * 4);

    g2d_finish(handle);

    if(memcmp(s_buf->buf_vaddr, d_buf->buf_vaddr, test_width * test_height * 4))
    {
        printf("g2d_cache_op error, the comparision result is different !\n");
    }

    gettimeofday(&tv1, NULL);

    for(i=0; i<TEST_LOOP; i++)
    {
        g2d_cache_op(s_buf, G2D_CACHE_CLEAN);
        g2d_cache_op(d_buf, G2D_CACHE_INVALIDATE);

        g2d_copy(handle, d_buf, s_buf, test_width * test_height * 4);

        g2d_finish(handle);
    }

    gettimeofday(&tv2, NULL);
    diff = ((tv2.tv_sec-tv1.tv_sec)*1000000+(tv2.tv_usec-tv1.tv_usec)) / TEST_LOOP;

    printf("g2d copy with cache op time %dus, %dfps, %dMpixel/s ........\n", diff, 1000000/diff, test_width * test_height / diff);

/****************************************** test g2d_blit with vg core *********************************************************/

    g2d_free(s_buf);
    g2d_free(d_buf);

    //set current hardware type as vg core
    if(g2d_make_current(handle, G2D_HARDWARE_VG) == -1)
    {
        printf("vg core is not supported in device!\n");
        g2d_close(handle);
        return 0;
    }

    s_buf = g2d_alloc(test_width * test_height * 4, 0);
    d_buf = g2d_alloc(test_width * test_height * 4, 0);

    src.planes[0] = s_buf->buf_paddr;
    src.left = 0;
    src.top = 0;
    src.right = test_width;
    src.bottom = test_height;
    src.stride = test_width;
    src.width  = test_width;
    src.height = test_height;
    src.rot    = G2D_ROTATION_0;
    src.format = G2D_RGBA8888;

    dst.planes[0] = d_buf->buf_paddr;
    dst.left = 0;
    dst.top = 0;
    dst.right = test_width;
    dst.bottom = test_height;
    dst.stride = test_width;
    dst.width  = test_width;
    dst.height = test_height;
    dst.rot    = G2D_ROTATION_0;
    dst.format = G2D_RGBA8888;

    printf("---------------- g2d performance with vg core ----------------\n");

    //set garbage data in dst buffer
    memset(d_buf->buf_vaddr, 0xcd, test_width * test_height * 4);

    dst.clrcolor = 0xffeeddcc;

    g2d_clear(handle, &dst);

    g2d_finish(handle);

    //check if the generated color is correct
    for(i=0; i<test_width * test_height; i++)
    {
        int clrcolor = *(int *)(((char*)d_buf->buf_vaddr) + i*4);
        if(clrcolor != dst.clrcolor)
        {
            printf("[%d] Clear color 0x%x, Error color 0x%x\n", i, dst.clrcolor, clrcolor);
        }
    }

    gettimeofday(&tv1, NULL);

    for(i=0; i<TEST_LOOP; i++)
    {
        g2d_clear(handle, &dst);
    }

    g2d_finish(handle);

    gettimeofday(&tv2, NULL);
    diff = ((tv2.tv_sec-tv1.tv_sec)*1000000+(tv2.tv_usec-tv1.tv_usec)) / TEST_LOOP;

    printf("g2d clear with vg time %dus, %dfps, %dMpixel/s ........\n", diff, 1000000/diff, test_width * test_height / diff);

    gettimeofday(&tv1, NULL);

    for(i=0; i<TEST_LOOP; i++)
    {
        g2d_blit(handle, &src, &dst);
    }

    g2d_finish(handle);

    gettimeofday(&tv2, NULL);
    diff = ((tv2.tv_sec-tv1.tv_sec)*1000000+(tv2.tv_usec-tv1.tv_usec)) / TEST_LOOP;

    printf("g2d blit with vg time %dus, %dfps, %dMpixel/s ........\n", diff, 1000000/diff, test_width * test_height / diff);

    //set test data in src buffer
    memset(s_buf->buf_vaddr, 0xab, test_width * test_height * 4);

    //set garbage data in dst buffer
    memset(d_buf->buf_vaddr, 0xcd, test_width * test_height * 4);

    g2d_copy(handle, d_buf, s_buf, test_width * test_height * 4);

    g2d_finish(handle);

    if(memcmp(s_buf->buf_vaddr, d_buf->buf_vaddr, test_width * test_height * 4))
    {
        printf("g2d_copy: dst buffer is not copied from src buffer correctly !\n");
    }

    gettimeofday(&tv1, NULL);

    for(i=0; i<TEST_LOOP; i++)
    {
        g2d_copy(handle, d_buf, s_buf, test_width * test_height * 4);
    }

    g2d_finish(handle);

    gettimeofday(&tv2, NULL);
    diff = ((tv2.tv_sec-tv1.tv_sec)*1000000+(tv2.tv_usec-tv1.tv_usec)) / TEST_LOOP;

    printf("g2d copy with vg time %dus, %dfps, %dMpixel/s ........\n", diff, 1000000/diff, test_width * test_height / diff);

   /******************************test alpha blending with vg core ***************************************/

    //SRC: alpha blending mode G2D_ONE, G2D_ZERO
    //set test data in src buffer
    for(i=0; i<test_height; i++)
    {
         for(j=0; j<test_width; j++)
         {
             char* p = (char *)(((char*)s_buf->buf_vaddr) + (i*test_width+j)*4);
             p[0] = p[1] = p[2] = p[3] = (i*test_width+j) % 255;

             p = (char *)(((char*)d_buf->buf_vaddr) + (i*test_width+j)*4);
             p[0] = p[1] = p[2] = p[3] = ((i*test_width+j + 128) % 255);
         }
    }

    src.blendfunc = G2D_ONE;
    dst.blendfunc = G2D_ZERO;

    g2d_enable(handle,G2D_BLEND);
    g2d_blit(handle, &src, &dst);
    g2d_disable(handle, G2D_BLEND);

    g2d_finish(handle);

    for(i=0; i<test_height; i++)
    {
         for(j=0; j<test_width; j++)
         {
             unsigned char Cs,As,Co,Ao;
             unsigned char* p = (unsigned char *)(((char*)d_buf->buf_vaddr) + (i*test_width+j)*4);

             if(p[0] != p[1] || p[0] != p[2])
             {
                printf("vg blended r/g/b values(%d/%d/%d) are not same in SRC mode!\n", p[0], p[1], p[2]);
             }

             Co = Ao = Cs = As = (i*test_width+j) % 255;

             if(Co != p[0] || Ao != p[3])
             {
                printf("vg blended color(%d) or alpha(%d) is incorrect in SRC mode, Co %d, Ao %d\n", p[0],p[3],Co,Ao);
             }
         }
    }

    //SRC OVER: alpha blending mode G2D_ONE, G2D_ONE_MINUS_SRC_ALPHA
    //set test data in src buffer
    for(i=0; i<test_height; i++)
    {
         for(j=0; j<test_width; j++)
         {
             char* p = (char *)(((char*)s_buf->buf_vaddr) + (i*test_width+j)*4);
             p[0] = p[1] = p[2] = p[3] = (i*test_width+j) % 255;

             p = (char *)(((char*)d_buf->buf_vaddr) + (i*test_width+j)*4);
             p[0] = p[1] = p[2] = p[3] = ((i*test_width+j + 128) % 255);
         }
    }

    src.blendfunc = G2D_ONE;
    dst.blendfunc = G2D_ONE_MINUS_SRC_ALPHA;

    g2d_enable(handle,G2D_BLEND);
    g2d_blit(handle, &src, &dst);
    g2d_disable(handle, G2D_BLEND);

    g2d_finish(handle);

    for(i=0; i<test_height; i++)
    {
         for(j=0; j<test_width; j++)
         {
             unsigned int  iCo, iAo;
             unsigned char Cs,As,Cd,Ad,Co,Ao;
             unsigned char* p = (unsigned char *)(((char*)d_buf->buf_vaddr) + (i*test_width+j)*4);

             if(p[0] != p[1] || p[0] != p[2])
             {
                printf("vg blended r/g/b values(%d/%d/%d) are not same in SRC OVER mode!\n", p[0], p[1], p[2]);
             }

             Cs = As = (i*test_width+j) % 255;
             Cd = Ad = ((i*test_width+j + 128) % 255);

             iCo = ((unsigned int)Cs * 255 + (unsigned int)Cd * (255 - As)) / 255;
             iAo = ((unsigned int)As * 255 + (unsigned int)Ad * (255 - As)) / 255;

             if(iCo > 255) Co = 255;
             else Co = (unsigned char)iCo;

             if(iAo > 255) Ao = 255;
             else Ao = (unsigned char)iAo;

             //compare the result with +/-1 threshold
             if(abs(Co - p[0]) > 2 || abs(Ao - p[3]) > 2)
             {
                printf("vg blended color(%d) or alpha(%d) is incorrect in SRC OVER mode, Cs %d, As %d, Cd %d, Ad %d, Co %d, Ao %d\n", p[0],p[3],Cs,As,Cd,Ad,Co,Ao);
             }
         }
    }

    //DST OVER: alpha blending mode G2D_ONE_MINUS_DST_ALPHA, G2D_ONE
    //set test data in src buffer
    for(i=0; i<test_height; i++)
    {
         for(j=0; j<test_width; j++)
         {
             char* p = (char *)(((char*)s_buf->buf_vaddr) + (i*test_width+j)*4);
             p[0] = p[1] = p[2] = p[3] = (i*test_width+j) % 255;

             p = (char *)(((char*)d_buf->buf_vaddr) + (i*test_width+j)*4);
             p[0] = p[1] = p[2] = p[3] = ((i*test_width+j + 128) % 255);
         }
    }

    src.blendfunc = G2D_ONE_MINUS_DST_ALPHA;
    dst.blendfunc = G2D_ONE;

    g2d_enable(handle,G2D_BLEND);
    g2d_blit(handle, &src, &dst);
    g2d_disable(handle, G2D_BLEND);

    g2d_finish(handle);

    for(i=0; i<test_height; i++)
    {
         for(j=0; j<test_width; j++)
         {
             unsigned int  iCo, iAo;
             unsigned char Cs,As,Cd,Ad,Co,Ao;
             unsigned char* p = (unsigned char *)(((char*)d_buf->buf_vaddr) + (i*test_width+j)*4);

             if(p[0] != p[1] || p[0] != p[2])
             {
                printf("vg blended r/g/b values(%d/%d/%d) are not same in DST OVER mode!\n", p[0], p[1], p[2]);
             }

             Cs = As = (i*test_width+j) % 255;
             Cd = Ad = ((i*test_width+j + 128) % 255);

             iCo = ((unsigned int)Cs * (255 - Ad) + (unsigned int)Cd * 255) / 255;
             iAo = ((unsigned int)As * (255 - Ad) + (unsigned int)Ad * 255) / 255;

             if(iCo > 255) Co = 255;
             else Co = (unsigned char)iCo;

             if(iAo > 255) Ao = 255;
             else Ao = (unsigned char)iAo;

             //compare the result with +/-1 threshold
             if(abs(Co - p[0]) > 2 || abs(Ao - p[3]) > 2)
             {
                printf("vg blended color(%d) or alpha(%d) is incorrect in DST OVER mode, Cs %d, As %d, Cd %d, Ad %d, Co %d, Ao %d\n", p[0],p[3],Cs,As,Cd,Ad,Co,Ao);
             }
         }
    }

    //SRC IN: alpha blending mode G2D_DST_ALPHA, G2D_ZERO
    //set test data in src buffer
    for(i=0; i<test_height; i++)
    {
         for(j=0; j<test_width; j++)
         {
             char* p = (char *)(((char*)s_buf->buf_vaddr) + (i*test_width+j)*4);
             p[0] = p[1] = p[2] = p[3] = (i*test_width+j) % 255;

             p = (char *)(((char*)d_buf->buf_vaddr) + (i*test_width+j)*4);
             p[0] = p[1] = p[2] = p[3] = ((i*test_width+j + 128) % 255);
         }
    }

    src.blendfunc = G2D_DST_ALPHA;
    dst.blendfunc = G2D_ZERO;

    g2d_enable(handle,G2D_BLEND);
    g2d_blit(handle, &src, &dst);
    g2d_disable(handle, G2D_BLEND);

    g2d_finish(handle);

    for(i=0; i<test_height; i++)
    {
         for(j=0; j<test_width; j++)
         {
             unsigned char Cs,As,Ad,Co,Ao;
             unsigned char* p = (unsigned char *)(((char*)d_buf->buf_vaddr) + (i*test_width+j)*4);

             if(p[0] != p[1] || p[0] != p[2])
             {
                printf("vg blended r/g/b values(%d/%d/%d) are not same in SRC IN mode!\n", p[0], p[1], p[2]);
             }

             Cs = As = (i*test_width+j) % 255;
             Ad = ((i*test_width+j + 128) % 255);

             Co = (unsigned char)(((unsigned int)Cs * Ad) / 255);
             Ao = (unsigned char)(((unsigned int)As * Ad) / 255);

             //compare the result with +/-1 threshold
             if(abs(Co - p[0]) > 2 || abs(Ao - p[3]) > 2)
             {
                printf("vg blended color(%d) or alpha(%d) is incorrect in SRC IN mode, Cs %d, As %d, Ad %d, Co %d, Ao %d\n", p[0],p[3],Cs,As,Ad,Co,Ao);
             }
         }
    }

    //DST IN: alpha blending mode G2D_ZERO, G2D_SRC_ALPHA
    //set test data in src buffer
    for(i=0; i<test_height; i++)
    {
         for(j=0; j<test_width; j++)
         {
             char* p = (char *)(((char*)s_buf->buf_vaddr) + (i*test_width+j)*4);
             p[0] = p[1] = p[2] = p[3] = (i*test_width+j) % 255;

             p = (char *)(((char*)d_buf->buf_vaddr) + (i*test_width+j)*4);
             p[0] = p[1] = p[2] = p[3] = ((i*test_width+j + 128) % 255);
         }
    }

    src.blendfunc = G2D_ZERO;
    dst.blendfunc = G2D_SRC_ALPHA;

    g2d_enable(handle,G2D_BLEND);
    g2d_blit(handle, &src, &dst);
    g2d_disable(handle, G2D_BLEND);

    g2d_finish(handle);

    for(i=0; i<test_height; i++)
    {
         for(j=0; j<test_width; j++)
         {
             unsigned char As,Cd,Ad,Co,Ao;
             unsigned char* p = (unsigned char *)(((char*)d_buf->buf_vaddr) + (i*test_width+j)*4);

             if(p[0] != p[1] || p[0] != p[2])
             {
                printf("vg blended r/g/b values(%d/%d/%d) are not same in DST IN mode!\n", p[0], p[1], p[2]);
             }

             As = (i*test_width+j) % 255;
             Cd = Ad = ((i*test_width+j + 128) % 255);

             Co = (unsigned char)(((unsigned int)Cd * As) / 255);
             Ao = (unsigned char)(((unsigned int)Ad * As) / 255);

             //compare the result with +/-1 threshold
             if(abs(Co - p[0]) > 2 || abs(Ao - p[3]) > 2)
             {
                printf("vg blended color(%d) or alpha(%d) is incorrect in DST IN mode, As %d, Cd %d, Ad %d, Co %d, Ao %d\n", p[0],p[3],As,Cd,Ad,Co,Ao);
             }
         }
    }


/****************************************** test vg resize performance *********************************************************/

    src.left = 0;
    src.top = 0;
    src.right = (test_width > 1280) ? 1280 : test_width >> 1;
    src.bottom = (test_height > 720) ? 720 : test_height >> 1;
    src.stride = (test_width > 1280) ? 1280 : test_width >> 1;
    src.width  = (test_width > 1280) ? 1280 : test_width >> 1;
    src.height = (test_height > 720) ? 720 : test_height >> 1;
    src.rot    = G2D_ROTATION_0;
    src.format = G2D_RGBA8888;

    dst.left = 0;
    dst.top = 0;
    dst.right = test_width;
    dst.bottom = test_height;
    dst.stride = test_width;
    dst.width  = test_width;
    dst.height = test_height;
    dst.rot    = G2D_ROTATION_0;
    dst.format = G2D_RGBA8888;

    printf("g2d resize with vg from %dx%d to %dx%d: \n", src.width, src.height, dst.width, dst.height);

    gettimeofday(&tv1, NULL);

    for(i=0; i<TEST_LOOP; i++)
    {
        g2d_blit(handle, &src, &dst);
    }

    g2d_finish(handle);

    gettimeofday(&tv2, NULL);
    diff = ((tv2.tv_sec-tv1.tv_sec)*1000000+(tv2.tv_usec-tv1.tv_usec)) / TEST_LOOP;

    printf("resize format from rgba8888 to rgba8888, time %dus, %dfps, %dMpixel/s ........\n", diff, 1000000/diff, test_width * test_height / diff);

    g2d_free(s_buf);
    g2d_free(d_buf);

    g2d_close(handle);

    return 0;
}
