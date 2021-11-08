/*
 * Copyright 2021 NXP
 * Copyright 2016 Freescale Semiconductor, Inc.
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/*
 * g2d_wayland_cf.c
 */

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#include <wayland-client-protocol.h>
#include <wayland-client.h>
#include <wayland-server-protocol.h>

#include "test_context.h"
#include <g2dExt.h>

struct wl_display *g_display = NULL;
struct wl_compositor *g_compositor = NULL;
struct wl_surface *g_surface;
struct wl_shell *g_shell;
struct wl_shell_surface *g_shell_surface;
struct wl_shm *g_shm;
struct wl_buffer *g_buffer;
struct wl_callback *g_frame_callback;

static const struct wl_callback_listener frame_listener;

static void handle_ping(void *data, struct wl_shell_surface *shell_surface,
                        uint32_t serial) {
  wl_shell_surface_pong(shell_surface, serial);
  fprintf(stderr, "Pinged and ponged\n");
}

static void handle_configure(void *data, struct wl_shell_surface *shell_surface,
                             uint32_t edges, int32_t width, int32_t height) {}

static void handle_popup_done(void *data,
                              struct wl_shell_surface *shell_surface) {}

static const struct wl_shell_surface_listener shell_surface_listener = {
    handle_ping, handle_configure, handle_popup_done};

static void redraw(void *data, struct wl_callback *callback, uint32_t time) {
  if (!data)
    return;
  test_context *tc = (test_context *)data;

  wl_callback_destroy(g_frame_callback);
  wl_surface_damage(g_surface, 0, 0, tc->dst_width, tc->dst_height);

  paint_pixels(tc);

  g_frame_callback = wl_surface_frame(g_surface);
  wl_surface_attach(g_surface, g_buffer, 0, 0);
  wl_callback_add_listener(g_frame_callback, &frame_listener, tc);
  wl_surface_commit(g_surface);
}

static const struct wl_callback_listener frame_listener = {redraw};

static struct wl_buffer *create_buffer(test_context *tc) {
  struct wl_shm_pool *pool;
  int stride = tc->dst_width * 4; // 4 bytes per pixel
  int size = stride * tc->dst_height;
  int fd;
  struct wl_buffer *buff;

  void *g2dHandle;
  if (g2d_open(&g2dHandle) == -1 || g2dHandle == NULL) {
    fprintf(stderr, "g2d_open failed\n");
    return NULL;
  }
  struct g2d_buf *g2d_data = g2d_alloc(size, 0);
  if (g2d_data == NULL) {
    fprintf(stderr, "g2d_alloc failed\n");
    return NULL;
  }
  fd = g2d_buf_export_fd(g2d_data);
  if (fd < 0) {
    fprintf(stderr, "g2d_buf_export_fd failed\n");
    return NULL;
  }

  tc->dst_vaddr = g2d_data->buf_vaddr;
  tc->dst_paddr = g2d_data->buf_paddr;

  pool = wl_shm_create_pool(g_shm, fd, size);
  buff = wl_shm_pool_create_buffer(pool, 0, tc->dst_width, tc->dst_height,
                                   stride, WL_SHM_FORMAT_ARGB8888);
  // wl_buffer_add_listener(g_buffer, &buffer_listener, g_buffer);
  wl_shm_pool_destroy(pool);
  return buff;
}

static void create_window(test_context *tc) {
  g_buffer = create_buffer(tc);

  wl_surface_attach(g_surface, g_buffer, 0, 0);
  // wl_surface_damage(g_surface, 0, 0, g_width, g_height);
  wl_surface_commit(g_surface);
}

static void shm_format(void *data, struct wl_shm *wl_shm, uint32_t format) {
  char *s;
  switch (format) {
  case WL_SHM_FORMAT_ARGB8888:
    s = "ARGB8888";
    break;
  case WL_SHM_FORMAT_XRGB8888:
    s = "XRGB8888";
    break;
  case WL_SHM_FORMAT_RGB565:
    s = "RGB565";
    break;
  case WL_SHM_FORMAT_RGBA8888:
    s = "RGBA8888";
    break;
  case WL_SHM_FORMAT_YUV420:
    s = "YUV420";
    break;
  case WL_SHM_FORMAT_NV12:
    s = "NV12";
    break;
  case WL_SHM_FORMAT_YUYV:
    s = "YUYV";
    break;
  default:
    s = "other format";
    break;
  }
  fprintf(stderr, "Possible shmem format: %s\t(0x%X)\n", s, format);
}

struct wl_shm_listener shm_listener = {shm_format};

static void global_registry_handler(void *data, struct wl_registry *registry,
                                    uint32_t id, const char *interface,
                                    uint32_t version) {
  if (strcmp(interface, "wl_compositor") == 0) {
    g_compositor = wl_registry_bind(registry, id, &wl_compositor_interface, 1);
  } else if (strcmp(interface, "wl_shell") == 0) {
    g_shell = wl_registry_bind(registry, id, &wl_shell_interface, 1);
  } else if (strcmp(interface, "wl_shm") == 0) {
    g_shm = wl_registry_bind(registry, id, &wl_shm_interface, 1);
    wl_shm_add_listener(g_shm, &shm_listener, NULL);
  }
}

static void global_registry_remover(void *data, struct wl_registry *registry,
                                    uint32_t id) {
  fprintf(stderr, "Got a registry losing event for %d\n", id);
}

static const struct wl_registry_listener registry_listener = {
    global_registry_handler, global_registry_remover};

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

  test_context *tc = test_context_alloc(1024, 768);

  if (!set_frame_callback(&g_frame_callback, tc))
    exit(1);

  if (!shell_surface_create(g_surface, &g_shell_surface))
    exit(1);

  create_window(tc);
  redraw(NULL, NULL, 0);

  while (wl_display_dispatch(g_display) != -1) {
    ;
  }

  wl_display_disconnect(g_display);
  fprintf(stderr, "Disconnected from display\n");

  exit(0);
}
