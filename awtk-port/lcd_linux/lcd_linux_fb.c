/**
 * File:   lcd_linux_fb.h
 * Author: AWTK Develop Team
 * Brief:  linux framebuffer lcd
 *
 * Copyright (c) 2018 - 2025 Guangzhou ZHIYUAN Electronics Co.,Ltd.
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

#ifdef WITH_LINUX_FB

#include <signal.h>
#include "fb_info.h"
#include "tkc/mem.h"
#include "base/lcd.h"
#include "tkc/thread.h"
#include "awtk_global.h"
#include "tkc/time_now.h"
#include "tkc/mutex.h"
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
#include "base/lcd_orientation_helper.h"

#define __FB_SUP_RESIZE    1
#define __FB_WAIT_VSYNC    1

static fb_info_t s_fb;
static int s_ttyfd = -1;

static bool_t lcd_linux_fb_open(fb_info_t* fb, const char* filename) {
  if (fb_open(fb, filename) == 0) {
    s_ttyfd = open("/dev/tty1", O_RDWR);
    if (s_ttyfd >= 0) {
      ioctl(s_ttyfd, KDSETMODE, KD_GRAPHICS);
    }

    // fix FBIOPAN_DISPLAY block issue when run in vmware double fb mode
    if (check_if_run_in_vmware()) {
      log_info("run in vmware and fix FBIOPAN_DISPLAY block issue\n");
      // if memset/memcpy the entire fb then call FBIOPAN_DISPLAY immediately, 
      // the ubuntu in vmware will stuck by unknown reason, sleep for avoid this bug
      fb->var.activate = FB_ACTIVATE_INV_MODE;
      fb->var.pixclock = 60;
      usleep(500000);
    }
    return TRUE;
  }
  return FALSE;
}

static void lcd_linux_fb_close(fb_info_t* fb) {
  if (s_ttyfd >= 0) {
    ioctl(s_ttyfd, KDSETMODE, KD_TEXT);
  }
  fb_close(fb);
}

static void on_app_exit(void) {
  fb_info_t* fb = &s_fb;
  lcd_linux_fb_close(fb);
  log_debug("on_app_exit\n");
}

#if __FB_SUP_RESIZE
static ret_t (*lcd_mem_linux_resize_default)(lcd_t* lcd, wh_t w, wh_t h, uint32_t line_length);
static ret_t lcd_mem_linux_resize(lcd_t* lcd, wh_t w, wh_t h, uint32_t line_length) {
  ret_t ret = RET_OK;
  fb_info_t* fb = &s_fb;
  return_value_if_fail(lcd != NULL, RET_BAD_PARAMS);

  ret = fb_resize_reopen(fb, w, h);
  lcd_mem_set_double_fb_bitmap(lcd, fb->online_fb, fb->offline_fb);
  lcd_mem_set_line_length(lcd, fb_line_length(fb));

  if (lcd_mem_linux_resize_default && ret == RET_OK) {
    lcd_mem_linux_resize_default(lcd, w, h, line_length);
  }

  log_debug("lcd_linux_fb_resize \r\n");
  return ret;
}
#endif

static void on_signal_int(int sig) {
  tk_quit();
}

static ret_t (*lcd_mem_linux_flush_default)(lcd_t* lcd);
static ret_t lcd_mem_linux_flush(lcd_t* lcd) {
#if __FB_WAIT_VSYNC
  fb_info_t* fb = &s_fb;
  if (!fb_is_1fb(fb)) {
    struct fb_var_screeninfo vi = (fb->var);
    vi.yoffset = 0;
    ioctl(fb->fd, FBIOPAN_DISPLAY, &vi);
  }
  int dummy = 0;
  ioctl(fb->fd, FBIO_WAITFORVSYNC, &dummy);
#endif

  if (lcd_mem_linux_flush_default) {
    lcd_mem_linux_flush_default(lcd);
  }
  return RET_OK;
}

static lcd_t* lcd_linux_create_flushable(fb_info_t* fb) {
  lcd_t* lcd = NULL;
  int bpp = fb_bpp(fb);
  int line_length = fb_line_length(fb);
  bitmap_t* online_fb = fb->online_fb;
  bitmap_t* offline_fb = fb->offline_fb;
  return_value_if_fail(offline_fb != NULL, NULL);

  if (bpp == 16) {
    if (fb_is_bgra5551(fb)) {
      lcd = lcd_mem_bgra5551_create(fb);
    } else if (fb_is_bgr565(fb)) {
      lcd = lcd_mem_bgr565_create_double_fb_bitmap(online_fb, offline_fb);
    } else if (fb_is_rgb565(fb)) {
      lcd = lcd_mem_rgb565_create_double_fb_bitmap(online_fb, offline_fb);
    } else {
      assert(!"not supported framebuffer format.");
    }
  } else if (bpp == 32) {
    if (fb_is_bgra8888(fb)) {
      lcd = lcd_mem_bgra8888_create_double_fb_bitmap(online_fb, offline_fb);
    } else if (fb_is_rgba8888(fb)) {
      lcd = lcd_mem_rgba8888_create_double_fb_bitmap(online_fb, offline_fb);
    } else {
      assert(!"not supported framebuffer format.");
    }
  } else if (bpp == 24) {
    if (fb_is_bgr888(fb)) {
      lcd = lcd_mem_bgr888_create_double_fb_bitmap(online_fb, offline_fb);
    } else if (fb_is_rgb888(fb)) {
      lcd = lcd_mem_rgb888_create_double_fb_bitmap(online_fb, offline_fb);
    } else {
      assert(!"not supported framebuffer format.");
    }
  } else {
    assert(!"not supported framebuffer format.");
  }

  if (lcd != NULL) {
    lcd_mem_linux_flush_default = lcd->flush;
    lcd->flush = lcd_mem_linux_flush;
    lcd_mem_set_line_length(lcd, line_length);

#if __FB_SUP_RESIZE
    lcd_mem_linux_resize_default = lcd->resize;
    lcd->resize = lcd_mem_linux_resize;
#endif
  }

  return lcd;
}

static lcd_t* lcd_linux_create(fb_info_t* fb) {
  printf("=========fb_number=%d\n", fb_number(fb));
  return lcd_linux_create_flushable(fb);
}

lcd_t* lcd_linux_fb_create(const char* filename) {
  lcd_t* lcd = NULL;
  fb_info_t* fb = &s_fb;
  return_value_if_fail(filename != NULL, NULL);

  if (lcd_linux_fb_open(fb, filename)) {
    lcd = lcd_linux_create(fb);
  }

  atexit(on_app_exit);
  signal(SIGINT, on_signal_int);
  signal(SIGTERM, on_signal_int);

  return lcd;
}

#endif /*WITH_LINUX_FB*/
