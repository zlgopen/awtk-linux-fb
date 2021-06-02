/**
 * File:   fb_info.h
 * Author: AWTK Develop Team
 * Brief:  linux framebuffer related functions
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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <assert.h>
#include <linux/fb.h>
#include <linux/kd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include "base/lcd.h"
#include "tkc/mem.h"

#ifndef TK_FB_INFO_H
#define TK_FB_INFO_H

typedef struct _fb_info_t {
  int fd;
  uint8_t* fbmem0;
  uint8_t* fbmem1;
  struct fb_fix_screeninfo fix;
  struct fb_var_screeninfo var;

  /*for swappable draw snapshot*/
  uint8_t* offline_fb;
} fb_info_t;

#define fb_width(fb) ((fb)->var.xres)
#define fb_height(fb) ((fb)->var.yres)
#define fb_memsize(fb) ((fb)->fix.smem_len)
#define fb_bpp(fb) ((fb)->var.bits_per_pixel)
#define fb_line_length(fb) ((fb)->fix.line_length)
#define fb_size(fb) ((fb)->var.yres * (fb)->fix.line_length)
#define fb_vsize(fb) ((fb)->var.yres_virtual * (fb)->fix.line_length)
#define fb_number(fb) (fb_vsize(fb) / fb_size(fb))

#define fb_is_1fb(fb) ((fb)->var.yres_virtual < 2 * (fb)->var.yres)
#define fb_is_2fb(fb) (fb_vsize(fb) / fb_size(fb) >= 2)
#define fb_is_3fb(fb) 0  //((fb)->var.yres_virtual == 3 * (fb)->var.yres)

static inline bool_t fb_is_bgra5551(fb_info_t* fb) {
  struct fb_var_screeninfo* var = &(fb->var);
  if (var->bits_per_pixel == 16 && var->blue.offset == 0 && var->green.offset == 5 &&
      var->red.offset == 10 && var->blue.length == 5 && var->green.length == 5 &&
      var->red.length == 5 && var->transp.length == 1) {
    return TRUE;
  } else {
    return FALSE;
  }
}

static inline bool_t fb_is_bgr565(fb_info_t* fb) {
  struct fb_var_screeninfo* var = &(fb->var);
  if (var->bits_per_pixel == 16 && var->blue.offset == 0 && var->green.offset == 5 &&
      var->red.offset == 11 && var->blue.length == 5 && var->green.length == 6 &&
      var->red.length == 5) {
    return TRUE;
  } else {
    return FALSE;
  }
}

static inline bool_t fb_is_rgb565(fb_info_t* fb) {
  struct fb_var_screeninfo* var = &(fb->var);
  if (var->bits_per_pixel == 16 && var->red.offset == 0 && var->green.offset == 5 &&
      var->blue.offset == 11 && var->red.length == 5 && var->green.length == 6 &&
      var->blue.length == 5) {
    return TRUE;
  } else {
    return FALSE;
  }
}

static inline bool_t fb_is_rgba8888(fb_info_t* fb) {
  struct fb_var_screeninfo* var = &(fb->var);
  if (var->bits_per_pixel == 32 && var->red.offset == 0 && var->green.offset == 8 &&
      var->blue.offset == 16 && var->red.length == 8 && var->green.length == 8 &&
      var->blue.length == 8) {
    return TRUE;
  } else {
    return FALSE;
  }
}

static inline bool_t fb_is_bgra8888(fb_info_t* fb) {
  struct fb_var_screeninfo* var = &(fb->var);
  if (var->bits_per_pixel == 32 && var->blue.offset == 0 && var->green.offset == 8 &&
      var->red.offset == 16 && var->red.length == 8 && var->green.length == 8 &&
      var->blue.length == 8) {
    return TRUE;
  } else {
    return FALSE;
  }
}

static inline bool_t fb_is_rgb888(fb_info_t* fb) {
  struct fb_var_screeninfo* var = &(fb->var);
  if (var->bits_per_pixel == 24 && var->red.offset == 0 && var->green.offset == 8 &&
      var->blue.offset == 16 && var->red.length == 8 && var->green.length == 8 &&
      var->blue.length == 8) {
    return TRUE;
  } else {
    return FALSE;
  }
}

static inline bool_t fb_is_bgr888(fb_info_t* fb) {
  struct fb_var_screeninfo* var = &(fb->var);
  if (var->bits_per_pixel == 24 && var->blue.offset == 0 && var->green.offset == 8 &&
      var->red.offset == 16 && var->red.length == 8 && var->green.length == 8 &&
      var->blue.length == 8) {
    return TRUE;
  } else {
    return FALSE;
  }
}

