/**
 * File:   lcd_mem_others.h
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

#ifndef LCD_MEM_OTHERS_H
#define LCD_MEM_OTHERS_H

#include "fb_info.h"
#include "lcd/lcd_mem_special.h"

BEGIN_C_DECLS

lcd_t* lcd_mem_bgra5551_create(fb_info_t* fb);

END_C_DECLS

#endif /*LCD_MEM_OTHERS_H*/
