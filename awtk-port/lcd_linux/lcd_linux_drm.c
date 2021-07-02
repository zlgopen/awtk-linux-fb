/**
 * File:   lcd_linux_drm.c
 * Author: AWTK Develop Team
 * Brief:  linux drm lcd
 *
 * Copyright (c) 2020 - 2020  Guangzhou ZHIYUAN Electronics Co.,Ltd.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * License file for more details.
 *
 */

/**
 * adapted from https://github.com/dvdhrm/docs
 */

/**
 * History:
 * ================================================================
 * 2020-05-16 Li XianJing <xianjimli@hotmail.com> created
 *
 */

#ifdef WITH_LINUX_DRM

#include <signal.h>
#include <pthread.h>
#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <time.h>
#include <unistd.h>
#include <xf86drm.h>
#include <xf86drmMode.h>

#include "tkc/mem.h"
#include "base/lcd.h"
#include "tkc/time_now.h"
#include "awtk_global.h"
#include "lcd_mem_others.h"

#include "lcd/lcd_mem_special.h"

#include "lcd_linux.h"

struct modeset_buf;
struct modeset_dev;

static void* s_fb_resize_func_ctx = NULL;
static lcd_linux_fb_resize_func_t s_fb_resize_func = NULL;

static int modeset_prepare(int fd);
static void modeset_cleanup(int fd);
static int modeset_open(int* out, const char* node);
static int modeset_create_fb(int fd, struct modeset_buf* buf);
static void modeset_destroy_fb(int fd, struct modeset_buf* buf);
static int modeset_setup_dev(int fd, drmModeRes* res, drmModeConnector* conn,
                             struct modeset_dev* dev);
static int modeset_find_crtc(int fd, drmModeRes* res, drmModeConnector* conn,
                             struct modeset_dev* dev);

static int modeset_open(int* out, const char* node) {
  int fd, ret;
  uint64_t has_dumb;

  fd = open(node, O_RDWR | O_CLOEXEC);
  if (fd < 0) {
    ret = -errno;
    fprintf(stderr, "cannot open '%s': %m\n", node);
    return ret;
  }

  if (drmGetCap(fd, DRM_CAP_DUMB_BUFFER, &has_dumb) < 0 || !has_dumb) {
    fprintf(stderr, "drm device '%s' does not support dumb buffers\n", node);
    close(fd);
    return -EOPNOTSUPP;
  }

  *out = fd;
  return 0;
}

struct modeset_buf {
  uint32_t width;
  uint32_t height;
  uint32_t stride;
  uint32_t size;
  uint32_t handle;
  uint8_t* map;
  uint32_t fb;
};

struct modeset_dev {
  struct modeset_dev* next;

  unsigned int front_buf;
  struct modeset_buf bufs[2];

  drmModeModeInfo mode;
  uint32_t mode_list_size;
  drmModeModeInfo* mode_list;
  uint32_t conn;
  uint32_t crtc;
  drmModeCrtc* saved_crtc;

  bool pflip_pending;
  bool cleanup;
};

static struct modeset_dev* modeset_list = NULL;

/*
 * modeset_prepare() stays the same.
 */

static int modeset_prepare(int fd) {
  drmModeRes* res;
  drmModeConnector* conn;
  unsigned int i;
  struct modeset_dev* dev;
  int ret;

  /* retrieve resources */
  res = drmModeGetResources(fd);
  if (!res) {
    fprintf(stderr, "cannot retrieve DRM resources (%d): %m\n", errno);
    return -errno;
  }

  /* iterate all connectors */
  for (i = 0; i < res->count_connectors; ++i) {
    /* get information for each connector */
    conn = drmModeGetConnector(fd, res->connectors[i]);
    if (!conn) {
      fprintf(stderr, "cannot retrieve DRM connector %u:%u (%d): %m\n", i, res->connectors[i],
              errno);
      continue;
    }

    /* create a device structure */
    dev = malloc(sizeof(*dev));
    memset(dev, 0, sizeof(*dev));
    dev->conn = conn->connector_id;

    /* call helper function to prepare this connector */
    ret = modeset_setup_dev(fd, res, conn, dev);
    if (ret) {
      if (ret != -ENOENT) {
        errno = -ret;
        fprintf(stderr, "cannot setup device for connector %u:%u (%d): %m\n", i, res->connectors[i],
                errno);
      }
      free(dev);
      drmModeFreeConnector(conn);
      continue;
    }

    /* free connector data and link device into global list */
    drmModeFreeConnector(conn);
    dev->next = modeset_list;
    modeset_list = dev;
  }

  /* free resources again */
  drmModeFreeResources(res);
  return 0;
}

