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
#include <pthread.h>
#include "fb_info.h"
#include "tkc/mem.h"
#include "base/lcd.h"
#include "tkc/time_now.h"
#include "awtk_global.h"
#include "lcd_mem_others.h"
#include "lcd/lcd_mem_bgr565.h"
#include "lcd/lcd_mem_rgb565.h"
#include "lcd/lcd_mem_bgra8888.h"
#include "lcd/lcd_mem_rgba8888.h"

static fb_info_t s_fb;
static int s_ttyfd = -1;
static bool_t s_app_quited = FALSE;
static pthread_mutex_t s_mutex = PTHREAD_MUTEX_INITIALIZER;

static void on_app_exit(void) {
  fb_info_t* fb = &s_fb;

  s_app_quited = TRUE;
  if (s_ttyfd >= 0) {
    ioctl(s_ttyfd, KDSETMODE, KD_TEXT);
  }

  fb_close(fb);

  log_debug("on_app_exit\n");
}

static void* display_thread(void* ctx) {
  uint32_t i = 0;
  uint32_t index = 0;
  fb_info_t* fb = &s_fb;
  int fb_nr = fb_number(fb);
  uint32_t size = fb_size(fb);
  lcd_mem_t* lcd = (lcd_mem_t*)ctx;
  struct fb_var_screeninfo vi = (fb->var);

  log_info("display_thread start\n");
  while (!s_app_quited) {
    uint8_t* buff = fb->fbmem0 + size * i;
    uint32_t start = time_now_ms();

    vi.yoffset = i * fb_height(fb);
    pthread_mutex_lock(&s_mutex);
    memcpy(buff, lcd->offline_fb, size);
    pthread_mutex_unlock(&s_mutex);

    if (ioctl(fb->fd, FBIOPUT_VSCREENINFO, &vi) < 0) {
      perror("active fb swap failed");
    }

#if 0
    log_info("index=%u: i=%u cost=%u yoffset=%u buff=%p size=%u\n", index, i, 
        (time_now_ms() - start), vi.yoffset,
        buff, size);
#endif
    index++;
    i = index % fb_nr;
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
  int line_length = fb->fix.line_length;

  int size = fb_size(fb);
  int bpp = fb->var.bits_per_pixel;
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

static ret_t lcd_mem_linux_lock(lcd_t* lcd, rect_t* dirty_rect) {
  if (lcd->draw_mode != LCD_DRAW_OFFLINE) {
    pthread_mutex_lock(&s_mutex);
  }

  return RET_OK;
}

static ret_t lcd_mem_linux_unlock(lcd_t* lcd) {
  if (lcd->draw_mode != LCD_DRAW_OFFLINE) {
    pthread_mutex_unlock(&s_mutex);
  }

  return RET_OK;
}

static lcd_t* lcd_linux_create_swappable(fb_info_t* fb) {
  lcd_t* lcd = NULL;
  int w = fb_width(fb);
  int h = fb_height(fb);
  int line_length = fb->fix.line_length;
  int bpp = fb->var.bits_per_pixel;

  if (bpp == 16) {
    if (fb_is_bgr565(fb)) {
      lcd = lcd_mem_bgr565_create(w, h, TRUE);
    } else if (fb_is_rgb565(fb)) {
      lcd = lcd_mem_rgb565_create(w, h, TRUE);
    } else {
      assert(!"not supported framebuffer format.");
    }
  } else if (bpp == 32) {
    if (fb_is_bgra8888(fb)) {
      lcd = lcd_mem_bgra8888_create(w, h, TRUE);
    } else if (fb_is_rgba8888(fb)) {
      lcd = lcd_mem_rgba8888_create(w, h, TRUE);
    } else {
      assert(!"not supported framebuffer format.");
    }
  } else if (bpp == 24) {
    assert(!"not supported framebuffer format.");
  } else {
    assert(!"not supported framebuffer format.");
  }

  if (lcd != NULL) {
    pthread_t tid;
    pthread_create(&tid, NULL, display_thread, lcd);
    lcd_mem_set_line_length(lcd, line_length);
    lcd->swap = lcd_mem_linux_unlock;
    lcd->begin_frame = lcd_mem_linux_lock;
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

    lcd = lcd_linux_create(fb);
  }

  atexit(on_app_exit);
  signal(SIGINT, on_signal_int);

  return lcd;
}
