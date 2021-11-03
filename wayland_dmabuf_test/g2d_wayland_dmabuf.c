/*
 * Copyright 2021 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/*
 * g2d_wayland_dmabuf.c
 */

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#include <drm/drm_fourcc.h>
#include <wayland-client-protocol.h>
#include <wayland-client.h>
#include <wayland-server-protocol.h>

#include "linux_dmabuf_wp.h"
#include "test_context.h"
#include <g2dExt.h>

struct wl_display *g_display = NULL;
struct wl_compositor *g_compositor = NULL;
struct wl_surface *g_surface;
struct wl_shell *g_shell;
struct wl_shell_surface *g_shell_surface;
struct wl_callback *g_frame_callback;
struct zwp_linux_dmabuf_v1 *g_dmabuf = NULL;
bool g_xrgb8888_format_found = 0;

static const struct wl_callback_listener frame_listener;

static void handle_ping(void *data, struct wl_shell_surface *shell_surface,
                        uint32_t serial) {
  wl_shell_surface_pong(shell_surface, serial);
  fprintf(stderr, "Pinged and ponged\n");
}

static void handle_configure(void *data, struct wl_shell_surface *shell_surface,
                             uint32_t edges, int32_t width, int32_t height) {
  fprintf(stderr, "Pinged and ponged\n");
}

static void handle_popup_done(void *data,
                              struct wl_shell_surface *shell_surface) {}

static const struct wl_shell_surface_listener shell_surface_listener = {
    handle_ping, handle_configure, handle_popup_done};

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

static struct wl_buffer *
linux_dmabuf_construct_wl_buffer(test_context *tc, struct client_buffer *);

static void *window_next_buffer(test_context *tc) {
  struct client_buffer *client_buffer;
  int ret = 0;
  int bufid = 0;
  if (!tc->dmabuffers[0]->busy) {
    client_buffer = tc->dmabuffers[0];
    bufid = 0;
  } else if (!tc->dmabuffers[1]->busy) {
    client_buffer = tc->dmabuffers[1];
    bufid = 1;
  } else {
    bufid = -1;
    fprintf(stderr, "window_next_buffer failed\n");
    return NULL;
  }

  if (!client_buffer->wlbuffer) {
    ret = linux_dmabuf_construct_wl_buffer(tc, client_buffer);

    if (ret < 0)
      return NULL;
  }

  return client_buffer;
}

static void redraw(void *data, struct wl_callback *callback, uint32_t time) {

  struct client_buffer *client_buffer;
  if (!data)
    return;

  test_context *tc = (test_context *)data;

  client_buffer = window_next_buffer(tc);

  if (callback)
    wl_callback_destroy(g_frame_callback);

  wl_surface_damage(g_surface, 0, 0, tc->window_width, tc->window_height);
  test_paint_pixels(tc, client_buffer);

  g_frame_callback = wl_surface_frame(g_surface);
  wl_surface_attach(g_surface, client_buffer->wlbuffer, 0, 0);

  wl_callback_add_listener(g_frame_callback, &frame_listener, tc);
  wl_surface_commit(g_surface);

  client_buffer->busy = 1;
}

static const struct wl_callback_listener frame_listener = {redraw};

struct client_buffer *test_buffer_new() {
  struct client_buffer *buf =
      (struct client_buffer *)calloc(sizeof(struct client_buffer), 1);

  return buf;
}

static bool create_g2d_buffer(test_context *tc, struct client_buffer *buf) {
  int stride = tc->window_width * 4; // 4 bytes per pixel
  int size = stride * tc->window_height;

  void *g2dHandle;
  if (g2d_open(&g2dHandle) == -1 || g2dHandle == NULL) {
    fprintf(stderr, "create_buffer: g2d_open failed\n");
    return false;
  }

  buf->g2d_data = g2d_alloc(size, 0);
  if (buf->g2d_data == NULL) {
    fprintf(stderr, "create_buffer: g2d_alloc failed\n");
    return false;
  }

  buf->dmabuf_fd = g2d_buf_export_fd(buf->g2d_data);
  if (buf->dmabuf_fd < 0) {
    fprintf(stderr, "create_buffer: g2d_buf_export_fd failed\n");
    return false;
  }

  return true;
}

