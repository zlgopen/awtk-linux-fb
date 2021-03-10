# 如何搭建 Ubuntu 调试环境

awtk-linux-fb 是为嵌入式 linux 准备的，但为了调试方便，可以让 awtk-linux-fb 运行在 PC 机的 Ubuntu 环境中。需要让 Ubuntu 开机时不要启动图形桌面，而是启动普通的字符模式，目的是让 /dev/fb0 设备不被抢占，这样 awtk 程序才可以正常显示到 framebuffer 上。

## 操作步骤

##### 1. 在命令行执行 systemctl 命令切换图形和字符模式

如果当前在图形桌面模式，执行下面的命令重启后进入字符模式：

```
sudo systemctl set-default multi-user.target
sudo reboot
```

如果当前在字符模式，执行下面的命令重启后进入桌面模式：

```
sudo systemctl set-default graphical.target
sudo reboot
```

##### 2. 设置编译器

修改 **awtk-linux-fb/awtk_config.py** 使用 Ubuntu 自带的 GCC 编译器，把下面两行代码的注释去掉：

```
#for pc build
TOOLS_PREFIX=''
TSLIB_LIB_DIR=''
```

也可以在该配置文件中选择使用 fb 模式或 drm 模式，新的系统建议使用 drm 模式，效率更高：

```
# lcd devices
LCD_DEICES='fb'
# LCD_DEICES='drm'
```

##### 3. 编译 demo 并抽取资源文件

确保 awtk 和 awtk-linux-fb 在同一级目录

```
cd awtk-linux-fb
scons
sh release.sh
```

##### 4. 运行 demo

需要在管理员模式运行 demo 程序，运行前请确保当前 Ubuntu 是以字符模式启动

```
cd awtk-linux-fb
sudo release/bin/demoui
```

## 配置 VMware 显存

如果 Ubuntu 是安装在 VMware 虚拟机中，默认只能使用 awtk-linux-fb 单缓冲模式，因为 VMware 虚拟机默认 svga 显存大小是 4MB，不够分配 awtk 使用的双缓冲显存。如需要调试双缓冲模式，则要将虚拟机的 svga 显存调整为 8MB。

#### 调整方法

打开 **Ubuntu 64 位.vmx** 文件修改，加入下面两行配置：

```
svga.minVRAMSize=8388608
svga.minVRAM8MB=TRUE
```

#### 调整显存前

fb 分辨率是 800x600：yres=600，调整前 yres_virtual==885，yres_virtual < yres*2 不满足双缓冲的最低要求，fbset 命令显示如下：

```
mode "800x600"
    geometry 800 600 1176 885 32
    timings 0 0 0 0 0 0 0
    rgba 8/16,8/8,8/0,0/0
endmode

Frame buffer device information:
    Name        : svgadrmfb
    Address     : 0
    Size        : 4163040
    Type        : PACKED PIXELS
    Visual      : TRUECOLOR
    XPanStep    : 1
    YPanStep    : 1
    YWrapStep   : 0
    LineLength  : 4704
    Accelerator : No
```

#### 调整显存后

调整后 yres_virtual==1254，yres_virtual >= yres*2 满足双缓冲要求，fbset 命令显示如下：

```
mode "800x600"
    geometry 800 600 1672 1254 32
    timings 0 0 0 0 0 0 0
    rgba 8/16,8/8,8/0,0/0
endmode

Frame buffer device information:
    Name        : svgadrmfb
    Address     : 0
    Size        : 8386752
    Type        : PACKED PIXELS
    Visual      : TRUECOLOR
    XPanStep    : 1
    YPanStep    : 1
    YWrapStep   : 0
    LineLength  : 6688
    Accelerator : No
```

> 双缓冲判断请参考 awtk-port/fb_info.h 中的代码

#### 修改屏幕分辨率的方法

比如要调整屏幕为 800x480x32bit，虚拟分辨率为 800x960（双缓冲）时，可以用 fbset 命令进行设置：

```
sudo fbset -g 800 480 800 960 32
```

> 这种方法同样适用于真正的嵌入式 Linux 板子设备