static int modeset_fb(int fd, struct modeset_dev* dev, uint32_t connector_id) {
  /* create framebuffer #1 for this CRTC */
  int ret = modeset_create_fb(fd, &dev->bufs[0]);
  if (ret) {
    fprintf(stderr, "cannot create framebuffer for connector %u\n", connector_id);
    return ret;
  }

  /* create framebuffer #2 for this CRTC */
  ret = modeset_create_fb(fd, &dev->bufs[1]);
  if (ret) {
    fprintf(stderr, "cannot create framebuffer for connector %u\n", connector_id);
    modeset_destroy_fb(fd, &dev->bufs[0]);
  }
  return ret;
}

static int modeset_setup_dev(int fd, drmModeRes* res, drmModeConnector* conn,
                             struct modeset_dev* dev) {
  int ret;
  int i = 0;

  /* check if a monitor is connected */
  if (conn->connection != DRM_MODE_CONNECTED) {
    fprintf(stderr, "ignoring unused connector %u\n", conn->connector_id);
    return -ENOENT;
  }

  /* check if there is at least one valid mode */
  if (conn->count_modes == 0) {
    fprintf(stderr, "no valid mode for connector %u\n", conn->connector_id);
    return -EFAULT;
  }

  /* copy the mode information into our device structure and into both
   * buffers */
  dev->mode_list = TKMEM_ZALLOCN(drmModeModeInfo, conn->count_modes);
  dev->mode_list_size = conn->count_modes;
  for (; i < conn->count_modes; i++) {
    memcpy(&(dev->mode_list[i]), &conn->modes[i], sizeof(dev->mode));
  }

  memcpy(&dev->mode, &conn->modes[0], sizeof(dev->mode));
  dev->bufs[0].width = conn->modes[0].hdisplay;
  dev->bufs[0].height = conn->modes[0].vdisplay;
  dev->bufs[1].width = conn->modes[0].hdisplay;
  dev->bufs[1].height = conn->modes[0].vdisplay;
  fprintf(stderr, "mode for connector %u is %ux%u\n", conn->connector_id, dev->bufs[0].width,
          dev->bufs[0].height);

  /* find a crtc for this connector */
  ret = modeset_find_crtc(fd, res, conn, dev);
  if (ret) {
    fprintf(stderr, "no valid crtc for connector %u\n", conn->connector_id);
    return ret;
  }

  return modeset_fb(fd, dev, conn->connector_id);
}

/*
 * modeset_find_crtc() stays the same.
 */

static int modeset_find_crtc(int fd, drmModeRes* res, drmModeConnector* conn,
                             struct modeset_dev* dev) {
  drmModeEncoder* enc;
  unsigned int i, j;
  int32_t crtc;
  struct modeset_dev* iter;

  /* first try the currently conected encoder+crtc */
  if (conn->encoder_id)
    enc = drmModeGetEncoder(fd, conn->encoder_id);
  else
    enc = NULL;

  if (enc) {
    if (enc->crtc_id) {
      crtc = enc->crtc_id;
      for (iter = modeset_list; iter; iter = iter->next) {
        if (iter->crtc == crtc) {
          crtc = -1;
          break;
        }
      }

      if (crtc >= 0) {
        drmModeFreeEncoder(enc);
        dev->crtc = crtc;
        return 0;
      }
    }

    drmModeFreeEncoder(enc);
  }

  /* If the connector is not currently bound to an encoder or if the
   * encoder+crtc is already used by another connector (actually unlikely
   * but lets be safe), iterate all other available encoders to find a
   * matching CRTC. */
  for (i = 0; i < conn->count_encoders; ++i) {
    enc = drmModeGetEncoder(fd, conn->encoders[i]);
    if (!enc) {
      fprintf(stderr, "cannot retrieve encoder %u:%u (%d): %m\n", i, conn->encoders[i], errno);
      continue;
    }

    /* iterate all global CRTCs */
    for (j = 0; j < res->count_crtcs; ++j) {
      /* check whether this CRTC works with the encoder */
      if (!(enc->possible_crtcs & (1 << j))) continue;

      /* check that no other device already uses this CRTC */
      crtc = res->crtcs[j];
      for (iter = modeset_list; iter; iter = iter->next) {
        if (iter->crtc == crtc) {
          crtc = -1;
          break;
        }
      }

      /* we have found a CRTC, so save it and return */
      if (crtc >= 0) {
        drmModeFreeEncoder(enc);
        dev->crtc = crtc;
        return 0;
      }
    }

    drmModeFreeEncoder(enc);
  }

  fprintf(stderr, "cannot find suitable CRTC for connector %u\n", conn->connector_id);
  return -ENOENT;
}