static struct wl_buffer *create_wl_buffer(test_context *tc) { return NULL; }

static bool create_window(test_context *tc) {
  wl_surface_attach(g_surface, tc->dmabuffers[0]->wlbuffer, 0, 0);
  wl_surface_damage(g_surface, 0, 0, tc->window_width, tc->window_height);
  wl_surface_commit(g_surface);
  test_setup(tc);

  fprintf(stderr, "create_window done\n");
  return true;
}

static void dmabuf_format(void *data,
                          struct zwp_linux_dmabuf_v1 *zwp_linux_dmabuf,
                          uint32_t format) {
  char *s;
  switch (format) {
  case DRM_FORMAT_ARGB8888:
    s = "ARGB8888";
    break;
  case DRM_FORMAT_XRGB8888:
    s = "XRGB8888";
    break;
  case DRM_FORMAT_XBGR8888:
    s = "XBGR8888";
    break;
  case DRM_FORMAT_RGB565:
    s = "RGB565";
    break;
  case DRM_FORMAT_RGBX8888:
    s = "RGBX8888";
    break;
  case DRM_FORMAT_RGBA8888:
    s = "RGBA8888";
    break;
  case DRM_FORMAT_BGRX8888:
    s = "BGRX8888";
    break;
  case DRM_FORMAT_YUV420:
    s = "YUV420";
    break;
  case DRM_FORMAT_NV12:
    s = "NV12";
    break;
  case DRM_FORMAT_NV21:
    s = "NV21";
    break;
  case DRM_FORMAT_YUYV:
    s = "YUYV";
    break;
  case DRM_FORMAT_UYVY:
    s = "UYVY";
    break;
  default:
    s = "other dma-buf format";
    break;
  }
  fprintf(stderr, "Possible dma-buf format: %s\t(0x%X)\n", s, format);
}

static const struct zwp_linux_dmabuf_v1_listener dmabuf_listener = {
    dmabuf_format};

static void registry_handle_global(void *data, struct wl_registry *registry,
                                   uint32_t id, const char *interface,
                                   uint32_t version) {
  if (strcmp(interface, "wl_compositor") == 0) {
    g_compositor = wl_registry_bind(registry, id, &wl_compositor_interface, 1);
  } else if (strcmp(interface, "wl_shell") == 0) {
    g_shell = wl_registry_bind(registry, id, &wl_shell_interface, 1);
  } else if (strcmp(interface, "zwp_linux_dmabuf_v1") == 0) {
    g_dmabuf =
        wl_registry_bind(registry, id, &zwp_linux_dmabuf_v1_interface, 1);
    zwp_linux_dmabuf_v1_add_listener(g_dmabuf, &dmabuf_listener,
                                     NULL // user-data
    );
  }
}

static void registry_handle_global_remove(void *data,
                                          struct wl_registry *registry,
                                          uint32_t id) {
  fprintf(stderr, "Got a registry losing event for %d\n", id);
}

static const struct wl_registry_listener registry_listener = {
    registry_handle_global, registry_handle_global_remove};

///////////////////////////////////////
static bool display_connect(struct wl_display **display) {
  *display = wl_display_connect(NULL);
  if (*display == NULL) {
    fprintf(stderr, "Can't connect to display\n");
    return false;
  }
  return true;
}

static bool registry_setup(struct wl_display *display) {
  struct wl_registry *registry = wl_display_get_registry(display);

  wl_registry_add_listener(registry, &registry_listener, NULL);
  wl_display_dispatch(display);
  wl_display_roundtrip(display);

  return true;
}

static bool surface_create(struct wl_compositor *compositor,
                           struct wl_surface **surface) {
  *surface = wl_compositor_create_surface(compositor);
  if (*surface == NULL) {
    fprintf(stderr, "Can't create surface\n");
    return false;
  }
  fprintf(stderr, "Created surface\n");
  return true;
}

