/* vim: set ts=8 sw=8 tw=0 noet : */

/* Copyright 2022-2023 NXP */

#include <cairo.h>
#include <errno.h>
#include <fcntl.h>
#include <math.h>
#include <stdbool.h>
#include <getopt.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>

#include "g2dExt.h"
#include "warp_buffer.h"
#include "warp_buffer_1080p.h"
#include "warp_buffer_4k.h"
#include "dewarp_buffer.h"
#include "dewarp_buffer_1080p.h"
#include "dewarp_buffer_4k.h"

static const struct option longOptions[] = {
	{"help", no_argument, NULL, 'h'},
	{"mode", required_argument, NULL, 'm'},
	{NULL, 0, NULL, 0}};

struct ctx {
	void *handle;
	struct g2d_surface src;
	struct g2d_surface dst;
	struct g2d_warp_coordinates coord;

	struct g2d_buf *s_buf;
	struct g2d_buf *d_buf;
	struct g2d_buf *coord_buf;
};

static void usage() {
fprintf(stderr, "Usage: cmd [options]\n"
				"\n"
				"Options:\n"
				"  -m, --mode  Warp Dewarp test mode.\n"
				"      mode 1: 800x480\n"
				"      mode 2: 1920x1080\n"
				"      mode 3: 3840x2160\n"
				"  -h, --help  Show this message.\n"
				"\n");
}

static uint32_t get_buffer_size(uint16_t width, uint16_t height, enum g2d_format format)
{
	switch (format) {
	case G2D_RGBA8888:
	case G2D_RGBX8888:
	case G2D_BGRA8888:
	case G2D_BGRX8888:
	case G2D_ARGB8888:
	case G2D_ABGR8888:
	case G2D_XRGB8888:
	case G2D_XBGR8888:
		return width * height * 4;

	case G2D_RGB888:
	case G2D_BGR888:
		return width * height * 3;

	case G2D_RGB565:
	case G2D_BGR565:
		return width * height * 2;

	case G2D_NV12:
	case G2D_NV21:
		return width * height * 3 / 2;

	case G2D_YUYV:
	case G2D_YVYU:
	case G2D_UYVY:
	case G2D_VYUY:
		return width * height * 2;

	default:
		fprintf(stderr, "Unsupported format!\n");
		exit(EXIT_FAILURE);
	}

	return 0;
}

static int g2d_init(struct ctx *ctx, int width, int height,
		    enum g2d_format in_format, enum g2d_format out_format,
		    enum  g2d_warp_map_format coord_format)
{
	int can_warp;

	if (g2d_open(&ctx->handle)) {
		printf("g2d_open fail.\n");
		return -ENOTTY;
	}

	g2d_query_feature(ctx->handle, G2D_WARP_DEWARP, &can_warp);
	if (!can_warp) {
		fprintf(stderr, "G2D device cannot perform warp/dewarp operations\n");
		return -1;
	}

	ctx->s_buf = g2d_alloc(get_buffer_size(width, height, in_format), 0);
	ctx->d_buf = g2d_alloc(get_buffer_size(width, height, out_format), 0);

	ctx->src.left = 0;
	ctx->src.top = 0;
	ctx->src.right = width;
	ctx->src.bottom = height;
	ctx->src.width = width;
	ctx->src.height = height;
	ctx->src.format = in_format;
	ctx->src.stride = width;
	ctx->src.planes[0] = ctx->s_buf->buf_paddr;
	if (in_format == G2D_NV12 ||
	    in_format == G2D_NV21)
		ctx->src.planes[1] = ctx->s_buf->buf_paddr +
				     width * height;

	ctx->dst.left = 0;
	ctx->dst.top = 0;
	ctx->dst.right = width;
	ctx->dst.bottom = height;
	ctx->dst.width = width;
	ctx->dst.height = height;
	ctx->dst.format = out_format;
	ctx->dst.stride = width;
	ctx->dst.planes[0] = ctx->d_buf->buf_paddr;
	if (out_format == G2D_NV12 ||
	    out_format == G2D_NV21)
		ctx->dst.planes[1] = ctx->d_buf->buf_paddr +
				     width * height;

	ctx->coord_buf = g2d_alloc(width * height * 4, 0);
	ctx->coord.addr = ctx->coord_buf->buf_paddr;
	ctx->coord.width = width;
	ctx->coord.height = height;
	ctx->coord.format = coord_format;
	ctx->coord.bpp = 32;

	if (coord_format == G2D_WARP_MAP_DDPNT) {
		ctx->coord.bpp = 8;
	}

	return 0;
}