/*
 * modeset_create_fb() stays the same.
 */

static int modeset_create_fb(int fd, struct modeset_buf* buf) {
  struct drm_mode_create_dumb creq;
  struct drm_mode_destroy_dumb dreq;
  struct drm_mode_map_dumb mreq;
  int ret;

  /* create dumb buffer */
  memset(&creq, 0, sizeof(creq));
  creq.width = buf->width;
  creq.height = buf->height;
  creq.bpp = 32;
  ret = drmIoctl(fd, DRM_IOCTL_MODE_CREATE_DUMB, &creq);
  if (ret < 0) {
    fprintf(stderr, "cannot create dumb buffer (%d): %m\n", errno);
    return -errno;
  }
  buf->stride = creq.pitch;
  buf->size = creq.size;
  buf->handle = creq.handle;

  /* create framebuffer object for the dumb-buffer */
  ret = drmModeAddFB(fd, buf->width, buf->height, 24, 32, buf->stride, buf->handle, &buf->fb);
  if (ret) {
    fprintf(stderr, "cannot create framebuffer (%d): %m\n", errno);
    ret = -errno;
    goto err_destroy;
  }

  /* prepare buffer for memory mapping */
  memset(&mreq, 0, sizeof(mreq));
  mreq.handle = buf->handle;
  ret = drmIoctl(fd, DRM_IOCTL_MODE_MAP_DUMB, &mreq);
  if (ret) {
    fprintf(stderr, "cannot map dumb buffer (%d): %m\n", errno);
    ret = -errno;
    goto err_fb;
  }

  /* perform actual memory mapping */
  buf->map = mmap(0, buf->size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, mreq.offset);
  if (buf->map == MAP_FAILED) {
    fprintf(stderr, "cannot mmap dumb buffer (%d): %m\n", errno);
    ret = -errno;
    goto err_fb;
  }

  /* clear the framebuffer to 0 */
  memset(buf->map, 0, buf->size);

  return 0;

err_fb:
  drmModeRmFB(fd, buf->fb);
err_destroy:
  memset(&dreq, 0, sizeof(dreq));
  dreq.handle = buf->handle;
  drmIoctl(fd, DRM_IOCTL_MODE_DESTROY_DUMB, &dreq);
  return ret;
}

/*
 * modeset_destroy_fb() stays the same.
 */
static void modeset_destroy_fb(int fd, struct modeset_buf* buf) {
  struct drm_mode_destroy_dumb dreq;

  /* unmap buffer */
  munmap(buf->map, buf->size);

  /* delete framebuffer */
  drmModeRmFB(fd, buf->fb);

  /* delete dumb buffer */
  memset(&dreq, 0, sizeof(dreq));
  dreq.handle = buf->handle;
  drmIoctl(fd, DRM_IOCTL_MODE_DESTROY_DUMB, &dreq);
}

static void modeset_page_flip_event(int fd, unsigned int frame, unsigned int sec, unsigned int usec,
                                    void* data) {
  struct modeset_dev* dev = (struct modeset_dev*)data;

  dev->pflip_pending = false;
}

static void modeset_cleanup(int fd) {
  struct modeset_dev* iter;
  drmEventContext ev;
  int ret;

  /* init variables */
  memset(&ev, 0, sizeof(ev));
  ev.version = DRM_EVENT_CONTEXT_VERSION;
  ev.page_flip_handler = modeset_page_flip_event;

  while (modeset_list) {
    /* remove from global list */
    iter = modeset_list;
    modeset_list = iter->next;

    /* if a pageflip is pending, wait for it to complete */
    iter->cleanup = true;
    fprintf(stderr, "wait for pending page-flip to complete...\n");
    while (iter->pflip_pending) {
      ret = drmHandleEvent(fd, &ev);
      if (ret) break;
    }

    /* restore saved CRTC configuration */
    if (!iter->pflip_pending)
      drmModeSetCrtc(fd, iter->saved_crtc->crtc_id, iter->saved_crtc->buffer_id,
                     iter->saved_crtc->x, iter->saved_crtc->y, &iter->conn, 1,
                     &iter->saved_crtc->mode);
    drmModeFreeCrtc(iter->saved_crtc);

    /* destroy framebuffers */
    modeset_destroy_fb(fd, &iter->bufs[1]);
    modeset_destroy_fb(fd, &iter->bufs[0]);

    /* free allocated memory */
    
    TKMEM_FREE(iter->mode_list);
    free(iter);
  }
}

