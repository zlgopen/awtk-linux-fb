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

#define __FB_SUP_RESIZE    1
#define __FB_WAIT_VSYNC    1
#define __FB_ASYNC_SWAP    0

#if __FB_ASYNC_SWAP
static tk_thread_t* s_t_fbswap = NULL;
static tk_semaphore_t* s_sem_spare = NULL;
static tk_semaphore_t* s_sem_ready = NULL;
static tk_mutex_t* s_lck_fblist = NULL;
static bool_t s_app_quited = FALSE;
#endif

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

#if __FB_ASYNC_SWAP
  s_app_quited = TRUE;
  tk_semaphore_post(s_sem_spare);
  tk_semaphore_post(s_sem_ready);
  sleep_ms(200);

  if (s_t_fbswap) {
    tk_thread_join(s_t_fbswap);
    tk_thread_destroy(s_t_fbswap);
  }

  if (s_sem_spare) {
    tk_semaphore_destroy(s_sem_spare);
  }

  if (s_sem_ready) {
    tk_semaphore_destroy(s_sem_ready);
  }

  if (s_lck_fblist) {
    tk_mutex_destroy(s_lck_fblist);
  }
#endif

  lcd_linux_fb_close(fb);

  log_debug("on_app_exit\n");
}

#if __FB_SUP_RESIZE
static ret_t (*lcd_mem_linux_resize_defalut)(lcd_t* lcd, wh_t w, wh_t h, uint32_t line_length);
static ret_t lcd_mem_linux_resize(lcd_t* lcd, wh_t w, wh_t h, uint32_t line_length) {
#if __FB_ASYNC_SWAP
  log_debug("linuxfb async swap mode not support fb resize.\n");
  return RET_FAIL;
#endif

  ret_t ret = RET_OK;
  fb_info_t* fb = &s_fb;
  lcd_mem_t* mem = (lcd_mem_t*)lcd;
  return_value_if_fail(lcd != NULL, RET_BAD_PARAMS);

  ret = fb_resize_reopen(fb, w, h);
  if (!mem->own_offline_fb) {
    // mem->own_offline_fb=1 means lcd_mem_special_create will manage(alloc/free) it's own offline fb mem
    lcd_mem_set_offline_fb(mem, fb->fbmem_offline);
  }
  lcd_mem_set_online_fb(mem, (uint8_t*)(fb->fbmem0));
  lcd_mem_set_line_length(lcd, fb_line_length(fb));

  if (lcd_mem_linux_resize_defalut && ret == RET_OK) {
    lcd_mem_linux_resize_defalut(lcd, w, h, line_length);
  }

  log_debug("lcd_linux_fb_resize \r\n");
  return ret;
}
#endif

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

static ret_t lcd_linux_flush(lcd_t* base, int fbid) {
//printf("==========lcd_linux_flush fbid=%d\n", fbid);//###DEBUG###

  uint8_t* buff = NULL;
  fb_info_t* fb = &s_fb;
  int fb_nr = fb_number(fb);
  uint32_t size = fb_size(fb);
  lcd_mem_t* lcd = (lcd_mem_t*)base;
  const dirty_rects_t* dirty_rects = NULL;
  lcd_orientation_t o = system_info()->lcd_orientation;

  return_value_if_fail(lcd != NULL && fb != NULL && fbid < fb_nr, RET_BAD_PARAMS);

  buff = fb->fbmem0 + size * fbid;

  bitmap_t online_fb;
  bitmap_t offline_fb;
  lcd_linux_init_drawing_fb(lcd, &offline_fb);
  lcd_linux_init_online_fb(lcd, &online_fb, buff, fb_width(fb), fb_height(fb), fb_line_length(fb));
  
  // add current online buff to the dirty manager, and notify all managed buff update/merge with the base dirty rect
  lcd_fb_dirty_rects_add_fb_info(&(lcd->fb_dirty_rects_list), buff);
  lcd_fb_dirty_rects_update_all_fb_dirty_rects(&(lcd->fb_dirty_rects_list), base->dirty_rects);

  // get the merged dirty rects of current online buff, and then copy each small rects from offline fb
  dirty_rects = lcd_fb_dirty_rects_get_dirty_rects_by_fb(&(lcd->fb_dirty_rects_list), buff);
  if (dirty_rects != NULL && dirty_rects->nr > 0) {
    for (int i = 0; i < dirty_rects->nr; i++) {
      const rect_t* dr = (const rect_t*)dirty_rects->rects + i;
      if (o == LCD_ORIENTATION_0) {
        image_copy(&online_fb, &offline_fb, dr, dr->x, dr->y);
      } else {
        image_rotate(&online_fb, &offline_fb, dr, o);
      }
    }
  }
  
  // reset current online buff dirty rect to empty, because all new contnet has copyed from offline fb
  lcd_fb_dirty_rects_reset_dirty_rects_by_fb(&(lcd->fb_dirty_rects_list), buff);
  return RET_OK;
}

