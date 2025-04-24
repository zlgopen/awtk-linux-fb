/**
 * File:   wayland_tools.h
 * Author: AWTK Develop Team
 * Brief:  wayland tools
 *
 * Copyright (c) 2018 - 2024 Guangzhou ZHIYUAN Electronics Co.,Ltd.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * License file for more details.
 *
 */

/**
 * History:
 * ================================================================
 * 2024-07-17 Yang Zewu <yangzewu@zlg.cn> created
 *
 */

#ifndef WAYLAND_TOOLS_H
#define WAYLAND_TOOLS_H

#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include <linux/input.h>
#include <wayland-client.h>
#include <wayland-cursor.h>
#include <xkbcommon/xkbcommon.h>
#include "tkc/types_def.h"

#include "pthread_signal.h"
#include "../protocol/xdg-shell-protocol.h"
#include "../protocol/fullscreen-shell-protocol.h"

#define container_of(ptr, type, member) ({                   \
        const typeof(((type*)0)->member)* __mptr = (ptr);    \
        (type*)((char*)__mptr - offsetof(type, member));})

static void do_nothing () {}

typedef void (*pointer_dispatch_t)(int32_t state, int32_t button, int32_t x, int32_t y);
typedef void (*keyboard_dispatch_t)(int32_t state, int32_t key);

typedef struct _wayland_keyboard_t {
  struct wl_keyboard* kbd;
  uint32_t buf_size;
  int32_t keymap_fd;
  struct xkb_context* ctx;
  struct xkb_keymap* map;
  struct xkb_state* kb_state;
  keyboard_dispatch_t keyboard_dispatch;
} wayland_keyboard_t;

typedef struct _wayland_pointer_t {
	struct wl_pointer* pointer;
	struct wl_surface* point_surface;
	struct wl_cursor_theme* cursor_theme;
	pointer_dispatch_t pointer_dispatch;
} wayland_pointer_t;

typedef struct _wayland_touch_t {
  struct wl_touch* touch;
  pointer_dispatch_t pointer_dispatch;
} wayland_touch_t;

typedef struct _input_bundle_t {
  wayland_keyboard_t keyboard;
  wayland_pointer_t mouse;
  wayland_touch_t touch;
  struct wl_seat* seat;
} input_bundle_t;

typedef struct _wayland_data_t {
  struct wl_display* display;
  struct wl_registry* registry;
  struct wl_compositor* compositor;
  struct wl_surface* surface;
  struct xdg_wm_base* xdg_wm_base;
  struct xdg_surface* xdg_surface;
  struct xdg_toplevel* xdg_toplevel;
  struct zwp_fullscreen_shell_v1* fullscreen_shell;
  struct wl_shm* shm;
  /* List of outputs */
  struct wl_list* monitors;
  input_bundle_t inputs;
} wayland_data_t;

typedef struct _output_info_t {
  int32_t width;
  int32_t height;
  int32_t hertz;
  int32_t transform;
} output_info_t;

typedef struct _wayland_output_t {
  struct wl_output* out;
  struct wl_list link;
  output_info_t info;
} wayland_output_t;

typedef struct _double_buffer_list_t {
  struct wl_buffer* wl_buffer;
  uint32_t* pixels;
  struct double_buffer_list* next;
} double_buffer_list_t;

typedef struct _buffer_t {
  ThreadSignal used;
  uint32_t width;
  uint32_t height;
  double_buffer_list_t* bufs;
} buffer_t;

typedef enum {
    SETUP_OK,
    NO_WAY_DISP,
    NO_REG,
    NO_COMP,
    NO_SHELL,
    NO_SEAT,
    NO_SHM,
    NO_MONITORS,
    NO_SURFACE,
    NO_SHELL_SURFACE,
    NO_TOPLEVEL
} WAYLAND_SETUP_ERR;

WAYLAND_SETUP_ERR setup_wayland(wayland_data_t* objs, bool_t fullscreen);
void destroy_wayland_data(wayland_data_t* objs);
void ref_display(struct wl_surface* surface, struct wl_buffer* buffer, int width, int height);
buffer_t* wayland_create_double_buffer(struct wl_shm* shm, int width, int height);

#endif /* WAYLAND_TOOLS_H */
