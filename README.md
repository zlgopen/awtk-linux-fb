# AWTK 针对 arm-linux 平台的移植。

[AWTK](https://github.com/zlgopen/awtk) 是为嵌入式系统开发的 GUI 引擎库。

[awtk-linux-fb](https://github.com/zlgopen/awtk-linux-fb) 是 AWTK 在 arm-linux 上的移植。

本项目以 [ZLG 周立功 linux 开发套件 AWork 平台 iMX287A 入门级 ARM9 开发板](https://item.taobao.com/item.htm?spm=a230r.1.14.1.29c8b3f8qxjYf7&id=536334628394&ns=1&abbucket=17#detail) 为载体移植，其它开发板可能要做些修改，有问题请请创建 issue。 

## 使用方法

* 1. 获取源码

> 以下三者并列放在同一个目录。

```
git clone https://github.com/zlgopen/awtk.git
git clone https://github.com/zlgopen/awtk-examples.git
git clone https://github.com/zlgopen/awtk-linux-fb.git
cd awtk-linux-fb
```

* 2. 编辑 awtk_config.py 设置工具链的路径

```
TSLIB_LIB_DIR='/opt/28x/tslib/lib'
TSLIB_INC_DIR='/opt/28x/tslib/include'
TOOLS_PREFIX='/opt/28x/gcc-4.4.4-glibc-2.11.1-multilib-1.0/arm-fsl-linux-gnueabi/bin/arm-linux-'
```

> 如果不需要 tslib，不定义 TSLIB\_LIB\_DIR 和 TSLIB\_INC\_DIR 即可。如：

```
#TOOLS_PREFIX=''
#TSLIB_LIB_DIR=''
```

* 3. 编辑 awtk-port/main\_loop\_linux.c 修改输入设备的文件名

```
#define FB_DEVICE_FILENAME "/dev/fb0"
#define TS_DEVICE_FILENAME "/dev/input/event0"
#define KB_DEVICE_FILENAME "/dev/input/event1"
#define MICE_DEVICE_FILENAME "/dev/input/mouse0"
```

> 注意：在有些平台下，如果设置 #define MICE_DEVICE_FILENAME "/dev/input/mice”，会出现触摸不灵的问题。通过 hexdump /dev/input/mice 命令发现，按下触摸屏或操作鼠标都会打印信息，即/dev/input/mice 会同时接收触摸和鼠标事件。可通过"hexdump  /dev/input/xx" 命令选择正确的鼠标设备文件名。

* 4. 编译（请先安装 scons)

生成内置 demoui 例子，生成结果在 build/bin 文件夹下的 demoui 文件

```
scons
```

也可以指定生成其他 Demo，生成结果在 build/bin 文件夹下的 demo 文件

```
scons APP=../awtk-examples/HelloWorld.Xml-Demo
```

有些 Demo 包含了两套不同 LCD 大小的资源，如：Chart-Demo

* 编译 Chart-Demo，并使用 LCD 为 800 * 480 的资源：

```
scons APP=../awtk-examples/Chart-Demo
```
* 编译 Chart-Demo，并使用 LCD 为 480 * 272 的资源：

```
scons APP=../awtk-examples/Chart-Demo LCD=480_272
```
* 5. 生成发布包

对于内置的 demoui 例子

```
sh ./release.sh
```

对于其他 Demo，需要加入资源文件夹参数，指向应用程序 assets 的父目录

```
sh ./release.sh ../awtk-examples/HelloWorld.Xml-Demo/res demo
```

* 6. 运行

把 release.tar.gz 上传到开发板，并解压，然后运行：

```
sudo ./release/bin/demoui
sudo ./release/bin/demo
```

## 文档

* [如何搭建 Ubuntu 调试环境](docs/how_to_use_in_vmware.md)

## 其他问题

#### 1. 项目路径

默认情况下，scons 脚本假设以下文件夹在同一个目录。

```
zlgopen
  |-- awtk
  |-- awtk-examples
  |-- awtk-linux-fb
```

#### 2. 使用 Direct Rendering Manager (DRM)

缺省使用 framebuffer，如果使用 DRM，请修改 awtk\_config.py，指定 LCD_DEICES 和 drm 的路径。

```
LCD_DEICES='drm'

#for drm
OS_FLAGS=OS_FLAGS + ' -DWITH_LINUX_DRM=1 -I/usr/include/libdrm '
OS_LIBS = OS_LIBS + ['drm']
```
> DRM 目前只在虚拟机中测试过，如果有问题请参考 wtk-port/lcd\_linux\_drm.c 进行调试。

#### 3. 使用 EGL 硬件加速

缺省使用 framebuffer，如果使用 EGL，请参考文档 [how_to_use_egl.md](docs/how_to_use_egl.md)。

#### 4. 编译输出 AWTK 动态链接库文件

请修改 awtk\_config.py，将 os.environ['WITH_AWTK_SO'] 标记为 true，则执行 scons 会同时输出 libawtk.so 等动态链接库文件。

```
os.environ['WITH_AWTK_SO'] = 'true'
```

#### 5. 上传文件到开发板的方法

如果开发板支持 ssh，可以使用 scp 命令上传文件或文件夹，比如上传文件：

```
# 开发板ip：192.168.1.136
# 登录用户名：root
# 上传文件 release.tar.gz 到 /opt 目录
scp release.tar.gz root@192.168.1.136:/opt/release.tar.gz
```

上传文件夹：

```
# 创建文件夹 ./release 中所有文件到 /opt/awtk 目录
scp -r ./release root@192.168.1.136:/opt/awtk
```

