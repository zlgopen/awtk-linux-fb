/**
 * File:   lcd_linux_fb.h
 * Author: AWTK Develop Team
 * Brief:  linux framebuffer lcd
 *
 * Copyright (c) 2018 - 2020  Guangzhou ZHIYUAN Electronics Co.,Ltd.
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

#include <signal.h>
#include "fb_info.h"
#include "tkc/mem.h"
#include "base/lcd.h"
#include "tkc/thread.h"
#include "awtk_global.h"
#include "tkc/time_now.h"
#include "tkc/semaphore.h"
#include "lcd_mem_others.h"
#include "blend/image_g2d.h"
#include "base/system_info.h"
#include "lcd/lcd_mem_bgr565.h"
#include "lcd/lcd_mem_rgb565.h"
#include "lcd/lcd_mem_bgra8888.h"
#include "lcd/lcd_mem_rgba8888.h"
#include "lcd/lcd_mem_bgr888.h"
#include "lcd/lcd_mem_rgb888.h"

#include "lcd_linux_fb.h"
#if !defined(WITH_LINUX_DRM) && !defined(WITH_LINUX_EGL)

#ifndef DISPLAY_WAIT_TIME
#define DISPLAY_WAIT_TIME 5000
#endif

static fb_info_t s_fb;
static int s_ttyfd = -1;
static int32_t s_buff_index = 0;
static bool_t s_app_quited = FALSE;
static const char* s_filename = NULL;
static void* s_fb_resize_func_ctx = NULL;
static lcd_linux_fb_resize_func_t s_fb_resize_func = NULL;

static bool_t lcd_linux_fb_open(fb_info_t* fb, const char* filename);

static void lcd_linux_fb_close(fb_info_t* fb) {
  if (s_ttyfd >= 0) {
    ioctl(s_ttyfd, KDSETMODE, KD_TEXT);
  }
  fb_close(fb);
}

static void on_app_exit(void) {
  fb_info_t* fb = &s_fb;
  s_app_quited = TRUE;
  lcd_linux_fb_close(fb);
  log_debug("on_app_exit\n");
}

static ret_t lcd_linux_init_drawing_fb(lcd_mem_t* mem, bitmap_t* fb) {
  return_value_if_fail(mem != NULL && fb != NULL, RET_BAD_PARAMS);

  memset(fb, 0x00, sizeof(bitmap_t));

  fb->w = mem->base.w;
  fb->h = mem->base.h;
  fb->format = mem->format;
  fb->buffer = mem->offline_gb;
  graphic_buffer_attach(mem->offline_gb, mem->offline_fb, fb->w, fb->h);
  bitmap_set_line_length(fb, mem->line_length);

  return RET_OK;
}

static ret_t lcd_linux_init_online_fb(lcd_mem_t* mem, bitmap_t* fb, uint8_t* buff, uint32_t w, uint32_t h, uint32_t line_length) {
  return_value_if_fail(mem != NULL && fb != NULL && buff != NULL, RET_BAD_PARAMS);

  memset(fb, 0x00, sizeof(bitmap_t));

  fb->w = w;
  fb->h = h;
  fb->format = mem->format;
  fb->buffer = mem->online_gb;
  graphic_buffer_attach(mem->online_gb, buff, w, h);
  bitmap_set_line_length(fb, line_length);

  return RET_OK;
}

static ret_t lcd_linux_flush(lcd_t* base) {
  uint8_t* buff = NULL;
  fb_info_t* fb = &s_fb;
  int fb_nr = fb_number(fb);
  uint32_t size = fb_size(fb);
  lcd_mem_t* lcd = (lcd_mem_t*)base;
  lcd_orientation_t o = system_info()->lcd_orientation;

  return_value_if_fail(lcd != NULL && fb != NULL && s_buff_index < fb_nr, RET_BAD_PARAMS);

  buff = fb->fbmem0 + size * s_buff_index;
  if (o == LCD_ORIENTATION_0) {
    bitmap_t online_fb;
    bitmap_t offline_fb;
    rect_t r = {0, 0, fb_width(fb), fb_height(fb)};

    lcd_linux_init_drawing_fb(lcd, &offline_fb);
    lcd_linux_init_online_fb(lcd, &online_fb, buff, fb_width(fb), fb_height(fb), fb_line_length(fb));

    image_copy(&online_fb, &offline_fb, &r, r.x, r.y);
  } else {
    rect_t r = {0};
    bitmap_t online_fb;
    bitmap_t offline_fb;
    if (o == LCD_ORIENTATION_180) {
      r.x = 0;
      r.y = 0;
      r.w = fb_width(fb);
      r.h = fb_height(fb);
    } else {
      r.x = 0;
      r.y = 0;
      r.w = fb_height(fb);
      r.h = fb_width(fb);
    }

    lcd_linux_init_drawing_fb(lcd, &offline_fb);
    lcd_linux_init_online_fb(lcd, &online_fb, buff, fb_width(fb), fb_height(fb), fb_line_length(fb));

    image_rotate(&online_fb, &offline_fb, &r, o);
  }
  return RET_OK;
}

static void on_signal_int(int sig) {
  tk_quit();
}

static ret_t (*lcd_mem_linux_flush_defalut)(lcd_t* lcd);
static ret_t lcd_mem_linux_flush(lcd_t* lcd) {
  fb_info_t* fb = (fb_info_t*)(lcd->impl_data);
  fb_sync(fb);

  if (lcd_mem_linux_flush_defalut) {
    lcd_mem_linux_flush_defalut(lcd);
  }
  return RET_OK;
}

static ret_t (*lcd_mem_linux_resize_defalut)(lcd_t* lcd, wh_t w, wh_t h, uint32_t line_length);
static ret_t lcd_mem_linux_resize(lcd_t* lcd, wh_t w, wh_t h, uint32_t line_length) {
  ret_t ret = RET_OK;
  fb_info_t* fb = &s_fb;
  bool_t is_1fb = fb_is_1fb(fb);
  uint32_t fb_num = fb_number(fb);
  lcd_mem_t* mem = (lcd_mem_t*)lcd;
  return_value_if_fail(lcd != NULL && s_fb_resize_func != NULL, RET_BAD_PARAMS)
  
  if (lcd_mem_linux_resize_defalut != NULL) {
    lcd_mem_linux_resize_defalut(lcd, w, h, line_length);
  }

  lcd_linux_fb_close(fb);

  ret = s_fb_resize_func(fb_num, w, h, s_fb_resize_func_ctx);
  return_value_if_fail(ret == RET_OK, ret);
  return_value_if_fail(lcd_linux_fb_open(fb, s_filename), RET_FAIL);

  if (fb_is_1fb(fb)) {
    mem->online_fb = (uint8_t*)(fb->fbmem0);
  } else {
    s_buff_index = 0;
    TKMEM_FREE(mem->offline_fb);
  }
  mem->offline_fb = (uint8_t*)TKMEM_ALLOC(fb_size(fb));
  lcd_mem_set_line_length(lcd, fb_line_length(fb));
  if (fb_is_1fb(fb)) {
    lcd->impl_data = fb;
    fb->fbmem1 = mem->offline_fb;
  }

  log_debug("lcd_linux_fb_resize \r\n");
  return ret;
}

static lcd_t* lcd_linux_create_flushable(fb_info_t* fb) {
  lcd_t* lcd = NULL;
  int w = fb_width(fb);
  int h = fb_height(fb);
  int line_length = fb_line_length(fb);

  int bpp = fb_bpp(fb);
  int size = fb_size(fb);
  uint8_t* online_fb = (uint8_t*)(fb->fbmem0);

  fb->fbmem1 = (uint8_t*)malloc(size);
  return_value_if_fail(fb->fbmem1 != NULL, NULL);

  if (bpp == 16) {
    if (fb_is_bgra5551(fb)) {
      lcd = lcd_mem_bgra5551_create(fb);
    } else if (fb_is_bgr565(fb)) {
      lcd = lcd_mem_bgr565_create_double_fb(w, h, online_fb, fb->fbmem1);
    } else if (fb_is_rgb565(fb)) {
      lcd = lcd_mem_rgb565_create_double_fb(w, h, online_fb, fb->fbmem1);
    } else {
      assert(!"not supported framebuffer format.");
    }
  } else if (bpp == 32) {
    if (fb_is_bgra8888(fb)) {
      lcd = lcd_mem_bgra8888_create_double_fb(w, h, online_fb, fb->fbmem1);
    } else if (fb_is_rgba8888(fb)) {
      lcd = lcd_mem_rgba8888_create_double_fb(w, h, online_fb, fb->fbmem1);
    } else {
      assert(!"not supported framebuffer format.");
    }
  } else if (bpp == 24) {
    if (fb_is_bgr888(fb)) {
      lcd = lcd_mem_bgr888_create_double_fb(w, h, online_fb, fb->fbmem1);
    } else if (fb_is_rgb888(fb)) {
      lcd = lcd_mem_rgb888_create_double_fb(w, h, online_fb, fb->fbmem1);
    } else {
      assert(!"not supported framebuffer format.");
    }
  } else {
    assert(!"not supported framebuffer format.");
  }

  if (lcd != NULL) {
    lcd->impl_data = fb;
    lcd_mem_linux_flush_defalut = lcd->flush;
    lcd->flush = lcd_mem_linux_flush;
    lcd_mem_linux_resize_defalut = lcd->resize;
    lcd->resize = lcd_mem_linux_resize;
    lcd_mem_set_line_length(lcd, line_length);
  }

  return lcd;
}

static ret_t lcd_mem_linux_wirte_buff(lcd_t* lcd) {
  ret_t ret = RET_OK;
  fb_info_t* fb = &s_fb;
  int fb_nr = fb_number(fb);
  struct fb_var_screeninfo vi = (fb->var);

  if (lcd->draw_mode != LCD_DRAW_OFFLINE) {
    int dummy = 0;
    ioctl(fb->fd, FBIO_WAITFORVSYNC, &dummy);

    s_buff_index++;
    if (s_buff_index >= fb_nr) {
      s_buff_index = 0;
    }
    ret = lcd_linux_flush(lcd);

    vi.yoffset = s_buff_index * fb_height(fb);
    ioctl(fb->fd, FBIOPAN_DISPLAY, &vi);
  }

  return ret;
}

static lcd_t* lcd_linux_create_swappable(fb_info_t* fb) {
  lcd_t* lcd = NULL;
  int w = fb_width(fb);
  int h = fb_height(fb);
  int bpp = fb_bpp(fb);
  int line_length = fb_line_length(fb);
  uint8_t* buff = (uint8_t*)TKMEM_ALLOC(fb_size(fb));

  if (bpp == 16) {
    if (fb_is_bgr565(fb)) {
      lcd = lcd_mem_bgr565_create_single_fb(w, h, buff);
    } else if (fb_is_rgb565(fb)) {
      lcd = lcd_mem_rgb565_create_single_fb(w, h, buff);
    } else {
      assert(!"not supported framebuffer format.");
    }
  } else if (bpp == 32) {
    if (fb_is_bgra8888(fb)) {
      lcd = lcd_mem_bgra8888_create_single_fb(w, h, buff);
    } else if (fb_is_rgba8888(fb)) {
      lcd = lcd_mem_rgba8888_create_single_fb(w, h, buff);
    } else {
      assert(!"not supported framebuffer format.");
    }
  } else if (bpp == 24) {
    if (fb_is_bgr888(fb)) {
      lcd = lcd_mem_bgr888_create_single_fb(w, h, buff);
    } else if (fb_is_rgb888(fb)) {
      lcd = lcd_mem_rgb888_create_single_fb(w, h, buff);
    } else {
      assert(!"not supported framebuffer format.");
    }
  } else {
    assert(!"not supported framebuffer format.");
  }

  if (lcd != NULL) {

    lcd->swap = lcd_mem_linux_wirte_buff;
    lcd->flush = lcd_mem_linux_wirte_buff;
    lcd_mem_linux_resize_defalut = lcd->resize;
    lcd->resize = lcd_mem_linux_resize;
    ((lcd_mem_t*)lcd)->own_offline_fb = TRUE;
    lcd_mem_set_line_length(lcd, line_length);
  }

  return lcd;
}

static lcd_t* lcd_linux_create(fb_info_t* fb) {
  if (fb_is_1fb(fb)) {
    return lcd_linux_create_flushable(fb);
  } else {
    return lcd_linux_create_swappable(fb);
  }
}

ret_t lcd_linux_set_fb_resize_func(lcd_linux_fb_resize_func_t fb_resize_func, void* ctx) {
  return_value_if_fail(fb_resize_func != NULL, RET_OK);
  s_fb_resize_func = fb_resize_func;
  s_fb_resize_func_ctx = ctx;
  return RET_OK;
}

static bool_t lcd_linux_fb_open(fb_info_t* fb, const char* filename) {
  if (fb_open(fb, filename) == 0) {
    s_filename = filename;
    s_ttyfd = open("/dev/tty1", O_RDWR);
    if (s_ttyfd >= 0) {
      ioctl(s_ttyfd, KDSETMODE, KD_GRAPHICS);
    }
    return TRUE;
  }
  return FALSE;
}

static ret_t lcd_linux_fb_resize_func(uint32_t fb_num, wh_t w, wh_t h, void* ctx) {
  struct fb_var_screeninfo var_set;
  int fh = open(s_filename, O_RDONLY);
  return_value_if_fail(fh >=0, RET_FAIL);

	ioctl(fh, FBIOGET_VSCREENINFO, &var_set);

  var_set.xres = w;
  var_set.yres = h;
  var_set.xres_virtual = w;
  var_set.yres_virtual = h * fb_num;
  ioctl(fh, FBIOPUT_VSCREENINFO, &var_set);
  close(fh);

  return RET_OK;
}

lcd_t* lcd_linux_fb_create(const char* filename) {
  lcd_t* lcd = NULL;
  fb_info_t* fb = &s_fb;
  return_value_if_fail(filename != NULL, NULL);

  if (lcd_linux_fb_open(fb, filename)) {
    lcd_linux_set_fb_resize_func(lcd_linux_fb_resize_func, NULL);
    // fix FBIOPAN_DISPLAY block issue when run in vmware double fb mode
    if (check_if_run_in_vmware()) {
      log_info("run in vmware and fix FBIOPAN_DISPLAY block issue\n");
      // if memset/memcpy the entire fb then call FBIOPAN_DISPLAY immediately, 
      // the ubuntu in vmware will stuck by unknown reason, sleep for avoid this bug
      fb->var.activate = FB_ACTIVATE_INV_MODE;
      fb->var.pixclock = 60;
      usleep(500000);
    }

    lcd = lcd_linux_create(fb);
  }

  atexit(on_app_exit);
  signal(SIGINT, on_signal_int);

  return lcd;
}

#endif
