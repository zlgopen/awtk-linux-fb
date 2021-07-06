# 如何支持动态切换屏幕分辨率

#### 应用程序使用说明

在 AWTK 应用中，用户可以简单的调用 window_manager_resize 函数实现分辨率切换。

~~~h
/**
 * @method window_manager_resize
 * 调整原生窗口的大小。
 * @annotation ["scriptable"]
 * @param {widget_t*} widget 窗口管理器对象。
 * @param {wh_t}   w 宽度
 * @param {wh_t}   h 高度
 *
 * @return {ret_t} 返回RET_OK表示成功，否则表示失败。
 */
ret_t window_manager_resize(widget_t* widget, wh_t w, wh_t h);
~~~

#### 适配层实现方法

通常开发者在适配层创建 LCD 对象时，需要绑定 lcd->resize 回调函数，在这个回调中实现分辨率切换的系统调用。

```
static lcd_t* lcd_linux_create_flushable(fb_info_t* fb) {
    /*.....省略无关代码......*/
    lcd = lcd_mem_bgr565_create_double_fb(w, h, online_fb, offline_fb);
    lcd_mem_linux_resize_defalut = lcd->resize;
    lcd->resize = lcd_mem_linux_resize;
    /*.....省略无关代码......*/
}
```

awtk-linux-fb 提供了多个默认的切换分辨率的方案，比如当 LCD_DEVICES = fb 时，相关代码在 lcd_linux_fb.c 文件的 lcd_mem_linux_resize() 函数中：

```
static ret_t lcd_mem_linux_resize(lcd_t* lcd, wh_t w, wh_t h, uint32_t line_length) {
  ret_t ret = RET_OK;
  fb_info_t* fb = &s_fb;
  lcd_mem_t* mem = (lcd_mem_t*)lcd;
  return_value_if_fail(lcd != NULL, RET_BAD_PARAMS);

  ret = fb_resize_reopen(fb, w, h);
  mem->online_fb = (uint8_t*)(fb->fbmem0);
  mem->offline_fb = fb->fbmem_offline;
  lcd_mem_set_line_length(lcd, fb_line_length(fb));

  if (lcd_mem_linux_resize_defalut && ret == RET_OK) {
    lcd_mem_linux_resize_defalut(lcd, w, h, line_length);
  }

  log_debug("lcd_linux_fb_resize \r\n");
  return ret;
}
```

其中，fb_resize_reopen() 实现了分辨率切换的系统调用，其他代码为对接 AWTK 环境的上下文。如果目标平台 API 比较特殊，使用默认的方案无法实现分辨率切换，请自行修改该函数。

> 备注：LCD_DEVICES 的类型在 awtk_config.py 中设置

