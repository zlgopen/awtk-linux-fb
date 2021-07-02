/**
 * File:   lcd_linux_egl.h
 * Author: AWTK Develop Team
 * Brief:  linux egl lcd
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
 * History:
 * ================================================================
 * 2020-11-06 Lou ZhiMing <luozhiming@zlg.com> created
 *
 */

#ifndef TK_LCD_LINUX_EGL_H
#define TK_LCD_LINUX_EGL_H

#include "lcd_linux.h"
#include "base/types_def.h"

BEGIN_C_DECLS

typedef struct _lcd_egl_context_t {
  int32_t w;
  int32_t h;
  float_t ratio;
  void* elg_ctx;
  void* native_window;
} lcd_egl_context_t;

lcd_egl_context_t* lcd_linux_egl_create(const char* filename);

END_C_DECLS

#endif /*TK_LCD_LINUX_EGL_H*/

