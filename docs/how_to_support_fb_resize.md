# 如何支持动态切换屏幕分辨率

​	动态切换屏幕分辨率，在 awtk 中是统一使用 window_manager_resize 函数来实现的。

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

​	但是在不同的平台上，切换屏幕分辨率的命令（或者代码）都不太一样，但是在 linux-fb-port 提供了默认的切换屏幕的方案。

​	当 LCD_DEICES 为 fb， drm，egl_for_x11，egl_for_gbm 的时候，当用户没有重载的话，会使用默认方案，但是这些方案都只在树莓派上面测试过，可能在其他平台上会出问题，尤其是 egl 和 drm 的，如果出问题的话，请看下面两个解决方案。

> 备注：fsl 还没有实现动态的方案，所以如果是 fsl 的话，需要手动修改 egl_devices/fsl/egl_devices.c 的 egl_devices_resize 和 egl_devices_get_default_resize_func 函数。

#### 一，替换默认切换屏幕命令。

​	用户直接可以通过调用 lcd_linux_set_fb_resize_func 函数来覆盖原来的动态切换分辨率的方案：

~~~h
/* lcd_linux/lcd_linux.h */
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
~~~

​	用户调用 lcd_linux_set_fb_resize 后，就可以覆盖原来的默认的切换屏幕分辨率的代码，有时候不单单只是切换屏幕的分辨率就好了，有可能需要修改的相关的配置，例如在 egl 上面修改了屏幕分辩率后还需要修改 egl 相关的配置，这个时候就只能使用下面一种方案了。

#### 二，直接重载 lcd 的 resize 函数

​	当覆盖切换屏幕命令不能解决问题后，用户需要自己手动重载 lcd 的 resize 函数。

```h
/* awtk/src/base/lcd.h */

typedef ret_t (*lcd_resize_t)(lcd_t* lcd, wh_t w, wh_t h, uint32_t line_length);
```

​	举个例子，打开 lcd_linux/lcd_linux_fb.c 文件：

~~~c
/* lcd_linux/lcd_linux_fb.c */
/*.....省略无关代码......*/

static ret_t (*lcd_mem_linux_resize_defalut)(lcd_t* lcd, wh_t w, wh_t h, uint32_t line_length);
static ret_t lcd_mem_linux_resize(lcd_t* lcd, wh_t w, wh_t h, uint32_t line_length) {
  ret_t ret = RET_OK;
  fb_info_t* fb = &s_fb;
  bool_t is_1fb = fb_is_1fb(fb);
  uint32_t fb_num = fb_number(fb);
  lcd_mem_t* mem = (lcd_mem_t*)lcd;
  return_value_if_fail(lcd != NULL && s_fb_resize_func != NULL, RET_BAD_PARAMS)
  
  if (lcd_mem_linux_resize_defalut != NULL) {
    lcd_mem_linux_resize_defalut(lcd, w, h, line_length);
  }

  lcd_linux_fb_close(fb);

  ret = s_fb_resize_func(fb_num, w, h, s_fb_resize_func_ctx);
  return_value_if_fail(ret == RET_OK, ret);
  return_value_if_fail(lcd_linux_fb_open(fb, s_filename), RET_FAIL);

  if (fb_is_1fb(fb)) {
    mem->online_fb = (uint8_t*)(fb->fbmem0);
  } else {
    s_buff_index = 0;
    TKMEM_FREE(mem->offline_fb);
  }
  mem->offline_fb = (uint8_t*)TKMEM_ALLOC(fb_size(fb));
  lcd_mem_set_line_length(lcd, fb_line_length(fb));
  if (fb_is_1fb(fb)) {
    lcd->impl_data = fb;
    fb->fbmem1 = mem->offline_fb;
  }

  log_debug("lcd_linux_fb_resize \r\n");
  return ret;
}

static lcd_t* lcd_linux_create_flushable(fb_info_t* fb) {
    /*.....省略无关代码......*/
    lcd_mem_linux_resize_defalut = lcd->resize;
    lcd->resize = lcd_mem_linux_resize;
    /*.....省略无关代码......*/
}
~~~

​	可以看到在 lcd 的 resize 的重载函数为 lcd_mem_linux_resize 函数，这样子就可以更加灵活的处理切换分辨率了。