typedef struct _drm_info_t {
  int fd;
  uint32_t w;
  uint32_t h;
  uint32_t stride;
  struct modeset_dev* dev;
} drm_info_t;

static ret_t lcd_bgra8888_destroy(lcd_t* lcd) {
  lcd_mem_special_t* special = (lcd_mem_special_t*)lcd;
  drm_info_t* info = (drm_info_t*)(special->ctx);

  modeset_cleanup(info->fd);
  TKMEM_FREE(info);

  return RET_OK;
}

static ret_t drm_vsync(int fd) {
  fd_set fds;
  int ret = 0;
  time_t start, cur;
  struct timeval v;
  drmEventContext ev;

  FD_ZERO(&fds);
  srand(time(&start));
  memset(&v, 0, sizeof(v));
  memset(&ev, 0, sizeof(ev));

  ev.version = 2;
  ev.page_flip_handler = modeset_page_flip_event;

  while (time(&cur) < start + 5) {
    FD_SET(0, &fds);
    FD_SET(fd, &fds);
    v.tv_sec = start + 5 - cur;

    ret = select(fd + 1, &fds, NULL, NULL, &v);
    if (ret < 0) {
      fprintf(stderr, "select() failed with %d: %m\n", errno);
      break;
    } else if (FD_ISSET(fd, &fds)) {
      drmHandleEvent(fd, &ev);
      break;
    }
  }

  return RET_OK;
}

/*awtk related*/

static ret_t lcd_bgra8888_flush(lcd_t* lcd) {
  static int inited = 0;
  int ret = 0;
  uint32_t x = 0;
  uint32_t y = 0;
  lcd_mem_special_t* special = (lcd_mem_special_t*)lcd;
  drm_info_t* info = (drm_info_t*)(special->ctx);

  struct modeset_dev* dev = info->dev;
  struct modeset_buf* buf = &dev->bufs[dev->front_buf ^ 1];

  int fd = info->fd;
  int dst_line_length = info->stride;
  uint32_t* dst = (uint32_t*)(buf->map);
  uint32_t* src = (uint32_t*)(special->lcd_mem->offline_fb);

  dev->front_buf ^= 1;
  if (inited) {
    dev->pflip_pending = true;
    drm_vsync(fd);
  } else {
    inited = 1;
  }

  for (y = 0; y < lcd->h; y++) {
    for (x = 0; x < lcd->w; x++) {
      dst[x] = src[x];
    }
    src += lcd->w;
    dst = (uint32_t*)((char*)dst + dst_line_length);
  }

  ret = drmModePageFlip(fd, dev->crtc, buf->fb, DRM_MODE_PAGE_FLIP_EVENT, dev);
  if (ret) {
    log_error("cannot flip CRTC for connector %u (%d): %m\n", dev->conn, errno);
    return RET_FAIL;
  } else {
    return RET_OK;
  }
}

static int s_ttyfd = -1;
static bool_t s_app_quited = FALSE;

static void on_app_exit(void) {
  if (s_ttyfd >= 0) {
    ioctl(s_ttyfd, KDSETMODE, KD_TEXT);
  }
  close(s_ttyfd);
  log_debug("on_app_exit\n");
}

static void on_signal_int(int sig) {
  s_app_quited = TRUE;
  tk_quit();
}

ret_t lcd_linux_set_fb_resize_func(lcd_linux_fb_resize_func_t fb_resize_func, void* ctx) {
  return_value_if_fail(fb_resize_func != NULL, RET_OK);
  s_fb_resize_func = fb_resize_func;
  s_fb_resize_func_ctx = ctx;
  return RET_OK;
}

static ret_t (*lcd_drm_linux_resize_defalut)(lcd_t* lcd, wh_t w, wh_t h, uint32_t line_length);
static ret_t lcd_drm_linux_resize(lcd_t* lcd, wh_t w, wh_t h, uint32_t line_length) {
  ret_t ret = RET_OK;

  /* must has fb_resize_func */
  assert(s_fb_resize_func != NULL);
  ret = s_fb_resize_func(1, w, h, s_fb_resize_func_ctx);
  return_value_if_fail(ret == RET_OK, ret);

  if (lcd_drm_linux_resize_defalut != NULL) {
    lcd_drm_linux_resize_defalut(lcd, w, h, line_length);
  }

  return ret;
}

