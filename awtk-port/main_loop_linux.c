/**
 * File:   main_loop_linux.c
 * Author: AWTK Develop Team
 * Brief:  linux implemented main_loop interface
 *
 * Copyright (c) 2018 - 2018  Guangzhou ZHIYUAN Electronics Co.,Ltd.
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

#include "tslib_thread.h"
#include "input_thread.h"
#include "mouse_thread.h"
#include "lcd_linux_fb.h"
#include "main_loop_linux.h"

#define FB_DEVICE_FILENAME "/dev/fb0"
#define TS_DEVICE_FILENAME "/dev/input/event0"
#define KB_DEVICE_FILENAME "/dev/input/event1"
#define MICE_DEVICE_FILENAME "/dev/input/mice"

static ret_t main_loop_linux_destroy(main_loop_t* l) {
  main_loop_simple_t* loop = (main_loop_simple_t*)l;

  main_loop_simple_reset(loop);

  return RET_OK;
}

ret_t input_dispatch_to_main_loop(void* ctx, const event_queue_req_t* e) {
  main_loop_queue_event((main_loop_t*)ctx, e);
  input_dispatch_print(ctx, e);

  return RET_OK;
}

main_loop_t* main_loop_init(int w, int h) {
  main_loop_simple_t* loop = NULL;
  lcd_t* lcd = lcd_linux_fb_create(FB_DEVICE_FILENAME);

  return_value_if_fail(lcd != NULL, NULL);
  loop = main_loop_simple_init(lcd->w, lcd->h);

  loop->base.destroy = main_loop_linux_destroy;
  canvas_init(&(loop->base.canvas), lcd, font_manager());

#ifdef HAS_TSLIB
  tslib_thread_run(TS_DEVICE_FILENAME, input_dispatch_to_main_loop, loop, lcd->w, lcd->h);
#endif/*HAS_TSLIB*/  

  input_thread_run(KB_DEVICE_FILENAME, input_dispatch_to_main_loop, loop, lcd->w, lcd->h);
  mouse_thread_run(MICE_DEVICE_FILENAME, input_dispatch_to_main_loop, loop, lcd->w, lcd->h);

  return (main_loop_t*)loop;
}