static void g2d_deinit(struct ctx *ctx)
{
	g2d_free(ctx->s_buf);
	g2d_free(ctx->d_buf);
	g2d_free(ctx->coord_buf);

	g2d_close(ctx->handle);
}

int read_input_file(struct ctx *ctx, char *in_file_name,
		uint8_t *buf, uint32_t buf_size)
{
	int in_fd;
	struct stat file_stat;
	ssize_t nread;
	uint32_t in_file_size;
	uint32_t bytes_remaining;

	in_fd = open(in_file_name, O_RDONLY);
	fstat(in_fd, &file_stat);
	in_file_size = file_stat.st_size;

	if (in_file_size > buf_size) {
		fprintf(stderr, "Buffer size is less than input file size.\n");
		close(in_fd);
		return -1;
	}

	bytes_remaining = in_file_size;
	while (bytes_remaining) {
		nread = read(in_fd, buf, in_file_size);
		if (nread < 0) {
			fprintf(stderr, "cannot read file: %s\n",
					strerror(errno));
			fprintf(stderr, "remaining bytes to read: %d\n",
					bytes_remaining);
			close(in_fd);
			return -1;
		}

		buf += nread;
		bytes_remaining -= nread;
	}

	close(in_fd);

	return 0;
}

static int read_png_file(char *in_file_name, void *buf)
{
	cairo_surface_t *surface;
	int stride, height;

	surface = cairo_image_surface_create_from_png(in_file_name);
	height = cairo_image_surface_get_height(surface);
	stride = cairo_image_surface_get_stride(surface);

	memcpy(buf, cairo_image_surface_get_data(surface), stride * height);

	cairo_surface_destroy(surface);

	return 0;
}

static int write_png_file(char *out_file_name, void *buf, int width, int height)
{
	cairo_surface_t *surface;
	cairo_t *cr;
	cairo_format_t cairo_format = CAIRO_FORMAT_ARGB32;
	int stride = width * 4;
	int x, y;

	surface = cairo_image_surface_create_for_data(buf,
						      cairo_format,
						      width, height,
						      stride);

	cr = cairo_create(surface);
	cairo_surface_destroy(surface);

	cairo_surface_write_to_png(cairo_get_target(cr), out_file_name);

	cairo_destroy(cr);

	return 0;
}

static int draw_sphere(cairo_t *cr, int x, int y, float radius)
{
	cairo_pattern_t *pat;

	pat = cairo_pattern_create_radial(x + 2, y + 2, 5, x + 5, y + 5, radius);
	cairo_pattern_add_color_stop_rgba(pat, 0, 1, 1, 1, 1);
	cairo_pattern_add_color_stop_rgba(pat, 1, 0, 0, 0, 1);

	cairo_set_source(cr, pat);

	cairo_arc(cr, x, y, radius, 0, 2 * M_PI);
	cairo_fill(cr);

	cairo_pattern_destroy(pat);

	return 0;
}

static int draw_color_strips(cairo_t *cr, int x, int y, int width, int height)
{
	int i;
	cairo_pattern_t *pat;
	struct pat_stop {
		float stop1[3];
		float stop2[3];
	};
	struct pat_stop pat_stops[] = {
		{.stop1 = {0.0, 0.0, 0.0}, .stop2 = {1.0, 1.0, 1.0}},
		{.stop1 = {0.0, 0.0, 0.0}, .stop2 = {1.0, 0.0, 0.0}},
		{.stop1 = {0.0, 0.0, 0.0}, .stop2 = {0.0, 1.0, 0.0}},
		{.stop1 = {0.0, 0.0, 0.0}, .stop2 = {0.0, 0.0, 1.0}},
		{.stop1 = {0.0, 0.0, 0.0}, .stop2 = {1.0, 1.0, 0.0}},
		{.stop1 = {0.0, 0.0, 0.0}, .stop2 = {1.0, 0.0, 1.0}},
		{.stop1 = {0.0, 0.0, 0.0}, .stop2 = {0.0, 1.0, 1.0}},
	};

	for (i = 0; i < 7; i++) {
		pat = cairo_pattern_create_linear (0.0, 0.0, x + width, 0.0);
		cairo_pattern_add_color_stop_rgba(pat, 1,
						  pat_stops[i].stop1[0],
						  pat_stops[i].stop1[1],
						  pat_stops[i].stop1[2],
						  1);
		cairo_pattern_add_color_stop_rgba(pat, 0,
						  pat_stops[i].stop2[0],
						  pat_stops[i].stop2[1],
						  pat_stops[i].stop2[2],
						  1);

		cairo_set_source(cr, pat);

		cairo_rectangle(cr, x, y + i * height / 7, width, height / 7);
		cairo_fill(cr);

		cairo_pattern_destroy(pat);
	}

	return 0;
}