static bool shell_surface_create(struct wl_surface *surface,
                                 struct wl_shell_surface **shell_surface) {
  *shell_surface = wl_shell_get_shell_surface(g_shell, surface);
  if (*shell_surface == NULL) {
    fprintf(stderr, "Can't create shell surface\n");
    return false;
  }
  fprintf(stderr, "Created shell surface\n");
  wl_shell_surface_set_toplevel(*shell_surface);

  wl_shell_surface_add_listener(*shell_surface, &shell_surface_listener, NULL);
  return true;
}

static bool set_frame_callback(struct wl_callback **frame_callback,
                               test_context *tc) {
  *frame_callback = wl_surface_frame(g_surface);
  wl_callback_add_listener(*frame_callback, &frame_listener, tc);
  return true;
}
////
static void buffer_release(void *data, struct wl_buffer *client_buffer) {
  struct client_buffer *buf = data;
  buf->busy = 0;
}
static const struct wl_buffer_listener buffer_listener = {buffer_release};
static void dmabuf_create_succeeded(void *data,
                                    struct zwp_linux_buffer_params_v1 *params,
                                    struct wl_buffer *new_buffer) {
  struct client_buffer *client_buffer = data;
  client_buffer->wlbuffer = new_buffer;
  wl_buffer_add_listener(client_buffer->wlbuffer, &buffer_listener, data);
  fprintf(stderr, "dmabuf done\n");
}
static void dmabuf_create_failed(void *data,
                                 struct zwp_linux_buffer_params_v1 *params) {
  fprintf(stderr, "dmabuf failed\n");
}

static const struct zwp_linux_buffer_params_v1_listener params_listener = {
    dmabuf_create_succeeded, dmabuf_create_failed};

static struct wl_buffer *
linux_dmabuf_construct_wl_buffer(test_context *tc,
                                 struct client_buffer *client_buffer) {
  int rv = 0;
  void *user_data = NULL;
  struct zwp_linux_buffer_params_v1 *params;

  params = zwp_linux_dmabuf_v1_create_params(g_dmabuf);
  if (params == NULL) {
    fprintf(stderr, "zwp_linux_dmabuf_v1_create_params failed\n");
    return NULL;
  }

  if (client_buffer->g2d_data == NULL)
    create_g2d_buffer(tc, client_buffer);

  zwp_linux_buffer_params_v1_add(params, client_buffer->dmabuf_fd,
                                 0 // plane index
                                 ,
                                 0 // offset
                                 ,
                                 0 // client_buffer->stride //stride
                                 ,
                                 0 >> 32, 0 & 0xffffffff);
  rv = zwp_linux_buffer_params_v1_add_listener(params, &params_listener,
                                               client_buffer);
  if (rv == -1) {
    fprintf(stderr, "zwp_linux_buffer_params_v1_add_listener failed\n");
    return NULL;
  }

  zwp_linux_buffer_params_v1_create(params, tc->window_width, tc->window_height,
                                    DRM_FORMAT_XRGB8888, 0 // flags
  );
  wl_display_roundtrip(g_display);
  return (void *)0x01;
}

int main(int argc, char **argv) {
  if (!display_connect(&g_display))
    exit(1);

  if (!registry_setup(g_display))
    exit(1);

  if (g_compositor == NULL) {
    fprintf(stderr, "Can't find g_compositor\n");
    exit(1);
  }

  if (!surface_create(g_compositor, &g_surface))
    exit(1);

  test_context *tc = test_context_alloc(1920, 768);
  tc->dmabuffers[0] = test_buffer_new();
  tc->dmabuffers[1] = test_buffer_new();
  if (!set_frame_callback(&g_frame_callback, tc))
    exit(1);

  if (!shell_surface_create(g_surface, &g_shell_surface))
    exit(1);

  if (!create_window(tc))
    exit(1);

  test_setup(tc);

  redraw(tc, NULL, 0);

  while (wl_display_dispatch(g_display) != -1) {
    ;
  }

  wl_display_disconnect(g_display);
  printf("disconnected from display\n");

  exit(0);
}
