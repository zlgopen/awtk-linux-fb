# linux-fb 如何使用 egl

## 1.配置默认支持的 egl 平台

​	在 linux-fb 中默认适配了三种平台的 egl，分别是飞思卡尔（fsl）平台和支持 x11 平台以及树莓派，如果是默认已经适配的平台，直接在 awtk_config.py 中开启相关的配置就可以使用，如下：

```python
# awtk_config.py

# lcd devices
LCD_DEICES='fb'
# LCD_DEICES='drm'
# LCD_DEICES='egl_for_fsl'
# LCD_DEICES='egl_for_x11'
# LCD_DEICES='egl_for_rpi'
```

###  2.适配其他 egl 平台

​		这里假设需要适配的平台名字为 AAA。

1. 在 egl_devices 目录下创建 AAA/egl_devices.c 文件重载实现 liunx-fb 的 egl 接口（egl_devices.h 文件），egl 接口（egl_devices.h 文件）如下：

   ```h
   /* egl_devices.h */
   #ifndef TK_EGL_DEVICES_H
   #define TK_EGL_DEVICES_H
   
   #include "base/types_def.h"
   
   BEGIN_C_DECLS
   
   void* egl_devices_create(const char* filename);
   ret_t egl_devices_dispose(void* ctx);
   
   float_t egl_devices_get_ratio(void* ctx);
   int32_t egl_devices_get_width(void* ctx);
   int32_t egl_devices_get_height(void* ctx);
   
   ret_t egl_devices_make_current(void* ctx);
   ret_t egl_devices_swap_buffers(void* ctx);
    
   END_C_DECLS
   
   #endif /*TK_EGL_DEVICES_H*/
   ```

2. 修改 SConscript 文件，加入对应平台的适配文件（AAA/egl_devices.c 文件），如下：

   ```python
   # SConscript
   
   if LCD_DEICES =='egl_for_fsl' :
     SOURCES = Glob('egl_devices/fsl/*.c') + SOURCES;
   elif LCD_DEICES =='egl_for_x11' :
     SOURCES = Glob('egl_devices/x11/*.c') + SOURCES;
   elif LCD_DEICES =='egl_for_rpi' :
     SOURCES = Glob('egl_devices/rpi/*.c') + SOURCES;
   elif LCD_DEICES =='egl_for_AAA' : # 这里加入适配文件代码
     SOURCES = Glob('egl_devices/AAA/*.c') + SOURCES;
   ```

3. 在 awtk_config.py 加入编译选项，如下：

   ```python
   # awtk_config.py
   ...
   
   # lcd devices
   # LCD_DEICES='fb'
   # LCD_DEICES='drm'
   # LCD_DEICES='egl_for_fsl'
   # LCD_DEICES='egl_for_x11'
   # LCD_DEICES='egl_for_rpi'
   LCD_DEICES='egl_for_AAA'	# 这里让 LCD_DEICES 等于 egl_for_AAA，因为 SConscript 中需要 LCD_DEICES 来确定编译文件。
   
   ...
   
   if LCD_DEICES =='drm' :
     #for drm
     OS_FLAGS=OS_FLAGS + ' -DWITH_LINUX_DRM=1 -I/usr/include/libdrm '
     OS_LIBS=OS_LIBS + ['drm']
   elif LCD_DEICES =='egl_for_fsl':
     #for egl for fsl
     OS_FLAGS=OS_FLAGS + ' -DEGL_API_FB '
     OS_LIBS=OS_LIBS + [ 'GLESv2', 'EGL']
   elif LCD_DEICES =='egl_for_x11' :
     #for egl for fsl
     OS_FLAGS=OS_FLAGS + ' -fPIC '
     OS_LIBS=OS_LIBS + [ 'X11', 'EGL', 'GLESv2' ]
   elif LCD_DEICES =='egl_for_rpi' :
     #for egl for rpi
     OS_LIBPATH += ['/opt/vc/lib']
     OS_CPPPATH += ['/opt/vc/include']
     OS_LIBS=OS_LIBS + [ 'brcmEGL', 'brcmGLESv2', 'bcm_host' ]
     COMMON_CCFLAGS += ' -DWITH_GLAD_SPECIAL_OPENGL_LIB=\\\"\"/opt/vc/lib/libbrcmGLESv2.so\\\"\" '
   elif LCD_DEICES =='egl_for_AAA':
     # 这里添加链接相关的宏和链接类库以及头文件路径。
     # OS_FLAGS, OS_LIBPATH, OS_LIBS, COMMON_CCFLAGS
   ```
   

> 其中 WITH_GLAD_SPECIAL_OPENGL_LIB 宏是定义特殊链接的 egl 类库，提供给 glad 获取 OpenGLES 的相关函数使用的，如果调用默认的 libGLESv2.so 的话，则可以不需要设置，如 egl_for_x11 和 egl_for_fsl。