static void on_signal_int(int sig) {
  tk_quit();
}

static ret_t (*lcd_mem_linux_flush_defalut)(lcd_t* lcd);
static ret_t lcd_mem_linux_flush(lcd_t* lcd) {
#if __FB_WAIT_VSYNC
  fb_info_t* fb = &s_fb;
  fb_sync(fb);
#endif

  if (lcd_mem_linux_flush_defalut) {
    lcd_mem_linux_flush_defalut(lcd);
  }

//printf("---------lcd_mem_linux_flush VSYNC=%d, ASWAP=%d\n", __FB_WAIT_VSYNC, __FB_ASYNC_SWAP);//###DEBUG###
  return RET_OK;
}

static lcd_t* lcd_linux_create_flushable(fb_info_t* fb) {
  lcd_t* lcd = NULL;
  int w = fb_width(fb);
  int h = fb_height(fb);
  int line_length = fb_line_length(fb);

  int bpp = fb_bpp(fb);
  uint8_t* online_fb = (uint8_t*)(fb->fbmem0);
  uint8_t* offline_fb = (uint8_t*)(fb->fbmem_offline);
  return_value_if_fail(offline_fb != NULL, NULL);

  if (bpp == 16) {
    if (fb_is_bgra5551(fb)) {
      lcd = lcd_mem_bgra5551_create(fb);
    } else if (fb_is_bgr565(fb)) {
      lcd = lcd_mem_bgr565_create_double_fb(w, h, online_fb, offline_fb);
    } else if (fb_is_rgb565(fb)) {
      lcd = lcd_mem_rgb565_create_double_fb(w, h, online_fb, offline_fb);
    } else {
      assert(!"not supported framebuffer format.");
    }
  } else if (bpp == 32) {
    if (fb_is_bgra8888(fb)) {
      lcd = lcd_mem_bgra8888_create_double_fb(w, h, online_fb, offline_fb);
    } else if (fb_is_rgba8888(fb)) {
      lcd = lcd_mem_rgba8888_create_double_fb(w, h, online_fb, offline_fb);
    } else {
      assert(!"not supported framebuffer format.");
    }
  } else if (bpp == 24) {
    if (fb_is_bgr888(fb)) {
      lcd = lcd_mem_bgr888_create_double_fb(w, h, online_fb, offline_fb);
    } else if (fb_is_rgb888(fb)) {
      lcd = lcd_mem_rgb888_create_double_fb(w, h, online_fb, offline_fb);
    } else {
      assert(!"not supported framebuffer format.");
    }
  } else {
    assert(!"not supported framebuffer format.");
  }

  if (lcd != NULL) {
    lcd_mem_linux_flush_defalut = lcd->flush;
    lcd->flush = lcd_mem_linux_flush;
    lcd_mem_set_line_length(lcd, line_length);

#if __FB_SUP_RESIZE
    lcd_mem_linux_resize_defalut = lcd->resize;
    lcd->resize = lcd_mem_linux_resize;
#endif
  }

  return lcd;
}

enum {
  FB_TAG_UND = 0,
  FB_TAG_SPARE,
  FB_TAG_READY,
  FB_TAG_BUSY
};
typedef struct fb_taged {
  int fbid;
  int tags;
} fb_taged_t;

