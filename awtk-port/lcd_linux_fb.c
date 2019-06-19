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
#include "lcd/lcd_mem_rgb565.h"
#include "lcd/lcd_mem_bgra8888.h"
#include "lcd/lcd_mem_rgba8888.h"

static fb_info_t s_fb;

static void on_app_exit(void) {
  fb_info_t* fb = &s_fb;

  fb_close(fb);
}

static ret_t lcd_mem_linux_sync(lcd_t* lcd) {
  fb_info_t* fb = (fb_info_t*)(lcd->impl_data);

  fb_sync(fb);

  return RET_OK;
}

static lcd_t* lcd_linux_create_flushable(fb_info_t* fb) {
  lcd_t* lcd = NULL;
  int w = fb_width(fb);
  int h = fb_height(fb);
  int line_length = fb->fix.line_length;

  int size = fb_size(fb);
  int bpp = fb->var.bits_per_pixel;
  uint8_t* online_fb = (uint8_t*)(fb->fbmem0);

  fb->fbmem1 = (uint8_t*)malloc(size);
  return_value_if_fail(fb->fbmem1 != NULL, NULL);

  if (bpp == 16) {
    if (fb_is_bgra5551(fb)) {
      lcd = lcd_mem_bgra5551_create(fb);
    } else if(fb_is_bgr565(fb)) {
      lcd = lcd_mem_bgr565_create_double_fb(w, h, online_fb, fb->fbmem1);
    } else if(fb_is_rgb565(fb)) {
      lcd = lcd_mem_rgb565_create_double_fb(w, h, online_fb, fb->fbmem1);
    } else {
      assert(!"not supported framebuffer format.");
    }
  } else if (bpp == 32) {
    if (fb_is_bgra8888(fb)) {
      lcd = lcd_mem_bgra8888_create_double_fb(w, h, online_fb, fb->fbmem1);
    } else if(fb_is_rgba8888(fb)) {
      lcd = lcd_mem_rgba8888_create_double_fb(w, h, online_fb, fb->fbmem1);
    } else {
      assert(!"not supported framebuffer format.");
    }
  } else if (bpp == 24) {
    assert(!"not supported framebuffer format.");
  } else {
    assert(!"not supported framebuffer format.");
  }

  if(lcd != NULL) {
    lcd->impl_data = fb;
    lcd->sync = lcd_mem_linux_sync;
    lcd_mem_set_line_length(lcd, line_length);
  }

  return lcd;
}

static ret_t lcd_mem_linux_begin_frame(lcd_t* lcd, rect_t* dirty_rect) {
  lcd_mem_t* lcd_mem = (lcd_mem_t*)(lcd);
  fb_info_t* fb = (fb_info_t*)(lcd->impl_data);
  struct fb_var_screeninfo* var = &(fb->var);

  if (lcd->draw_mode == LCD_DRAW_OFFLINE) {
    lcd_mem->offline_fb = fb->offline_fb;
  } else {
    if(var->yoffset == 0) {
      lcd_mem->offline_fb = fb->fbmem0;
    } else {
      lcd_mem->offline_fb = fb->fbmem1;
    }
  }

  return RET_OK;
}

static ret_t lcd_mem_linux_swap(lcd_t* lcd) {
  int ret = 0;
  fb_info_t* fb = (fb_info_t*)(lcd->impl_data);
  struct fb_var_screeninfo* var = &(fb->var);

	var->xoffset = 0;
  if(var->yoffset == 0) {
	  var->yoffset = var->yres;
  } else {
	  var->yoffset = 0;
  }

	ret = ioctl(fb->fd, FBIOPAN_DISPLAY, &(fb->var));
	printf("FBIOPAN_DISPLAY ret=%d yoffset=%d\n", ret, var->yoffset);

	return RET_OK;
}

static lcd_t* lcd_linux_create_swappable(fb_info_t* fb) {
  lcd_t* lcd = NULL;
  int w = fb_width(fb);
  int h = fb_height(fb);
  int line_length = fb->fix.line_length;
  int bpp = fb->var.bits_per_pixel;
  uint8_t* fbmem = (uint8_t*)(fb->fbmem0);

  fb->offline_fb = (uint8_t*)malloc(fb_size(fb));
  return_value_if_fail(fb->offline_fb != NULL, NULL);

  if (bpp == 16) {
    if(fb_is_bgr565(fb)) {
      lcd = lcd_mem_bgr565_create_single_fb(w, h, fbmem);
    } else if(fb_is_rgb565(fb)) {
      lcd = lcd_mem_rgb565_create_single_fb(w, h, fbmem);
    } else {
      assert(!"not supported framebuffer format.");
    }
  } else if (bpp == 32) {
    if (fb_is_bgra8888(fb)) {
      lcd = lcd_mem_bgra8888_create_single_fb(w, h, fbmem);
    } else if(fb_is_rgba8888(fb)) {
      lcd = lcd_mem_rgba8888_create_single_fb(w, h, fbmem);
    } else {
      assert(!"not supported framebuffer format.");
    }
  } else if (bpp == 24) {
    assert(!"not supported framebuffer format.");
  } else {
    assert(!"not supported framebuffer format.");
  }
  
  if(lcd != NULL) {
    lcd->impl_data = fb;
    lcd->swap = lcd_mem_linux_swap;
    lcd->begin_frame = lcd_mem_linux_begin_frame;
    lcd_mem_set_line_length(lcd, line_length);
  }

  return NULL;
}

static lcd_t* lcd_linux_create(fb_info_t* fb) {
  if(fb_is_1fb(fb)) {
    return lcd_linux_create_flushable(fb);
  } else if(fb_is_2fb(fb)) {
    return lcd_linux_create_swappable(fb);
  } else if(fb_is_3fb(fb)) {
    return NULL;
  } else {
    return NULL;
  }
}

lcd_t* lcd_linux_fb_create(const char* filename) {
  lcd_t* lcd = NULL;
  fb_info_t* fb = &s_fb;
  return_value_if_fail(filename != NULL, NULL);

  if (fb_open(fb, filename) == 0) {
    lcd = lcd_linux_create(fb);
  }

  atexit(on_app_exit);

  return lcd;
}
