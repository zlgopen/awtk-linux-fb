/**
 * File:   lcd_linux_fb.h
 * Author: AWTK Develop Team
 * Brief:  linux framebuffer lcd
 *
 * Copyright (c) 2018 - 2018  Guangzhou ZHIYUAN Electronics Co.,Ltd.
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
 * 2018-09-07 Li XianJing <xianjimli@hotmail.com> created
 *
 */

#include "fb_info.h"
#include "tkc/mem.h"
#include "base/lcd.h"
#include "lcd_mem_others.h"
#include "lcd/lcd_mem_bgr565.h"
#include "lcd/lcd_mem_bgra8888.h"
#include "lcd/lcd_mem_rgba8888.h"

static fb_info_t s_fb;
static lcd_flush_t s_lcd_flush_default = NULL;

static ret_t lcd_linux_fb_flush(lcd_t* lcd) {
  if (s_lcd_flush_default != NULL) {
    s_lcd_flush_default(lcd);
  }

  fb_sync(&s_fb);

  return RET_OK;
}

static void* s_offline_fb = NULL;

static void on_app_exit(void) {
  fb_info_t* fb = &s_fb;
  free(s_offline_fb);
  fb_close(fb);
}

static lcd_t* lcd_linux_create(fb_info_t* fb) {
  lcd_t* lcd = NULL;
  int w = fb_width(fb);
  int h = fb_height(fb);
  int line_length = fb->fix.line_length;

  int size = fb_size(fb);
  int bpp = fb->var.bits_per_pixel;
  uint8_t* online_fb = (uint8_t*)(fb->bits);
  struct fb_var_screeninfo* var = &(fb->var);

  if (bpp == 16) {
    if (var->transp.length == 1 && var->red.length == 5 && var->green.length == 5 &&
        var->blue.length == 5 && var->blue.offset == 0) {
      lcd = lcd_mem_bgra5551_create(fb);
    } else {
      s_offline_fb = (uint8_t*)malloc(size);
      lcd = lcd_mem_bgr565_create_double_fb(w, h, online_fb, s_offline_fb);
    }
  } else if (bpp == 32) {
    if (fb->var.blue.offset == 0) {
      s_offline_fb = (uint8_t*)malloc(size);
      lcd = lcd_mem_bgra8888_create_double_fb(w, h, online_fb, s_offline_fb);
    } else {
      s_offline_fb = (uint8_t*)malloc(size);
      lcd = lcd_mem_rgba8888_create_double_fb(w, h, online_fb, s_offline_fb);
    }
  } else if (bpp == 24) {
    assert(!"not supported framebuffer format.");
  } else {
    assert(!"not supported framebuffer format.");
  }

  lcd_mem_set_line_length(lcd, line_length);

  return lcd;
}

lcd_t* lcd_linux_fb_create(const char* filename) {
  lcd_t* lcd = NULL;
  fb_info_t* fb = &s_fb;
  return_value_if_fail(filename != NULL, NULL);

  if (fb_open(fb, filename) == 0) {
    lcd = lcd_linux_create(fb);
  }

  if (lcd != NULL) {
    s_lcd_flush_default = lcd->flush;
    lcd->flush = lcd_linux_fb_flush;
  }

  atexit(on_app_exit);

  return lcd;
}
