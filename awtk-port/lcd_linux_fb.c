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

#ifndef WITH_LINUX_DRM

#ifndef DISPLAY_WAIT_TIME
#define DISPLAY_WAIT_TIME 5000
#endif

static fb_info_t s_fb;
static int s_ttyfd = -1;
static int32_t s_buff_index = 0;
static bool_t s_app_quited = FALSE;
static tk_thread_t* s_t_display = NULL;
static tk_semaphore_t* s_read_sema = NULL;
static tk_semaphore_t* s_wirte_sema = NULL;

static void on_app_exit(void) {
  fb_info_t* fb = &s_fb;

  s_app_quited = TRUE;
  if (s_read_sema != NULL) {
    tk_semaphore_post(s_read_sema);
    sleep_ms(16);
  }
  if (s_ttyfd >= 0) {
    ioctl(s_ttyfd, KDSETMODE, KD_TEXT);
  }

  log_info("wait for display thread quited \r\n");

  if (s_t_display != NULL) {
    tk_thread_join(s_t_display);
    tk_thread_destroy(s_t_display);
  }

  if (s_read_sema != NULL) {
    tk_semaphore_destroy(s_read_sema);
  }

  if (s_wirte_sema != NULL) {
    tk_semaphore_destroy(s_wirte_sema);
  }
  
  fb_close(fb);

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
    ret_t ret = RET_FAIL;
#ifdef WITH_G2D
    bitmap_t online_fb;
    bitmap_t offline_fb;
    rect_t r = {0, 0, fb_width(fb), fb_height(fb)};

    lcd_linux_init_drawing_fb(lcd, &offline_fb);
    lcd_linux_init_online_fb(lcd, &online_fb, buff, fb_width(fb), fb_height(fb), fb_line_length(fb));

    ret = image_copy(&online_fb, &offline_fb, &r, r.x, r.y);
#endif /*WITH_G2D*/
    if (ret != RET_OK) {
      memcpy(buff, lcd->offline_fb, size);
    }
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

static void* display_thread(void* ctx) {
  fb_info_t* fb = &s_fb;
  int fb_nr = fb_number(fb);
  struct fb_var_screeninfo vi = (fb->var);

  log_info("display_thread start\n");

  while (!s_app_quited) {

    if (s_buff_index < fb_nr) {
      vi.yoffset = s_buff_index * fb_height(fb);
      if (tk_semaphore_wait(s_read_sema, DISPLAY_WAIT_TIME) == RET_OK) {
        if (s_app_quited) {
          break;
        }
        if (ioctl(fb->fd, FBIOPUT_VSCREENINFO, &vi) < 0) {
          perror("active fb swap failed");
        }
        s_buff_index++;
        if (s_buff_index >= fb_nr) {
          s_buff_index = 0;
        }
        tk_semaphore_post(s_wirte_sema);
      }
      
    }
  }

  log_info("display_thread end\n");

  return NULL;
}

static void on_signal_int(int sig) {
  tk_quit();
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
    assert(!"not supported framebuffer format.");
  } else {
    assert(!"not supported framebuffer format.");
  }

  if (lcd != NULL) {
    lcd->impl_data = fb;
    lcd->sync = lcd_mem_linux_sync;
    lcd_mem_set_line_length(lcd, line_length);
  }

  return lcd;
}

static ret_t lcd_mem_linux_wirte_buff(lcd_t* lcd) {
  ret_t ret = RET_OK;
  if (lcd->draw_mode != LCD_DRAW_OFFLINE) {
    if (tk_semaphore_wait(s_wirte_sema, DISPLAY_WAIT_TIME) == RET_OK) {
      ret = lcd_linux_flush(lcd);
      tk_semaphore_post(s_read_sema);

      return_value_if_fail(ret == RET_OK, ret);
    }
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
    assert(!"not supported framebuffer format.");
  } else {
    assert(!"not supported framebuffer format.");
  }

  if (lcd != NULL) {

    lcd->swap = lcd_mem_linux_wirte_buff;
    lcd->flush = lcd_mem_linux_wirte_buff;
    ((lcd_mem_t*)lcd)->own_offline_fb = TRUE;
    lcd_mem_set_line_length(lcd, line_length);

    s_read_sema = tk_semaphore_create(0, NULL);
    s_wirte_sema = tk_semaphore_create(1, NULL);

    s_t_display = tk_thread_create(display_thread, lcd);
    tk_thread_start(s_t_display);
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

lcd_t* lcd_linux_fb_create(const char* filename) {
  lcd_t* lcd = NULL;
  fb_info_t* fb = &s_fb;
  return_value_if_fail(filename != NULL, NULL);

  if (fb_open(fb, filename) == 0) {
    s_ttyfd = open("/dev/tty1", O_RDWR);
    if (s_ttyfd >= 0) {
      ioctl(s_ttyfd, KDSETMODE, KD_GRAPHICS);
    }

    // fix FBIOPUT_VSCREENINFO block issue when run in vmware double fb mode
    if (check_if_run_in_vmware()) {
      log_info("run in vmware and fix FBIOPUT_VSCREENINFO block issue\n");
      fb->var.activate = FB_ACTIVATE_INV_MODE;
      fb->var.pixclock = 60;
    }

    lcd = lcd_linux_create(fb);
  }

  atexit(on_app_exit);
  signal(SIGINT, on_signal_int);

  return lcd;
}

#endif /*no WITH_LINUX_DRM*/
