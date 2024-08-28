/**
 * File:   lcd_wayland.h
 * Author: AWTK Develop Team
 * Brief:  thread to read /dev/input/
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

#ifndef UI_AWTK_WAYLAND_LCD_WAYLAND_H_
#define UI_AWTK_WAYLAND_LCD_WAYLAND_H_

#include "wayland_tools.h"
#include "tkc/mem.h"
#include "base/lcd.h"

typedef struct {
  struct wayland_data objs;
  void *impl_data;
} lcd_wayland_t;

lcd_wayland_t *lcd_wayland_create(int w, int h);
void kb_repeat(void);

#endif /* UI_AWTK_WAYLAND_LCD_WAYLAND_H_ */
