
#ifndef TK_LCD_LINUX_H
#define TK_LCD_LINUX_H

#include "base/types_def.h"

typedef ret_t (*lcd_linux_fb_resize_func_t)(uint32_t fb_num, wh_t w, wh_t h, void* ctx);

/**
 * @method lcd_linux_set_fb_resize_func
 * 设置平台相关的修改分辨率的回调函数
 *
 * @annotation ["scriptable"]
 * @param {lcd_linux_fb_resize_func_t} fb_resize_func 修改分辨率的回调函数。
 * @param {void*} ctx 上下文。
 *
 * @return {ret_t} 返回RET_OK表示成功，否则表示失败。
 */
ret_t lcd_linux_set_fb_resize_func(lcd_linux_fb_resize_func_t fb_resize_func, void* ctx);


#endif