static int create_test_buffer(void *buf, int width, int height)
{
	cairo_surface_t *surface;
	cairo_t *cr;
	cairo_format_t cairo_format = CAIRO_FORMAT_ARGB32;
	int stride = width * 4;
	int x, y;

	surface = cairo_image_surface_create_for_data(buf,
						      cairo_format,
						      width, height,
						      stride);

	memset(buf, 0xff, height * stride);

	cr = cairo_create(surface);
	cairo_surface_destroy(surface);

	cairo_set_line_cap(cr, CAIRO_LINE_CAP_SQUARE);

	cairo_set_line_width(cr, 2);
	cairo_set_source_rgba(cr, 0, 0, 0, 1);

	for (x = 0; x < width; x += width / 8) {
		cairo_move_to(cr, x, 0);
		cairo_line_to(cr, x, height);
	}

	for (y = 0; y < height; y += height / 8) {
		cairo_move_to(cr, 0, y);
		cairo_line_to(cr, width, y);
	}

	cairo_move_to(cr, 0, 0);
	cairo_line_to(cr, width, height);
	cairo_move_to(cr, 0, height);
	cairo_line_to(cr, width, 0);

	cairo_stroke(cr);

	draw_sphere(cr, width / 4, height / 4, height / 8);
	draw_sphere(cr, width - width / 4, height / 4, height / 8);
	draw_sphere(cr, width / 4, height - height / 4, height / 8);
	draw_sphere(cr, width - width / 4, height - height / 4, height / 8);

	draw_color_strips(cr, width / 4, 3 * height / 8,
			  width / 2, height / 4);

	cairo_surface_write_to_png(cairo_get_target(cr), "input.png");

	cairo_destroy(cr);
}