static ret_t lcd_linux_drm_resize_func(uint32_t fb_num, wh_t w, wh_t h, void* ctx) {
  int i = 0;
  int ret = 0;
  drmEventContext ev;
  int32_t find_number = -1;
  lcd_mem_special_t* special = (lcd_mem_special_t*)ctx;
  drm_info_t* info = (drm_info_t*)(special->ctx);
  struct modeset_dev* dev = info->dev;

  for (; i < dev->mode_list_size; i++) {
    if (dev->mode_list[i].hdisplay == w && dev->mode_list[i].vdisplay == h) {
      find_number = i;
      break;
    }
  }
  return_value_if_fail(find_number >= 0, RET_NOT_FOUND);

  /* init variables */
  memset(&ev, 0, sizeof(ev));
  ev.version = DRM_EVENT_CONTEXT_VERSION;
  ev.page_flip_handler = modeset_page_flip_event;

  while (dev->pflip_pending) {
    ret = drmHandleEvent(info->fd, &ev);
    if (ret) break;
  }

  if (!dev->pflip_pending) {
    drmModeSetCrtc(info->fd, dev->saved_crtc->crtc_id, dev->saved_crtc->buffer_id,
                    dev->saved_crtc->x, dev->saved_crtc->y, &dev->conn, 1,
                    &dev->saved_crtc->mode);
  }
  drmModeFreeCrtc(dev->saved_crtc);

  modeset_destroy_fb(info->fd, &(dev->bufs[1]));
  modeset_destroy_fb(info->fd, &(dev->bufs[0]));
  
  dev->bufs[0].width = w;
  dev->bufs[0].height = h;
  dev->bufs[1].width = w;
  dev->bufs[1].height = h;
  modeset_fb(info->fd, dev, dev->conn);      

  dev->front_buf = 0;
  info->w = dev->bufs[0].width;
  info->h = dev->bufs[0].height;
  info->stride = dev->bufs[0].stride;

  dev->saved_crtc = drmModeGetCrtc(info->fd, dev->crtc);
  memcpy(&(dev->mode), &(dev->mode_list[find_number]), sizeof(dev->mode));
  ret = drmModeSetCrtc(info->fd, dev->crtc, dev->bufs[0].fb, 0, 0, &dev->conn, 1, &dev->mode);

  return RET_OK;
}

lcd_t* lcd_linux_drm_create(const char* card) {
  lcd_t* lcd = NULL;
  int ret = 0, fd = 0;
  struct modeset_buf* buf = NULL;
  struct modeset_dev* iter = NULL;
  drm_info_t* drm = TKMEM_ZALLOC(drm_info_t);
  return_value_if_fail(drm != NULL, NULL);

  log_info("using card '%s'\n", card);

  ret = modeset_open(&fd, card);
  log_info("modeset_open return %d\n", ret);
  goto_error_if_fail(ret == 0);

  ret = modeset_prepare(fd);
  log_info("modeset_prepare return %d\n", ret);
  goto_error_if_fail(ret == 0);

  for (iter = modeset_list; iter; iter = iter->next) {
    iter->saved_crtc = drmModeGetCrtc(fd, iter->crtc);
    buf = &iter->bufs[iter->front_buf];
    ret = drmModeSetCrtc(fd, iter->crtc, buf->fb, 0, 0, &iter->conn, 1, &iter->mode);

    if (ret) {
      fprintf(stderr, "cannot set CRTC for connector %u (%d): %m\n", iter->conn, errno);
    } else {
      drm->fd = fd;
      drm->dev = iter;
      drm->w = buf->width;
      drm->h = buf->height;
      drm->stride = buf->stride;
      log_info("use dev %p w=%d h=%d\n", iter, drm->w, drm->h);
      break;
    }
  }

  s_ttyfd = open("/dev/tty1", O_RDWR);
  if (s_ttyfd >= 0) {
    ioctl(s_ttyfd, KDSETMODE, KD_GRAPHICS);
  }

  atexit(on_app_exit);
  signal(SIGINT, on_signal_int);
  lcd = lcd_mem_special_create(drm->w, drm->h, BITMAP_FMT_BGRA8888, lcd_bgra8888_flush, NULL,
                                lcd_bgra8888_destroy, drm);
  lcd_drm_linux_resize_defalut = lcd->resize;
  lcd->resize = lcd_drm_linux_resize;

   lcd_linux_set_fb_resize_func(lcd_linux_drm_resize_func, lcd);
  return lcd;
error:
  TKMEM_FREE(drm);
  if (fd > 0) {
    close(fd);
  }
  return NULL;
}
#endif /*WITH_LINUX_DRM*/