#ifndef FBIO_WAITFORVSYNC
#define FBIO_WAITFORVSYNC _IOW('F', 0x20, u_int32_t)
#endif /*FBIO_WAITFORVSYNC*/

static inline int fb_open(fb_info_t* fb, const char* filename) {
  uint32_t size = 0;
  uint32_t fb_nr = 1;
  uint32_t total_size = 0;

  memset(fb, 0x00, sizeof(fb_info_t));

  fb->fd = open(filename, O_RDWR);
  if (fb->fd < 0) {
    log_warn("open: %s failed\n", filename);
    return -1;
  }

  if (ioctl(fb->fd, FBIOGET_FSCREENINFO, &fb->fix) < 0) goto fail;
  if (ioctl(fb->fd, FBIOGET_VSCREENINFO, &fb->var) < 0) goto fail;

  fb->var.xoffset = 0;
  fb->var.yoffset = 0;

  size = fb_size(fb);
  fb_nr = fb_number(fb);
  total_size = fb_memsize(fb);

  log_info("fb_info_t: %s\n", filename);
  log_info("xres=%d yres=%d\n", fb->var.xres, fb->var.yres);
  log_info("xres_virtual=%d yres_virtual=%d\n", fb->var.xres_virtual, fb->var.yres_virtual);
  log_info("bits_per_pixel=%d line_length=%d\n", fb->var.bits_per_pixel, fb->fix.line_length);
  log_info("fb_info_t: red(%d %d) green(%d %d) blue(%d %d)\n", fb->var.red.offset,
           fb->var.red.length, fb->var.green.offset, fb->var.green.length, fb->var.blue.offset,
           fb->var.blue.length);
  log_info("xpanstep=%u ywrapstep=%u\n", fb->fix.xpanstep, fb->fix.ywrapstep);
  log_info("fb_size=%u fb_total_size=%u fb_nr=%u smem_len=%u\n", size, total_size, fb_nr,
           fb->fix.smem_len);

#ifdef FTK_FB_NOMMAP
  // uclinux doesn't support MAP_SHARED or MAP_PRIVATE with PROT_WRITE, so no mmap at all is simpler
  fb->fbmem0 = (uint8_t*)(fb->fix.smem_start);
#else
  fb->fbmem0 = (uint8_t*)mmap(0, total_size, PROT_READ | PROT_WRITE, MAP_SHARED, fb->fd, 0);
#endif

  if (fb->fbmem0 == MAP_FAILED) {
    perror("framebuffer");
    log_error("map framebuffer failed.\n");
    goto fail;
  }

  log_info("fb_open clear\n");
  //memset(fb->fbmem0, 0xff, total_size);
  if (fb_is_2fb(fb)) {
    fb->fbmem1 = fb->fbmem0 + size;
  } else {
    fb->fbmem1 = NULL;
  }
  log_info("fb_open ok\n");

  return 0;
fail:
  perror("framebuffer");
  log_warn("%s is not a framebuffer.\n", filename);
  close(fb->fd);

  return -1;
}

static inline void fb_close(fb_info_t* fb) {
  if (fb != NULL) {
    uint32_t total_size = fb_memsize(fb);

    log_info("fb_close\n");
    if (fb_is_1fb(fb)) {
      if (fb->fbmem1 != NULL) {
        free(fb->fbmem1);
      }
    }

    if (fb->offline_fb != NULL) {
      free(fb->offline_fb);
    }

    munmap(fb->fbmem0, total_size);
    close(fb->fd);
    log_info("fb_close ok\n");
  }

  return;
}

static inline void fb_sync(fb_info_t* info) {
  int ret = 0;
  int zero = 0;
  ret = ioctl(info->fd, FBIO_WAITFORVSYNC, &zero);

  if (ret != 0) {
    log_debug("FBIO_WAITFORVSYNC: %d %d\n", ret, zero);
  }

  return;
}

static inline bool_t check_if_run_in_vmware() {
  bool_t run_in_vmware = FALSE;

  FILE* dmidecode = popen("dmidecode -s system-product-name", "r");
  if (dmidecode) {
    char system_product_name[32] = {0};
    char* result = fgets(system_product_name, sizeof(system_product_name) - 1, dmidecode);

    if (result && strstr(result, "VMware")) {
      run_in_vmware = TRUE;
    }
    pclose(dmidecode);
  }
  return run_in_vmware;
}

#endif /*TK_FB_INFO_H*/
