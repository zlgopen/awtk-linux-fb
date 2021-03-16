/**
 * File:   lcd_linux_egl.c
 * Author: AWTK Develop Team
 * Brief:  linux egl lcd
 *
 * Copyright (c) 2020 - 2020  Guangzhou ZHIYUAN Electronics Co.,Ltd.
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
 * 2020-11-06 Lou ZhiMing <luozhiming@zlg.com> created
 *
 */

#ifdef WITH_LINUX_EGL

#include <signal.h>
#include "tkc/mem.h"
#include "glad/glad.h"
#include "awtk_global.h"
#include "lcd_linux_egl.h"
#include "../egl_devices/egl_devices.h"
#include "native_window/native_window_fb_gl.h"

static void on_app_exit(void) {

}

static void on_signal_int(int sig) {
  tk_quit();
}

static ret_t lcd_linux_gles_swap_buffer(native_window_t* win) {
  lcd_egl_context_t* lcd = NULL;
  return_value_if_fail(win != NULL, RET_BAD_PARAMS);

  lcd = (lcd_egl_context_t*)(win->handle);
  return_value_if_fail(lcd != NULL, RET_BAD_PARAMS);

  return egl_devices_swap_buffers(lcd->elg_ctx);
}

static ret_t lcd_linux_gles_make_current(native_window_t* win) {
  ret_t ret = RET_OK;
  lcd_egl_context_t* lcd = NULL;
  return_value_if_fail(win != NULL, RET_BAD_PARAMS);

  lcd = (lcd_egl_context_t*)(win->handle);
  return_value_if_fail(lcd != NULL, RET_BAD_PARAMS);

  ret = egl_devices_make_current(lcd->elg_ctx);

  if (ret == RET_OK) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    glViewport(0, 0, lcd->w, lcd->h);
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
  }
  return ret;
}

static ret_t lcd_linux_gles_destroy(native_window_t* win) {
  ret_t ret = RET_OK;
  lcd_egl_context_t* lcd = NULL;
  return_value_if_fail(win != NULL, RET_BAD_PARAMS);

  lcd = (lcd_egl_context_t*)(win->handle);
  return_value_if_fail(lcd != NULL, RET_BAD_PARAMS);

  ret = egl_devices_dispose(lcd->elg_ctx);
  if (ret == RET_OK) {
    TKMEM_FREE(lcd);
  }
  return ret;
}

lcd_egl_context_t* lcd_linux_egl_create(const char* filename) {
  native_window_t* win = NULL;
  lcd_egl_context_t* lcd = TKMEM_ZALLOC(lcd_egl_context_t);
  return_value_if_fail(lcd != NULL, NULL);

  lcd->elg_ctx = egl_devices_create(filename);
  goto_error_if_fail(lcd->elg_ctx != NULL);

  lcd->w = egl_devices_get_width(lcd->elg_ctx);
  lcd->h = egl_devices_get_height(lcd->elg_ctx);
  lcd->ratio = egl_devices_get_ratio(lcd->elg_ctx);

  win = native_window_fb_gl_init(lcd->w, lcd->h, lcd->ratio);
  goto_error_if_fail(win != NULL);

  win->handle = (void*)lcd;
  native_window_fb_gl_set_swap_buffer_func(win, lcd_linux_gles_swap_buffer);
  native_window_fb_gl_set_make_current_func(win, lcd_linux_gles_make_current);
  native_window_fb_gl_set_destroy_func(win, lcd_linux_gles_destroy);

  atexit(on_app_exit);
  signal(SIGINT, on_signal_int);

  return lcd;
error :
  native_window_fb_gl_deinit();
  return NULL;
}

#endif /*WITH_LINUX_EGL*/
