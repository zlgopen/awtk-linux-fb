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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/fb.h>
#include <linux/kd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include "base/lcd.h"
#include "base/mem.h"
#include "lcd/lcd_mem_rgb565.h"
#include "lcd/lcd_mem_bgra8888.h"
#include "lcd/lcd_mem_rgba8888.h"

typedef struct _fb_info_t {
  int fd;
  void* bits;
  struct fb_fix_screeninfo fix;
  struct fb_var_screeninfo var;
} fb_info_t;

#define fb_width(fb) ((fb)->var.xres)
#define fb_height(fb) ((fb)->var.yres)
#define fb_size(fb) ((fb)->var.xres * (fb)->var.yres * fb->var.bits_per_pixel / 8)
#ifndef FBIO_WAITFORVSYNC
#define FBIO_WAITFORVSYNC _IOW('F', 0x20, u_int32_t)
#endif

static int fb_open(fb_info_t* fb, const char* filename) {
  fb->fd = open(filename, O_RDWR);
  if (fb->fd < 0) {
    return -1;
  }

  if (ioctl(fb->fd, FBIOGET_FSCREENINFO, &fb->fix) < 0) goto fail;
  if (ioctl(fb->fd, FBIOGET_VSCREENINFO, &fb->var) < 0) goto fail;

  fb->var.xoffset = 0;
  fb->var.yoffset = 0;
  ioctl(fb->fd, FBIOPAN_DISPLAY, &(fb->var));

  log_debug("fb_info_t: %s\n", filename);
  log_debug("fb_info_t: xres=%d yres=%d bits_per_pixel=%d mem_size=%d\n", fb->var.xres,
            fb->var.yres, fb->var.bits_per_pixel, fb_size(fb));
  log_debug("fb_info_t: red(%d %d) green(%d %d) blue(%d %d)\n", fb->var.red.offset,
            fb->var.red.length, fb->var.green.offset, fb->var.green.length, fb->var.blue.offset,
            fb->var.blue.length);

#ifdef FTK_FB_NOMMAP
  // uclinux doesn't support MAP_SHARED or MAP_PRIVATE with PROT_WRITE, so no mmap at all is simpler
  fb->bits = fb->fix.smem_start;
#else
  fb->bits = mmap(0, fb_size(fb), PROT_READ | PROT_WRITE, MAP_SHARED, fb->fd, 0);
#endif

  if (fb->bits == MAP_FAILED) {
    log_debug("map framebuffer failed.\n");
    goto fail;
  }

  memset(fb->bits, 0xff, fb_size(fb));

  log_debug("xres_virtual =%d yres_virtual=%d xpanstep=%d ywrapstep=%d\n", fb->var.xres_virtual,
            fb->var.yres_virtual, fb->fix.xpanstep, fb->fix.ywrapstep);

  return 0;
fail:
  log_debug("%s is not a framebuffer.\n", filename);
  close(fb->fd);

  return -1;
}

static void fb_close(fb_info_t* fb) {
  if (fb != NULL) {
    munmap(fb->bits, fb_size(fb));
    close(fb->fd);
  }

  return;
}

static void fb_sync(fb_info_t* info) {
  int ret = 0;
  int zero = 0;
  ret = ioctl(info->fd, FBIO_WAITFORVSYNC, &zero);

  log_debug("FBIO_WAITFORVSYNC: %d %d\n", ret, zero);

  return;
}

static fb_info_t s_fb;
static lcd_flush_t s_lcd_flush_default = NULL;

static ret_t lcd_linux_fb_flush(lcd_t* lcd) {
  if(s_lcd_flush_default != NULL) {
    s_lcd_flush_default(lcd);
  }

  fb_sync(&s_fb);

  return RET_OK;
}

lcd_t* lcd_linux_fb_create(const char* filename) {
  lcd_t* lcd = NULL;
  fb_info_t* fb = &s_fb;
  return_value_if_fail(filename != NULL, NULL);

  if (fb_open(fb, filename) == 0) {
    int w = fb_width(fb);
    int h = fb_height(fb);
    int size = fb_size(fb);
    int bpp = fb->var.bits_per_pixel;
    uint8_t* online_fb = (uint8_t*)(fb->bits);
    uint8_t* offline_fb = (uint8_t*)malloc(size);

    if(offline_fb == NULL) {
      fb_close(fb);

      return NULL;
    }

    if (bpp == 16) {
      lcd = lcd_mem_rgb565_create_double_fb(w, h, online_fb, offline_fb);
    } else if (bpp == 32) {
      lcd = lcd_mem_bgra8888_create_double_fb(w, h, online_fb, offline_fb);
    } else if (bpp == 24) {
      assert(!"not supported framebuffer format.");
    } else {
      assert(!"not supported framebuffer format.");
    }
  }

  if(lcd != NULL) {
    s_lcd_flush_default = lcd->flush;
    lcd->flush = lcd_linux_fb_flush; 
  }

  return lcd;
}
