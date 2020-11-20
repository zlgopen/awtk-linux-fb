# 在树莓派中使用awtk的方法(fb&egl)

### FrameBuffer 模式

#### awtk-linux-fb

> sha-1：73da018ac8e8fba21e0d146d2ba5320b035214d7

该文件夹是 awtk 的 linux fb 适配层，用于编译 awtk，这个方法最简单：

1. 确保系统没有启动X或桌面，可以在 raspi-config 中配置系统启动时只进入命令行 CLI

   ```
   sudo raspi-config
   ```

2. 修改 awtk-linux-fb/awtk_config.py 文件的编译器和编译选项

   ```
   OS_FLAGS='-g -Wall -Os -mfloat-abi=hard '
   #for pc build
   TOOLS_PREFIX=''
   TSLIB_LIB_DIR=''
   ```

3. 直接在树莓派中编译 awtk 并运行

   ```
   cd awtk-linux-fb
   scons
   sh release.sh
   release/bin/demoui
   ```

<div STYLE="page-break-after: always;"></div>

### EGL-X11 模式

#### 准备工作(配置 EGL 环境)

1. 修改 raspi-config 开启 GL 加速

   ```
   sudo raspi-config
   修改 GPU 内存分配 128MB 以上
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

#### awtk-linux-egl

> sha-1：73da018ac8e8fba21e0d146d2ba5320b035214d7

该文件夹与 awtk-linux-fb 类似，用于适配编译 awtk，要修改 awtk-linux-fb 的如下几个文件来适配，比较复杂，具体参考 awtk-linux-egl 文件夹
- awtk_config.py
- SConstruct
- lcd_linux_fb.c
- lcd_linux_fb.h
- main_loop_linux.c

使用方法：

```
先按上面步骤配置好EGL环境,然后直接在树莓派中编译运行
cd awtk-linux-egl
scons
sh release.sh
release/bin/demoui
```

#### egl_test

该程序用于演示如何进入 EGL 模式，使用方法：

```
先按上面步骤配置好EGL环境,然后直接在树莓派中编译运行
cd egl_test
sh ccegl.sh   # 编译
sh runegl.sh  # 运行
```

#### nanovg_test

该程序演示官方 nanovg 与 EGL 对接的方法，并可以测试帧率，使用方法：

```
先按上面步骤配置好EGL环境,然后直接在树莓派中编译运行
cd nanovg_test/nanovg
sh ccnanovg.sh # 编译
bin/a.out      # 运行
```

