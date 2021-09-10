/*
 *  Copyright (C) 2013 Freescale Semiconductor, Inc.
 *  All Rights Reserved.
 *
 *  The following programs are the sole property of Freescale Semiconductor Inc.,
 *  and contain its proprietary and confidential information.
 *
 */

/*
 * @file g2d_overly_test.c
 *
 * @brief G2D overlay test application
 *
 */

#ifdef __cplusplus
extern "C"{
#endif

/*=======================================================================
                                        INCLUDE FILES
=======================================================================*/
/* Standard Include Files */
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <linux/fb.h>
#include <sys/mman.h>
#include <math.h>
#include <string.h>
#include <malloc.h>
#include <g2d.h>

#include "fsl_logo_bgr_480x360.c"
#include "ginger_bgr_800x600.c"
#include "akiyo_I420_176x144.c"
#include "wall-1024x768-565rgb.c"
#include "NV16_352x288.c"
#include "yuyv_352x288.c"

#define TFAIL -1
#define TPASS 0

#define CACHEABLE 0

int fd_fb0 = 0;
unsigned short * fb0;
int g_fb0_size;
int g_fb0_phys;

static void draw_image_to_framebuffer(int img_width, int img_height, int img_size, int img_format, unsigned char *img_ptr,
		 struct fb_var_screeninfo *screen_info, int left, int top, int to_width, int to_height, int set_alpha, int rotation, int set_blur)
{
	int i;
	struct g2d_surface src,dst;
	struct g2d_buf *buf;
	void *g2dHandle;

	if ( ( (left+to_width) > (int)screen_info->xres ) || ( (top+to_height) > (int)screen_info->yres ) )  {
		printf("Bad display image dimensions!\n");
		return;
	}

#if CACHEABLE
	buf = g2d_alloc(img_size, 1);//alloc physical contiguous memory for source image data with cacheable attribute
#else
	buf = g2d_alloc(img_size, 0);//alloc physical contiguous memory for source image data
#endif
	if(!buf) {
		printf("Fail to allocate physical memory for image buffer!\n");
		return;
	}

	memcpy(buf->buf_vaddr, img_ptr, img_size);

#if CACHEABLE
        g2d_cache_op(buf, G2D_CACHE_FLUSH);
#endif

	if(g2d_open(&g2dHandle) == -1 || g2dHandle == NULL) {
		printf("Fail to open g2d device!\n");
		g2d_free(buf);
		return;
	}

/*
	NOTE: in this example, all the test image data meet with the alignment requirement.
	Thus, in your code, you need to pay attention on that.

	Pixel buffer address alignment requirement,
	RGB/BGR:  pixel data in planes [0] with 16bytes alignment,
	NV12/NV16:  Y in planes [0], UV in planes [1], with 64bytes alignment,
	I420:    Y in planes [0], U in planes [1], V in planes [2], with 64 bytes alignment,
	YV12:  Y in planes [0], V in planes [1], U in planes [2], with 64 bytes alignment,
	NV21/NV61:  Y in planes [0], VU in planes [1], with 64bytes alignment,
	YUYV/YVYU/UYVY/VYUY:  in planes[0], buffer address is with 16bytes alignment.
*/

	src.format = img_format;
	switch (src.format) {
	case G2D_RGB565:
	case G2D_RGBA8888:
	case G2D_RGBX8888:
	case G2D_BGRA8888:
	case G2D_BGRX8888:
	case G2D_BGR565:
	case G2D_YUYV:
	case G2D_UYVY:
		src.planes[0] = buf->buf_paddr;
		break;
	case G2D_NV12:
		src.planes[0] = buf->buf_paddr;
		src.planes[1] = buf->buf_paddr + img_width * img_height;
		break;
	case G2D_I420:
		src.planes[0] = buf->buf_paddr;
		src.planes[1] = buf->buf_paddr + img_width * img_height;
		src.planes[2] = src.planes[1]  + img_width * img_height / 4;
		break;
	case G2D_NV16:
		src.planes[0] = buf->buf_paddr;
		src.planes[1] = buf->buf_paddr + img_width * img_height;
                break;
	default:
		printf("Unsupport image format in the example code\n");
		return;
	}

	src.left = 0;
	src.top = 0;
	src.right = img_width;
	src.bottom = img_height;
	src.stride = img_width;
	src.width  = img_width;
	src.height = img_height;
	src.rot  = G2D_ROTATION_0;

	dst.planes[0] = g_fb0_phys;
	dst.left = left;
	dst.top = top;
	dst.right = left + to_width;
	dst.bottom = top + to_height;
	dst.stride = screen_info->xres;
	dst.width  = screen_info->xres;
	dst.height = screen_info->yres;
	dst.rot    = rotation;
	dst.format = screen_info->bits_per_pixel == 16 ? G2D_RGB565 : (screen_info->red.offset == 0 ? G2D_RGBA8888 : G2D_BGRA8888);

	if (set_alpha)
	{
		src.blendfunc = G2D_ONE;
		dst.blendfunc = G2D_ONE_MINUS_SRC_ALPHA;
	
		src.global_alpha = 0x80;
		dst.global_alpha = 0xff;
	
		g2d_enable(g2dHandle, G2D_BLEND);
		g2d_enable(g2dHandle, G2D_GLOBAL_ALPHA);
	}

    if(set_blur)
    {
        g2d_enable(g2dHandle, G2D_BLUR);
    }

    g2d_blit(g2dHandle, &src, &dst);
	g2d_finish(g2dHandle);

	if (set_alpha)
	{
		g2d_disable(g2dHandle, G2D_GLOBAL_ALPHA);
		g2d_disable(g2dHandle, G2D_BLEND);
	}

    if(set_blur)
    {
        g2d_disable(g2dHandle, G2D_BLUR);
    }

	g2d_close(g2dHandle);
	g2d_free(buf);
}

/**/
struct img_info{
    int img_left;
    int img_top;
    int img_right;
    int img_bottom;
    int img_width;
    int img_height;
    int img_rot;
    int img_size;
    int img_format;
    unsigned char *img_ptr;
};

static void draw_image_with_multiblit(struct img_info * img_info_ptr[], const int layers, struct fb_var_screeninfo *screen_info)
{
    int i, n;
    struct g2d_surface src,dst;
    struct g2d_surface_pair *sp[layers];
    struct g2d_buf *buf;
    struct g2d_buf *mul_s_buf[layers];
    void *g2dHandle;
    int g2d_feature_available = 0;

    for(n = 0; n < layers; n++)
    {
        sp[n] = (struct g2d_surface_pair *)malloc(sizeof(struct g2d_surface_pair) * layers);
    }
    
    for(n = 0; n < layers; n++)
    {
        mul_s_buf[n] = g2d_alloc(img_info_ptr[n]->img_size, 0);
        if(!mul_s_buf[n])
        {
            printf("Fail to allocate physical memory for image buffer!\n");
            return;
        }
        memcpy(mul_s_buf[n]->buf_vaddr, img_info_ptr[n]->img_ptr, img_info_ptr[n]->img_size);
    }
    
    if(g2d_open(&g2dHandle) == -1 || g2dHandle == NULL) 
    {
        printf("Fail to open g2d device!\n");
        for(n = 0; n < layers; n++)
            g2d_free(mul_s_buf[n]);
        return;
    }

    g2d_query_feature(g2dHandle, G2D_MULTI_SOURCE_BLT, &g2d_feature_available);
    if(g2d_feature_available == 0)
    {
        printf("!!! g2d_feature 'G2D_MULTI_SOURCE_BLT' Not Supported for this hardware!!!\n");
        return;
    }

    sp[0]->d.planes[0] = g_fb0_phys;
    sp[0]->d.left = 0;
    sp[0]->d.top = 0;
    sp[0]->d.right = screen_info->xres;
    sp[0]->d.bottom = screen_info->yres;
    sp[0]->d.stride = screen_info->xres;
    sp[0]->d.width = screen_info->xres;
    sp[0]->d.height = screen_info->yres;
    sp[0]->d.rot = G2D_ROTATION_0;
    sp[0]->d.format = screen_info->bits_per_pixel == 16 ? G2D_RGB565 : (screen_info->red.offset == 0 ? G2D_RGBA8888 : G2D_BGRA8888);
    
    for(i = 1; i < layers; i++)
    {
        sp[i]->d = sp[0]->d;
    }

    for(i = 0; i < layers; i++)
    {
        sp[i]->s.left = img_info_ptr[i]->img_left;
        sp[i]->s.top = img_info_ptr[i]->img_top;
        sp[i]->s.right =  img_info_ptr[i]->img_right;
        sp[i]->s.bottom = img_info_ptr[i]->img_bottom;
        sp[i]->s.stride = img_info_ptr[i]->img_width;
        sp[i]->s.width =  img_info_ptr[i]->img_width;
        sp[i]->s.height = img_info_ptr[i]->img_height;
        sp[i]->s.rot =    img_info_ptr[i]->img_rot;
        sp[i]->s.format = img_info_ptr[i]->img_format;

        switch(sp[i]->s.format)
        {
        case G2D_RGB565:
        case G2D_RGBA8888:
        case G2D_RGBX8888:
        case G2D_BGRA8888:
        case G2D_BGRX8888:
        case G2D_BGR565:
        case G2D_YUYV:
        case G2D_UYVY:
            sp[i]->s.planes[0] = mul_s_buf[i]->buf_paddr;
            break;
        case G2D_NV16:
            sp[i]->s.planes[0] = mul_s_buf[i]->buf_paddr;
            sp[i]->s.planes[1] = mul_s_buf[i]->buf_paddr + sp[i]->s.width * sp[i]->s.height;
            break;
        case G2D_I420:
            sp[i]->s.planes[0] = mul_s_buf[i]->buf_paddr;
            sp[i]->s.planes[1] = mul_s_buf[i]->buf_paddr + sp[i]->s.width * sp[i]->s.height;
            sp[i]->s.planes[2] = sp[i]->s.planes[1] + sp[i]->s.width * sp[i]->s.height / 4;
            break;
        default:
            printf("Unsupport image format in the example code\n");
            return;
        }
    }

    /* alpha blending*/
    sp[0]->d.blendfunc = G2D_ONE_MINUS_SRC_ALPHA;
    sp[0]->d.global_alpha = 0xff;

    sp[0]->s.blendfunc = G2D_ONE;
    sp[0]->s.global_alpha = 0x80;
    sp[1]->s.blendfunc = G2D_ONE;
    sp[1]->s.global_alpha = 0x80;
    sp[2]->s.blendfunc = G2D_ONE;
    sp[2]->s.global_alpha = 0x80;
    sp[3]->s.blendfunc = G2D_ONE;
    sp[3]->s.global_alpha = 0x80;
    sp[4]->s.blendfunc = G2D_ONE;
    sp[4]->s.global_alpha = 0x80;
    sp[5]->s.blendfunc = G2D_ONE;
    sp[5]->s.global_alpha = 0x80;
    sp[6]->s.blendfunc = G2D_ONE;
    sp[6]->s.global_alpha = 0xff;
    sp[7]->s.blendfunc = G2D_ONE;
    sp[7]->s.global_alpha = 0x80;


    g2d_enable(g2dHandle, G2D_BLEND);
    g2d_enable(g2dHandle, G2D_GLOBAL_ALPHA);

    g2d_multi_blit(g2dHandle, sp, layers-1);
    g2d_finish(g2dHandle);

    g2d_disable(g2dHandle, G2D_GLOBAL_ALPHA);
    g2d_disable(g2dHandle, G2D_BLEND);

    g2d_close(g2dHandle);
    for(n = 0; n < layers; n++)
        g2d_free(mul_s_buf[n]);
}

void Test_g2d_multi_blit(struct fb_var_screeninfo *screen_info)
{
    int i = 0;
    const int layers = 8;
    struct img_info *img_info_ptr[layers];

    for(i = 0; i < layers; i++)
    {
        img_info_ptr[i] = (struct img_info *)malloc(sizeof(struct img_info));
    }

    img_info_ptr[0]->img_left = 0;
    img_info_ptr[0]->img_top = 0;
    img_info_ptr[0]->img_right = 1024;
    img_info_ptr[0]->img_bottom = 768;
    img_info_ptr[0]->img_width = 1024;
    img_info_ptr[0]->img_height = 768;
    img_info_ptr[0]->img_rot = G2D_ROTATION_0;
    img_info_ptr[0]->img_size = 1024*768*2;
    img_info_ptr[0]->img_format = G2D_RGB565;
    img_info_ptr[0]->img_ptr = (unsigned char *)wall_1024x768_565rgb;

    img_info_ptr[1]->img_left = 0;
    img_info_ptr[1]->img_top = 0;
    img_info_ptr[1]->img_right = 1024;
    img_info_ptr[1]->img_bottom = 768;
    img_info_ptr[1]->img_width = 1024;
    img_info_ptr[1]->img_height = 768;
    img_info_ptr[1]->img_rot = G2D_ROTATION_0;
    img_info_ptr[1]->img_size = 1024*768*2;
    img_info_ptr[1]->img_format = G2D_RGB565;
    img_info_ptr[1]->img_ptr = (unsigned char *)wall_1024x768_565rgb;

    img_info_ptr[2]->img_left = 0;
    img_info_ptr[2]->img_top = 0;
    img_info_ptr[2]->img_right = 1024;
    img_info_ptr[2]->img_bottom = 768;
    img_info_ptr[2]->img_width = 1024;
    img_info_ptr[2]->img_height = 768;
    img_info_ptr[2]->img_rot = G2D_ROTATION_0;
    img_info_ptr[2]->img_size = 1024*768*2;
    img_info_ptr[2]->img_format = G2D_RGB565;
    img_info_ptr[2]->img_ptr = (unsigned char *)wall_1024x768_565rgb;

    img_info_ptr[3]->img_left = 0;
    img_info_ptr[3]->img_top = 0;
    img_info_ptr[3]->img_right = 600;
    img_info_ptr[3]->img_bottom = 600;
    img_info_ptr[3]->img_width = 800;
    img_info_ptr[3]->img_height = 600;
    img_info_ptr[3]->img_rot = G2D_ROTATION_0;
    img_info_ptr[3]->img_size = 800*600*2;
    img_info_ptr[3]->img_format = G2D_BGR565;
    img_info_ptr[3]->img_ptr = (unsigned char *)ginger_bgr_800x600;

    img_info_ptr[4]->img_left = 0;
    img_info_ptr[4]->img_top = 0;
    img_info_ptr[4]->img_right = 480;
    img_info_ptr[4]->img_bottom = 260;
    img_info_ptr[4]->img_width = 480;
    img_info_ptr[4]->img_height = 360;
    img_info_ptr[4]->img_rot = G2D_ROTATION_90;
    img_info_ptr[4]->img_size = 480*360*2;
    img_info_ptr[4]->img_format = G2D_BGR565;
    img_info_ptr[4]->img_ptr = (unsigned char *)fsl_logo_bgr_480x360;

    img_info_ptr[5]->img_left = 0;
    img_info_ptr[5]->img_top = 0;
    img_info_ptr[5]->img_right = 352;
    img_info_ptr[5]->img_bottom = 288;
    img_info_ptr[5]->img_width = 352;
    img_info_ptr[5]->img_height = 288;
    img_info_ptr[5]->img_rot = G2D_ROTATION_0;
    img_info_ptr[5]->img_size = 352*288*2;
    img_info_ptr[5]->img_format = G2D_YUYV;
    img_info_ptr[5]->img_ptr = (unsigned char *)yuyv_352x288;
    
    img_info_ptr[6]->img_left = 0;
    img_info_ptr[6]->img_top = 0;
    img_info_ptr[6]->img_right = 352/2;
    img_info_ptr[6]->img_bottom = 288/2;
    img_info_ptr[6]->img_width = 352;
    img_info_ptr[6]->img_height = 288;
    img_info_ptr[6]->img_rot = G2D_ROTATION_0;
    img_info_ptr[6]->img_size = 352*288*2;
    img_info_ptr[6]->img_format = G2D_NV16;
    img_info_ptr[6]->img_ptr = (unsigned char *)NV16_352x288;

    img_info_ptr[7]->img_left = 0;
    img_info_ptr[7]->img_top = 0;
    img_info_ptr[7]->img_right = 176;
    img_info_ptr[7]->img_bottom = 144;
    img_info_ptr[7]->img_width = 176;
    img_info_ptr[7]->img_height = 144;
    img_info_ptr[7]->img_rot = G2D_ROTATION_0;
    img_info_ptr[7]->img_size = 176*144*3/2;
    img_info_ptr[7]->img_format = G2D_I420;
    img_info_ptr[7]->img_ptr = (unsigned char *)akiyo_I420_176x144;

    draw_image_with_multiblit(img_info_ptr, layers, screen_info);

}

void clear_screen_with_g2d(struct fb_var_screeninfo *screen_info, int color)
{
    struct g2d_surface dst;
    void *handle;

    if(g2d_open(&handle) == -1 || handle == NULL)
    {
        printf("Fail to open g2d device!\n");
        return;
    }

    dst.planes[0] = g_fb0_phys;
    dst.left = 0;
    dst.top = 0;
    dst.right = screen_info->xres;
    dst.bottom = screen_info->yres;
    dst.stride = screen_info->xres;
    dst.width  = screen_info->xres;
    dst.height = screen_info->yres;
    dst.rot    = G2D_ROTATION_0;
    dst.clrcolor = color;
    dst.format = screen_info->bits_per_pixel == 16 ? G2D_RGB565 : (screen_info->red.offset == 0 ? G2D_RGBA8888 : G2D_BGRA8888);

    g2d_clear(handle, &dst);
    g2d_finish(handle);
    
    g2d_close(handle);
}

int
main(int argc, char **argv)
{
	int retval = TPASS;
	struct fb_var_screeninfo screen_info;
	struct fb_fix_screeninfo fb_info;
	struct timeval tv1,tv2;
	
	if ((fd_fb0 = open("/dev/fb0", O_RDWR, 0)) < 0) {
		if ((fd_fb0 = open("/dev/graphics/fb0", O_RDWR, 0)) < 0) {
			printf("Unable to open fb0\n");
			retval = TFAIL;
			goto err0;
		}
	}

	/* Get fix screen info. */
	retval = ioctl(fd_fb0, FBIOGET_FSCREENINFO, &fb_info);
	if(retval < 0) {
		goto err2;
	}
	g_fb0_phys = fb_info.smem_start;

	/* Get variable screen info. */
	retval = ioctl(fd_fb0, FBIOGET_VSCREENINFO, &screen_info);
	if (retval < 0) {
		goto err2;
	}

	if (!screen_info.xres_virtual) {
		printf("Unable to get framebuffer physical address, not supported !\n");
		goto err2;
	}

	g_fb0_phys += (screen_info.xres_virtual * screen_info.yoffset * screen_info.bits_per_pixel / 8);

	g_fb0_size = screen_info.xres_virtual * screen_info.yres_virtual * screen_info.bits_per_pixel / 8;

    clear_screen_with_g2d(&screen_info, 0xff000000);

	gettimeofday(&tv1, NULL);

	draw_image_to_framebuffer(1024, 768, 1024*768*2, G2D_RGB565, (unsigned char *)wall_1024x768_565rgb, &screen_info, 0, 0, 1024, 768, 1, G2D_ROTATION_0, 0);

	draw_image_to_framebuffer(800, 600, 800*600*2, G2D_BGR565, (unsigned char *) ginger_bgr_800x600, &screen_info, 100, 40, 500, 300, 1, G2D_ROTATION_0, 0);

	draw_image_to_framebuffer(480, 360, 480*360*2, G2D_BGR565, (unsigned char *) fsl_logo_bgr_480x360, &screen_info, 350, 260, 400, 300, 0, G2D_ROTATION_0, 0);

	draw_image_to_framebuffer(800, 600, 800*600*2, G2D_BGR565, (unsigned char *) ginger_bgr_800x600, &screen_info, 650, 450, 300, 200, 1, G2D_ROTATION_90, 0);

	draw_image_to_framebuffer(800, 600, 800*600*2, G2D_BGR565, (unsigned char *) ginger_bgr_800x600, &screen_info, 50, 400, 300, 200, 0, G2D_ROTATION_180, 0);

	draw_image_to_framebuffer(176, 144, 176*144*3/2, G2D_I420, akiyo_I420_176x144, &screen_info, 550, 40, 150, 120, 0, G2D_ROTATION_0, 0);

	draw_image_to_framebuffer(352, 288, 352*288*2, G2D_NV16, NV16_352x288, &screen_info, 0, 620, 176, 144, 1, G2D_ROTATION_0, 0);

	draw_image_to_framebuffer(352, 288, 352*288*2, G2D_YUYV, yuyv_352x288, &screen_info, 420, 620, 176, 144, 1, G2D_ROTATION_0, 0);

	gettimeofday(&tv2, NULL);

	printf("Overlay rendering time %dus .\n",(int)((tv2.tv_sec-tv1.tv_sec)*1000000 + tv2.tv_usec-tv1.tv_usec));

    usleep(6000000);

    /* g2d_blit with blur effect */
    clear_screen_with_g2d(&screen_info, 0xff000000);

	gettimeofday(&tv1, NULL);

	draw_image_to_framebuffer(1024, 768, 1024*768*2, G2D_RGB565, (unsigned char *)wall_1024x768_565rgb, &screen_info, 0, 0, 1024, 768, 1, G2D_ROTATION_0, 1);

	draw_image_to_framebuffer(800, 600, 800*600*2, G2D_BGR565, (unsigned char *) ginger_bgr_800x600, &screen_info, 100, 40, 500, 300, 1, G2D_ROTATION_0, 1);

	draw_image_to_framebuffer(480, 360, 480*360*2, G2D_BGR565, (unsigned char *) fsl_logo_bgr_480x360, &screen_info, 350, 260, 400, 300, 0, G2D_ROTATION_0, 1);

	draw_image_to_framebuffer(800, 600, 800*600*2, G2D_BGR565, (unsigned char *) ginger_bgr_800x600, &screen_info, 650, 450, 300, 200, 1, G2D_ROTATION_90, 1);

	draw_image_to_framebuffer(800, 600, 800*600*2, G2D_BGR565, (unsigned char *) ginger_bgr_800x600, &screen_info, 50, 400, 300, 200, 0, G2D_ROTATION_180, 1);

	draw_image_to_framebuffer(176, 144, 176*144*3/2, G2D_I420, akiyo_I420_176x144, &screen_info, 550, 40, 150, 120, 0, G2D_ROTATION_0, 1);

	draw_image_to_framebuffer(352, 288, 352*288*2, G2D_NV16, NV16_352x288, &screen_info, 0, 620, 176, 144, 1, G2D_ROTATION_0, 1);

	draw_image_to_framebuffer(352, 288, 352*288*2, G2D_YUYV, yuyv_352x288, &screen_info, 420, 620, 176, 144, 1, G2D_ROTATION_0, 1);

	gettimeofday(&tv2, NULL);

	printf("Overlay rendering with blur effect time %dus .\n",(int)((tv2.tv_sec-tv1.tv_sec)*1000000 + tv2.tv_usec-tv1.tv_usec));

    usleep(6000000);

    /* overlay test with multiblit */
    clear_screen_with_g2d(&screen_info, 0xffffffff);

    gettimeofday(&tv1, NULL);

    Test_g2d_multi_blit(&screen_info);

    gettimeofday(&tv2, NULL);

    printf("Overlay rendering with multiblit time %dus .\n",(int)((tv2.tv_sec-tv1.tv_sec)*1000000 + tv2.tv_usec-tv1.tv_usec));

    
err2:
        close(fd_fb0);
err0:
        return retval;
}