#define FB_LIST_NUM 3
static fb_taged_t s_fblist[FB_LIST_NUM];
static void init_fblist(int num) {
  memset(s_fblist, 0, sizeof(s_fblist));
  for (int i = 0; i < num && i < FB_LIST_NUM; i++) {
    s_fblist[i].fbid = i;
    s_fblist[i].tags = FB_TAG_SPARE;
  }
}
static fb_taged_t* get_spare_fb() {
  for (int i = 0; i < FB_LIST_NUM; i++) {
    if (s_fblist[i].tags == FB_TAG_SPARE) {
      return &s_fblist[i];
    }
  }
  return NULL;
}
static fb_taged_t* get_busy_fb() {
  for (int i = 0; i < FB_LIST_NUM; i++) {
    if (s_fblist[i].tags == FB_TAG_BUSY) {
      return &s_fblist[i];
    }
  }
  return NULL;
}
static fb_taged_t* get_ready_fb() {
  fb_taged_t* last_busy_fb = get_busy_fb();

  if (last_busy_fb) {
    // get the first ready slot next to the busy one
    for (int i = 1; i < FB_LIST_NUM; i++) {
      int next_ready_fbid = (last_busy_fb->fbid + i) % FB_LIST_NUM;

      if (s_fblist[next_ready_fbid].tags == FB_TAG_READY) {
        return &s_fblist[next_ready_fbid];
      }
    }
  } else {
    // no last busy, get the first ready one
    for (int i = 0; i < FB_LIST_NUM; i++) {
      if (s_fblist[i].tags == FB_TAG_READY) {
        return &s_fblist[i];
      }
    }
  }
  return NULL;
}

#if __FB_ASYNC_SWAP

static ret_t lcd_mem_linux_wirte_buff(lcd_t* lcd) {
  ret_t ret = RET_OK;
  if (s_app_quited) {
    return ret;
  }

  if (lcd->draw_mode != LCD_DRAW_OFFLINE) {
    tk_semaphore_wait(s_sem_spare, -1);
    if (s_app_quited) {
      return ret;
    }

    tk_mutex_lock(s_lck_fblist);
    fb_taged_t* spare_fb = get_spare_fb();
    assert(spare_fb);
    tk_mutex_unlock(s_lck_fblist);

    ret = lcd_linux_flush(lcd, spare_fb->fbid);

    tk_mutex_lock(s_lck_fblist);
    spare_fb->tags = FB_TAG_READY;
    tk_semaphore_post(s_sem_ready);
    tk_mutex_unlock(s_lck_fblist);

    int sched_yield(void);
    sched_yield();

//printf("==========lcd_mem_linux_wirte_buff VSYNC=%d, ASWAP=%d\n", __FB_WAIT_VSYNC, __FB_ASYNC_SWAP);//###DEBUG###
  }

  return ret;
}

static void* fbswap_thread(void* ctx) {
  fb_info_t* fb = &s_fb;
  struct fb_var_screeninfo vi = (fb->var);

  log_info("display_thread start\n");

  while (!s_app_quited) {
    tk_semaphore_wait(s_sem_ready, -1);
    if (s_app_quited) {
      break;
    }

    tk_mutex_lock(s_lck_fblist);
    fb_taged_t* ready_fb = get_ready_fb();
    assert(ready_fb);
    int ready_fbid = ready_fb->fbid;
    tk_mutex_unlock(s_lck_fblist);

    vi.yoffset = ready_fbid * fb_height(fb);
    ioctl(fb->fd, FBIOPAN_DISPLAY, &vi);

#if __FB_WAIT_VSYNC
    int dummy = 0;
    ioctl(fb->fd, FBIO_WAITFORVSYNC, &dummy);
#endif

    tk_mutex_lock(s_lck_fblist);
    fb_taged_t* last_busy_fb = get_busy_fb();
    if (last_busy_fb) {
      last_busy_fb->tags = FB_TAG_SPARE;
      tk_semaphore_post(s_sem_spare);
    }
    ready_fb->tags = FB_TAG_BUSY;
    tk_mutex_unlock(s_lck_fblist);
  }

  log_info("display_thread end\n");
  return NULL;
}
#else // __FB_ASYNC_SWAP

