/**
 * File:   lcd_mem_others.c
 * Author: AWTK Develop Team
 * Brief:  support other special format linux framebuffers
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
 * 2019-06-17 Li XianJing <xianjimli@hotmail.com> created
 *
 */

#include "lcd_mem_others.h"

static ret_t lcd_bgra5551_flush(lcd_t* lcd) {
  rect_t* dr = &(lcd->dirty_rect);
  lcd_mem_special_t* special = (lcd_mem_special_t*)lcd;
  fb_info_t* info = (fb_info_t*)(special->ctx);

  if (dr->w > 0 && dr->h > 0) {
    uint32_t x = 0;
    uint32_t y = 0;
    int src_line_length = lcd->w;
    int dst_line_length = info->fix.line_length / 2;
    uint16_t* dst = (uint16_t*)(info->fbmem0);
    uint16_t* src = (uint16_t*)(special->lcd_mem->offline_fb);

    src += dr->y * src_line_length + dr->x;
    dst += dr->y * dst_line_length + dr->x;

    for (y = 0; y < dr->h; y++) {
      for (x = 0; x < dr->w; x++) {
        uint16_t s = src[x];
        uint8_t b = 0x1f & s;
        uint8_t g = (0x7e0 & s) >> 5;
        uint8_t r = (0xf800 & s) >> 11;

        dst[x] = 0x8000 | (r << 10) | (g << 5) | b;
      }

      src += src_line_length;
      dst += dst_line_length;
    }
  }

  return RET_OK;
}

lcd_t* lcd_mem_bgra5551_create(fb_info_t* info) {
  wh_t w = fb_width(info);
  wh_t h = fb_height(info);

  return lcd_mem_special_create(w, h, BITMAP_FMT_BGR565, lcd_bgra5551_flush, NULL, NULL, info);
}
