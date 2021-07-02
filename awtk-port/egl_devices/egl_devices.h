/**
 * File:   egl_devices.h
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

#ifndef TK_EGL_DEVICES_H
#define TK_EGL_DEVICES_H

#include "base/types_def.h"
#include "../lcd_linux/lcd_linux.h"

BEGIN_C_DECLS

void* egl_devices_create(const char* filename);
ret_t egl_devices_dispose(void* ctx);

ret_t egl_devices_resize(void* ctx, uint32_t w, uint32_t h);

lcd_linux_fb_resize_func_t egl_devices_get_default_resize_func();

float_t egl_devices_get_ratio(void* ctx);
int32_t egl_devices_get_width(void* ctx);
int32_t egl_devices_get_height(void* ctx);

ret_t egl_devices_make_current(void* ctx);
ret_t egl_devices_swap_buffers(void* ctx);
 
END_C_DECLS

#endif /*TK_EGL_DEVICES_H*/
