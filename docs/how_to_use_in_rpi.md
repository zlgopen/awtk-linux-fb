# 在树莓派中使用awtk的方法(fb&egl)

### FrameBuffer 模式

1. 确保系统没有启动X或桌面，可以在 raspi-config 中配置系统启动时只进入命令行 CLI

   ```
   sudo raspi-config
   ```

2. 修改 awtk-linux-fb/awtk_config.py 文件的编译器和编译选项

   ```
   #for pc build
   TOOLS_PREFIX=''
   TSLIB_LIB_DIR=''
   ```

3. 直接在树莓派中编译 awtk 并运行

   ```
   cd awtk-linux-fb
   scons
   sh release.sh
   sudo release/bin/demoui
   ```

### EGL-X11 模式

1. 修改 raspi-config 开启 GL 加速

   ```
   sudo raspi-config
   开启 GL full KMS 驱动
   ```

2. 进入桌面或 X

   ```
   sudo X &
   ```

3. 修改环境变量指定 EGL 首选的显示设备

   ```
   export DISPLAY=:0.0
   ```

4. 安装编译依赖库

   ```
   sudo apt install libx11-dev
   sudo apt install libgles2-mesa libgles2-mesa-dev
   ```

5. 修改 awtk-linux-fb/awtk_config.py 文件的 LCD_DEICES 模式

   ```
   # lcd devices
   LCD_DEICES='egl_for_x11'
   
   #for pc build
   TOOLS_PREFIX=''
   TSLIB_LIB_DIR=''
   ```

6. 直接在树莓派中编译 awtk 并运行

   ```
   cd awtk-linux-fb
   scons
   sh release.sh
   sudo release/bin/demoui
   ```

### EGL-gbm 模式

1. 修改 raspi-config 开启 GL 加速

   ```
   sudo raspi-config
   开启 GL full KMS 驱动
   ```

2. 修改 awtk-linux-fb/awtk_config.py 文件的 LCD_DEICES 模式

   ```
   # lcd devices
   LCD_DEICES='egl_for_gbm'
   
   #for pc build
   TOOLS_PREFIX=''
   TSLIB_LIB_DIR=''
   ```

3. 直接在树莓派中编译 awtk 并运行

   ```
   cd awtk-linux-fb
   scons
   sh release.sh
   sudo release/bin/demoui
   ```