static ret_t lcd_mem_linux_wirte_buff(lcd_t* lcd) {
  ret_t ret = RET_OK;
  fb_info_t* fb = &s_fb;
  struct fb_var_screeninfo vi = (fb->var);

  if (lcd->draw_mode != LCD_DRAW_OFFLINE) {
    fb_taged_t* spare_fb = get_spare_fb();
    ret = lcd_linux_flush(lcd, spare_fb->fbid);

    vi.yoffset = spare_fb->fbid * fb_height(fb);
    ioctl(fb->fd, FBIOPAN_DISPLAY, &vi);

#if __FB_WAIT_VSYNC
    int dummy = 0;
    ioctl(fb->fd, FBIO_WAITFORVSYNC, &dummy);
#endif

    fb_taged_t* last_busy_fb = get_busy_fb();
    if (last_busy_fb) {
      last_busy_fb->tags = FB_TAG_SPARE;
    }
    spare_fb->tags = FB_TAG_BUSY;

//printf("==========lcd_mem_linux_wirte_buff VSYNC=%d, ASWAP=%d\n", __FB_WAIT_VSYNC, __FB_ASYNC_SWAP);//###DEBUG###
  }

  return ret;
}
#endif // __FB_ASYNC_SWAP

static lcd_t* lcd_linux_create_swappable(fb_info_t* fb) {
  lcd_t* lcd = NULL;
  int w = fb_width(fb);
  int h = fb_height(fb);
  int bpp = fb_bpp(fb);
  int line_length = fb_line_length(fb);

  uint8_t* offline_fb = (uint8_t*)(fb->fbmem_offline);
  return_value_if_fail(offline_fb != NULL, NULL);

  if (bpp == 16) {
    if (fb_is_bgr565(fb)) {
      lcd = lcd_mem_bgr565_create_single_fb(w, h, offline_fb);
    } else if (fb_is_rgb565(fb)) {
      lcd = lcd_mem_rgb565_create_single_fb(w, h, offline_fb);
    } else {
      assert(!"not supported framebuffer format.");
    }
  } else if (bpp == 32) {
    if (fb_is_bgra8888(fb)) {
      lcd = lcd_mem_bgra8888_create_single_fb(w, h, offline_fb);
    } else if (fb_is_rgba8888(fb)) {
      lcd = lcd_mem_rgba8888_create_single_fb(w, h, offline_fb);
    } else {
      assert(!"not supported framebuffer format.");
    }
  } else if (bpp == 24) {
    if (fb_is_bgr888(fb)) {
      lcd = lcd_mem_bgr888_create_single_fb(w, h, offline_fb);
    } else if (fb_is_rgb888(fb)) {
      lcd = lcd_mem_rgb888_create_single_fb(w, h, offline_fb);
    } else {
      assert(!"not supported framebuffer format.");
    }
  } else {
    assert(!"not supported framebuffer format.");
  }

  if (lcd != NULL) {
    lcd->swap = lcd_mem_linux_wirte_buff;
    lcd->flush = lcd_mem_linux_wirte_buff;
    lcd_mem_set_line_length(lcd, line_length);

#if __FB_SUP_RESIZE
    lcd_mem_linux_resize_defalut = lcd->resize;
    lcd->resize = lcd_mem_linux_resize;
#endif

#if __FB_ASYNC_SWAP
    s_lck_fblist = tk_mutex_create();
    s_sem_spare = tk_semaphore_create(fb_number(fb), NULL);
    s_sem_ready = tk_semaphore_create(0, NULL);
    s_t_fbswap = tk_thread_create(fbswap_thread, lcd);
    tk_thread_start(s_t_fbswap);
#endif
  }

  return lcd;
}

static lcd_t* lcd_linux_create(fb_info_t* fb) {
  init_fblist(fb_number(fb));
  printf("=========fb_number=%d\n", fb_number(fb));

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

  if (lcd_linux_fb_open(fb, filename)) {
    lcd = lcd_linux_create(fb);
  }

  atexit(on_app_exit);
  signal(SIGINT, on_signal_int);

  return lcd;
}

#endif /*WITH_LINUX_FB*/
