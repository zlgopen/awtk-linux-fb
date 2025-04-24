/**
 * File:   main_loop_wayland.c
 * Author: AWTK Develop Team
 * Brief:  main loop for wayland
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

#include <poll.h>
#include "main_loop/main_loop_simple.h"
#include "tkc/thread.h"

#include "lcd_wayland.h"

static void *wayland_run(void* ctx);

static ret_t main_loop_wayland_destroy(main_loop_t* l) {
  main_loop_simple_t* loop = (main_loop_simple_t*)l;

  main_loop_simple_reset(loop);
  native_window_raw_deinit();

  return RET_OK;
}

main_loop_t* main_loop_init(int w, int h) {
  lcd_t* lcd = lcd_wayland_create(w, h);
  return_value_if_fail(lcd != NULL, NULL);

  native_window_raw_init(lcd);
  main_loop_simple_t* loop = main_loop_simple_init(lcd->w, lcd->h, NULL, NULL);

  loop->base.destroy = main_loop_wayland_destroy;

  tk_thread_t* thread = tk_thread_create(wayland_run, lcd);
  if (thread != NULL) {
    tk_thread_start(thread);
  }

  return (main_loop_t*)loop;
}

static void PlatformPollEvents(wayland_data_t* objs) {
  struct wl_display* display = objs->display;
  struct pollfd fds[] = {
      { wl_display_get_fd(display), POLLIN },
  };

  while (wl_display_prepare_read(display) != 0)
      wl_display_dispatch_pending(display);

  if (poll(fds, 1, -1) > 0) {
      wl_display_read_events(display);
      wl_display_dispatch_pending(display);
  } else {
      wl_display_cancel_read(display);
  }
  kb_repeat(NULL);
}

static void* wayland_run(void* ctx) {
  lcd_wayland_t* lw = ((lcd_t*)ctx)->impl_data;

  while (1) {
    PlatformPollEvents(&lw->objs);
  }
}

