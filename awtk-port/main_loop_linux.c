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

#include "main_loop/main_loop_simple.h"
#include "main_loop_linux.h"
#include "base/window_manager.h"
#include "base/font_manager.h"
#include "lcd_linux_fb.h"
#include "base/idle.h"
#include "base/timer.h"

static ret_t main_loop_linux_destroy(main_loop_t* l) {
  main_loop_simple_t* loop = (main_loop_simple_t*)l;

  main_loop_simple_reset(loop);

  return RET_OK;
}

main_loop_t* main_loop_init(int w, int h) {
  main_loop_simple_t* loop = NULL;
  lcd_t* lcd = lcd_linux_fb_create("/dev/fb0");

  return_value_if_fail(lcd != NULL, NULL);
  loop = main_loop_simple_init(lcd->w, lcd->h);

  canvas_init(&(loop->base.canvas), lcd, font_manager());

  loop->base.destroy = main_loop_linux_destroy;

  /*TODO: start input device threads*/
  return (main_loop_t*)loop;
}
