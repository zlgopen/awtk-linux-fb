/**
 * File:   lcd_wayland.h
 * Author: AWTK Develop Team
 * Brief:  lcd wayland
 *
 * Copyright (c) 2018 - 2024 Guangzhou ZHIYUAN Electronics Co.,Ltd.
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
 * 2024-07-17 Yang Zewu <yangzewu@zlg.cn> created
 *
 */
#ifndef UI_AWTK_WAYLAND_LCD_WAYLAND_H
#define UI_AWTK_WAYLAND_LCD_WAYLAND_H

#include "pthread_signal.h"
#include "wayland_tools.h"
#include "tkc/mem.h"
#include "base/lcd.h"
#include "lcd/lcd_mem_bgr565.h"
#include "lcd/lcd_mem_rgb565.h"
#include "lcd/lcd_mem_bgra8888.h"
#include "lcd/lcd_mem_rgba8888.h"

typedef struct _lcd_wayland_t{
  double_buffer_list_t* current;
  wayland_data_t objs;
  void* impl_data;
} lcd_wayland_t;

lcd_t* lcd_wayland_create(int w, int h);
void kb_repeat(wayland_data_t* objs);

#endif /* UI_AWTK_WAYLAND_LCD_WAYLAND_H */
