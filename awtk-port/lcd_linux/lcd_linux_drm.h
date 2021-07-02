/**
 * File:   lcd_linux_drm.h
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
 * History:
 * ================================================================
 * 2020-05-16 Li XianJing <xianjimli@hotmail.com> created
 *
 */

#ifndef TK_LCD_LINUX_DRM_H
#define TK_LCD_LINUX_DRM_H

#include "base/lcd.h"
#include "lcd_linux.h"

BEGIN_C_DECLS

lcd_t* lcd_linux_drm_create(const char* card);

END_C_DECLS

#endif /*TK_LCD_LINUX_DRM_H*/