int main(int argc, char **argv)
{
	struct ctx ctx = {0,};
	int ret;
	int mode, fb_width, fb_height, coord_buffer_size;
	void *warp_coord_absolute;
	void *dewarp_coord_absolute;
	struct timeval tv1, tv2;
	int i, diff;
	int test_loop = 16;

	mode = 2;
	fb_width = 1920;
	fb_height = 1080;
	coord_buffer_size = fb_width * fb_height;
	warp_coord_absolute = warp_coord_absolute_1920_1080;
	dewarp_coord_absolute = dewarp_coord_absolute_1920_1080;

	while (true) {
		int optionIndex;
		int ic =
			getopt_long(argc, argv, "hm:1", longOptions, &optionIndex);
		if (ic == -1) {
			break;
		}

		switch (ic) {
			case 'h':
				usage();
				return 0;
				break;
			case 'm':
				if (!strcmp(optarg, "1")) {
					mode = 1;
					fb_width = 800;
					fb_height = 480;
					coord_buffer_size = fb_width * fb_height * 4;
					warp_coord_absolute = warp_coord_absolute_800_480;
					dewarp_coord_absolute = dewarp_coord_absolute_800_480;
				} else if (!strcmp(optarg, "2")) {
					mode = 2;
					fb_width = 1920;
					fb_height = 1080;
					coord_buffer_size = fb_width * fb_height;
					warp_coord_absolute = warp_coord_absolute_1920_1080;
					dewarp_coord_absolute = dewarp_coord_absolute_1920_1080;
				} else if (!strcmp(optarg, "3")) {
					mode = 3;
					fb_width = 3840;
					fb_height = 2160;
					coord_buffer_size = fb_width * fb_height;
					warp_coord_absolute = warp_coord_absolute_3840_2160;
					dewarp_coord_absolute = dewarp_coord_absolute_3840_2160;
				} else {
					fprintf(stderr, "Invalid mode '%s', must be 1, 2, 3\n", optarg);
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

	printf("Mode: %d, Width: %d, Height: %d\n", mode, fb_width, fb_height);

	/* mode 1 uses PNT WarpCoordinateMode: x and y (sample points), 32 WarpBitsPerPixel.
	 * Both mode 2 (default mode) and 3 use DD_PNT WarpCoordinateMode: ddx and ddy
	 * (deltas between adjacent vectors), 8 WarpBitsPerPixel.
	 */
	if (mode == 1) {
		ret = g2d_init(&ctx, fb_width, fb_height,
			G2D_BGRA8888, G2D_BGRA8888, G2D_WARP_MAP_PNT);
	} else {
		ret = g2d_init(&ctx, fb_width, fb_height,
			G2D_BGRA8888, G2D_BGRA8888, G2D_WARP_MAP_DDPNT);
	}

	if (ret < 0) {
		g2d_close(ctx.handle);
		exit(EXIT_FAILURE);
	}

	create_test_buffer(ctx.s_buf->buf_vaddr, fb_width, fb_height);

	/* copy the warping coordinates buffer to the contiguous allocated memory */
	memcpy(ctx.coord_buf->buf_vaddr, warp_coord_absolute,
		coord_buffer_size);

	/* parameters for calibration settings required by DD_PNT WarpCoordinateMode */
	if (mode == 2) {
		ctx.coord.arb_start_x = 0x1fb58f;
		ctx.coord.arb_start_y = 0x1fd5ec;
		ctx.coord.arb_delta_xx = 0x22;
		ctx.coord.arb_delta_xy = 0xf6;
		ctx.coord.arb_delta_yx = 0xf6;
		ctx.coord.arb_delta_yy = 0x2e;
	} else if (mode == 3) {
		ctx.coord.arb_start_x = 0x1f6b12;
		ctx.coord.arb_start_y = 0x1fac07;
		ctx.coord.arb_delta_xx = 0x22;
		ctx.coord.arb_delta_xy = 0xf6;
		ctx.coord.arb_delta_yx = 0xf6;
		ctx.coord.arb_delta_yy = 0x2e;
	}

	gettimeofday(&tv1, NULL);
	for(i = 0; i < test_loop; i++){
		/* perform warp operation */
		g2d_enable(ctx.handle, G2D_WARPING);
		g2d_set_warp_coordinates(ctx.handle, &ctx.coord);
		g2d_blit(ctx.handle, &ctx.src, &ctx.dst);
		g2d_disable(ctx.handle, G2D_WARPING);
		g2d_finish(ctx.handle);
	}
	gettimeofday(&tv2, NULL);
	diff = ((tv2.tv_sec - tv1.tv_sec) * 1000000 + (tv2.tv_usec - tv1.tv_usec)) /
			test_loop;
	printf("g2d warp time %dus, %dfps, %dMpixel/s ........\n", diff,
			1000000 / diff, fb_width * fb_height / diff);

	/* write the warped buffer */
	write_png_file("output_warped.png",
		       ctx.d_buf->buf_vaddr, fb_width, fb_height);

	/* === now test that de-warping works === */

	/* copy the warped buffer to the source buffer */
	memcpy(ctx.s_buf->buf_vaddr, ctx.d_buf->buf_vaddr,
	       get_buffer_size(fb_width, fb_height, G2D_BGRA8888));

	/* copy the dewarping coordinates buffer to the contiguous allocated memory */
	memcpy(ctx.coord_buf->buf_vaddr, dewarp_coord_absolute,
	       coord_buffer_size);

	if (mode == 2) {
		ctx.coord.arb_start_x = 0x286c;
		ctx.coord.arb_start_y = 0x16a6;
		ctx.coord.arb_delta_xx = 0x0e;
		ctx.coord.arb_delta_xy = 0xfc;
		ctx.coord.arb_delta_yx = 0xfc;
		ctx.coord.arb_delta_yy = 0x14;
	} else if (mode == 3) {
		ctx.coord.arb_start_x = 0x50d4;
		ctx.coord.arb_start_y = 0x2d64;
		ctx.coord.arb_delta_xx = 0x0e;
		ctx.coord.arb_delta_xy = 0xfa;
		ctx.coord.arb_delta_yx = 0xfc;
		ctx.coord.arb_delta_yy = 0x12;
	}

	gettimeofday(&tv1, NULL);
	for(i = 0; i < test_loop; i++){
		/* perform de-warp operation */
		g2d_enable(ctx.handle, G2D_WARPING);
		g2d_set_warp_coordinates(ctx.handle, &ctx.coord);
		g2d_blit(ctx.handle, &ctx.src, &ctx.dst);
		g2d_disable(ctx.handle, G2D_WARPING);
		g2d_finish(ctx.handle);
	}
	gettimeofday(&tv2, NULL);
	diff = ((tv2.tv_sec - tv1.tv_sec) * 1000000 + (tv2.tv_usec - tv1.tv_usec)) /
			test_loop;
	printf("g2d dewarp time %dus, %dfps, %dMpixel/s ........\n", diff,
			1000000 / diff, fb_width * fb_height / diff);

	/* write the de-warped buffer */
	write_png_file("output_dewarped.png",
		       ctx.d_buf->buf_vaddr, fb_width, fb_height);

	g2d_deinit(&ctx);

	printf("The test created the following files:\n");
	printf(" * input.png - the buffer used as input for the warp operation;\n");
	printf(" * output_warped.png - the result of the warp operation and input for the dewarp;\n");
	printf(" * output_dewarped.png - the result of the dewarp operation;\n");

	exit(EXIT_SUCCESS);
}
