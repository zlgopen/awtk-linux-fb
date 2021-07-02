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

#ifndef TK_LCD_LINUX_FB_H
#define TK_LCD_LINUX_FB_H

#include "lcd_linux.h"

BEGIN_C_DECLS

lcd_t* lcd_linux_fb_create(const char* filename);

END_C_DECLS

#endif /*TK_LCD_LINUX_FB_H*/
