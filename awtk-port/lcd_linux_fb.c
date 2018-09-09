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
#include "lcd/lcd_mem_bgra8888.h"
#include "lcd/lcd_mem_rgb565.h"

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

static ret_t fb_pan(fb_info_t* info, int xoffset, int yoffset, int onsync) {
  struct fb_var_screeninfo* var = &info->var;

  return_value_if_fail(var->xres_virtual >= (xoffset + var->xres), RET_FAIL);
  return_value_if_fail(var->yres_virtual >= (yoffset + var->yres), RET_FAIL);

  if (!info->fix.xpanstep && !info->fix.ypanstep && !info->fix.ywrapstep) {
    return RET_OK;
  }

  if (info->fix.xpanstep) {
    var->xoffset = xoffset - (xoffset % info->fix.xpanstep);
  } else {
    var->xoffset = 0;
  }

  if (info->fix.ywrapstep) {
    var->yoffset = yoffset - (yoffset % info->fix.ywrapstep);
    var->vmode |= FB_VMODE_YWRAP;
  } else if (info->fix.ypanstep) {
    var->yoffset = yoffset - (yoffset % info->fix.ypanstep);
    var->vmode &= ~FB_VMODE_YWRAP;
  } else {
    var->yoffset = 0;
  }

  var->activate = onsync ? FB_ACTIVATE_VBL : FB_ACTIVATE_NOW;

  log_debug("%s: xoffset=%d yoffset=%d ywrapstep=%d\n", __func__, var->xoffset, var->yoffset,
            info->fix.ywrapstep);
  if (ioctl(info->fd, FBIOPAN_DISPLAY, var) < 0) {
    return RET_FAIL;
  }

  return RET_OK;
}

static void fb_sync(fb_info_t* info) {
  int ret = 0;
  int zero = 0;
  ret = ioctl(info->fd, FBIO_WAITFORVSYNC, &zero);
  ret = fb_pan(info, 0, 0, 1);
  ret = ioctl(info->fd, FBIO_WAITFORVSYNC, &zero);
#ifdef USE_FB_ACTIVATE_ALL
  ret = ioctl(info->fd, FB_ACTIVATE_ALL, NULL);
  log_debug("%s: FB_ACTIVATE_ALL ret = %d\n", __func__, ret);
#endif
  (void)ret;
  return;
}


static fb_info_t fb;

static ret_t lcd_linux_fb_flush(lcd_t* lcd) {
  fb_sync(&fb);

  return RET_OK;
}

lcd_t* lcd_linux_fb_create(const char* filename) {
  lcd_t* lcd = NULL;
  return_value_if_fail(filename != NULL, NULL);

  if (fb_open(&fb, filename) == 0) {
    int w = fb_width(&fb);
    int h = fb_height(&fb);
    uint8_t* bits = (uint8_t*)(fb.bits);
    int bits_per_pixel = fb.var.bits_per_pixel;

    if (bits_per_pixel == 16) {
      lcd = lcd_mem_rgb565_create_single_fb(w, h, bits);
    } else if (bits_per_pixel == 32) {
      lcd = lcd_mem_bgra8888_create_single_fb(w, h, bits);
    } else if (bits_per_pixel == 24) {
      assert(!"not supported framebuffer format.");
    } else {
      assert(!"not supported framebuffer format.");
    }
  }

  if(lcd != NULL) {
    lcd->flush = lcd_linux_fb_flush;
  }

  return lcd;
}
