/**
 * File:   main_loop_linux.c
 * Author: AWTK Develop Team
 * Brief:  linux implemented main_loop interface
 *
 * Copyright (c) 2018 - 2020  Guangzhou ZHIYUAN Electronics Co.,Ltd.
 *
 * this program is distributed in the hope that it will be useful,
 * but without any warranty; without even the implied warranty of
 * merchantability or fitness for a particular purpose.  see the
 * license file for more details.
 *
 */

/**
 * history:
 * ================================================================
 * 2018-09-09 li xianjing <xianjimli@hotmail.com> created
 *
 */

#include "base/idle.h"
#include "base/timer.h"
#include "base/font_manager.h"
#include "base/window_manager.h"
#include "main_loop/main_loop_simple.h"
#include "native_window/native_window_raw.h"

#include "tslib_thread.h"
#include "input_thread.h"
#include "mouse_thread.h"
#include "lcd_linux_fb.h"
#include "lcd_linux_drm.h"
#include "lcd_linux_egl.h"
#include "main_loop_linux.h"
#include "devices.h"

#ifdef WITH_LINUX_EGL
#define LCD_T lcd_egl_context_t
#else
#define LCD_T lcd_t
#endif

#ifndef FB_DEVICE_FILENAME
#define FB_DEVICE_FILENAME "/dev/fb0"
#endif /*FB_DEVICE_FILENAME*/

#ifndef DRM_DEVICE_FILENAME
#define DRM_DEVICE_FILENAME "/dev/dri/card0"
#endif /*DRM_DEVICE_FILENAME*/

#ifndef TS_DEVICE_FILENAME
#define TS_DEVICE_FILENAME "/dev/input/event0"
#endif /*TS_DEVICE_FILENAME*/

#ifndef KB_DEVICE_FILENAME
#define KB_DEVICE_FILENAME "/dev/input/event1"
#endif /*KB_DEVICE_FILENAME*/

#ifndef MICE_DEVICE_FILENAME
#define MICE_DEVICE_FILENAME "/dev/input/mouse0"
#endif /*MICE_DEVICE_FILENAME*/

static device_info_t s_devices_default[] = {{"fb", FB_DEVICE_FILENAME},
                                            {"drm", DRM_DEVICE_FILENAME},
                                            {"ts", TS_DEVICE_FILENAME},
                                            {"input", KB_DEVICE_FILENAME},
                                            {"mouse", MICE_DEVICE_FILENAME}};
static slist_t s_device_threads_list;

static ret_t main_loop_linux_destroy(main_loop_t* l) {
  main_loop_simple_t* loop = (main_loop_simple_t*)l;

  main_loop_simple_reset(loop);

#ifdef WITH_LINUX_EGL
#else
  native_window_raw_deinit();
#endif

  return RET_OK;
}

ret_t input_dispatch_to_main_loop(void* ctx, const event_queue_req_t* evt, const char* msg) {
  main_loop_simple_t* l = (main_loop_simple_t*)ctx;
  event_queue_req_t event = *evt;
  event_queue_req_t* e = &event;

  if (l != NULL && l->base.queue_event != NULL) {
    switch (e->event.type) {
      case EVT_KEY_DOWN:
      case EVT_KEY_UP:
      case EVT_KEY_LONG_PRESS: {
        e->event.size = sizeof(e->key_event);
        break;
      }
      case EVT_CONTEXT_MENU: {
        e->event.size = sizeof(e->pointer_event);
        break;
      }
      case EVT_POINTER_DOWN: {
        l->pressed = TRUE;
        e->pointer_event.pressed = l->pressed;
        e->event.size = sizeof(e->pointer_event);
        break;
      }
      case EVT_POINTER_MOVE: {
        e->pointer_event.pressed = l->pressed;
        e->event.size = sizeof(e->pointer_event);
        break;
      }
      case EVT_POINTER_UP: {
        e->pointer_event.pressed = l->pressed;
        l->pressed = FALSE;
        e->event.size = sizeof(e->pointer_event);
        break;
      }
      case EVT_WHEEL: {
        e->event.size = sizeof(e->wheel_event);
        break;
      }
      default:
        break;
    }

    main_loop_queue_event(&(l->base), e);
    input_dispatch_print(ctx, e, msg);
  } else {
    return RET_BAD_PARAMS;
  }
  return RET_OK;
}

static void on_app_exit(void) {
  slist_deinit(&s_device_threads_list);
  input_thread_global_deinit();
  devices_unload();
}

static ret_t lcd_create_on_devices_visit(void* ctx, const device_info_t* info) {
  LCD_T** p_lcd = (LCD_T**)ctx;

#ifdef WITH_LINUX_EGL
  if (tk_str_eq(info->type, "fb")) {
    *p_lcd = lcd_linux_egl_create(info->path);
  }
#elif WITH_LINUX_DRM
  if (tk_str_eq(info->type, "drm")) {
    *p_lcd = lcd_linux_drm_create(info->path);
  }
#else
  if (tk_str_eq(info->type, "fb")) {
    *p_lcd = lcd_linux_fb_create(info->path);
  }
#endif

  if (*p_lcd != NULL) {
    return RET_STOP;
  }

  return RET_OK;
}

static ret_t device_thread_run_on_devices_visit(void* ctx, const device_info_t* info) {
  main_loop_simple_t* loop = (main_loop_simple_t*)ctx;
  tk_thread_t* thread = NULL;
  ret_t ret = RET_OK;

  if (tk_str_eq(info->type, "input")) {
    thread = input_thread_run(info->path, input_dispatch_to_main_loop, loop, loop->w, loop->h);
  } else if (tk_str_eq(info->type, "mouse")) {
    thread = mouse_thread_run(info->path, input_dispatch_to_main_loop, loop, loop->w, loop->h);
  } else if (tk_str_eq(info->type, "ts")) {
#ifdef HAS_TSLIB
    thread = tslib_thread_run(info->path, input_dispatch_to_main_loop, loop, loop->w, loop->h);
#endif /*HAS_TSLIB*/
  }

  if (thread != NULL) {
    ret = slist_append(&s_device_threads_list, thread);
  }

  return ret;
}

main_loop_t* main_loop_init(int w, int h) {
  main_loop_simple_t* loop = NULL;
  LCD_T* lcd = NULL;

  if (RET_OK != devices_load()) {
    log_warn("Devices load fail! Used default.\r\n");
    devices_set(s_devices_default, ARRAY_SIZE(s_devices_default));
  }
  devices_foreach(lcd_create_on_devices_visit, &lcd);
  return_value_if_fail(lcd != NULL, NULL);

#ifdef WITH_LINUX_EGL
#else
  native_window_raw_init(lcd);
#endif

  loop = main_loop_simple_init(lcd->w, lcd->h, NULL, NULL);
  loop->base.destroy = main_loop_linux_destroy;

  input_thread_global_init();

  slist_init(&s_device_threads_list, (tk_destroy_t)tk_thread_destroy, NULL);
  devices_foreach(device_thread_run_on_devices_visit, loop);

  atexit(on_app_exit);

  return (main_loop_t*)loop;
}
