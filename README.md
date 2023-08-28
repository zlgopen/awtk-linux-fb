# AWTK 针对 arm-linux 平台的移植。

[AWTK](https://github.com/zlgopen/awtk) 是为嵌入式系统开发的 GUI 引擎库。

[awtk-linux-fb](https://github.com/zlgopen/awtk-linux-fb) 是 AWTK 在 arm-linux 上的移植。

本项目以 [ZLG 周立功 linux 开发套件 AWork 平台 iMX287A 入门级 ARM9 开发板](https://item.taobao.com/item.htm?spm=a230r.1.14.1.29c8b3f8qxjYf7&id=536334628394&ns=1&abbucket=17#detail) 为载体移植，其它开发板可能要做些修改，有问题请请创建 issue。 

## 使用方法

#### 准备工作

* 1. 获取源码

> 以下两者并列放在同一个目录，如果用户有自己的项目，也建议与以下两者并列放在同一目录。

```
git clone https://github.com/zlgopen/awtk.git
git clone https://github.com/zlgopen/awtk-linux-fb.git
cd awtk-linux-fb
```

* 2. 配置 scons 交叉编译工具链，在 awtk-linux-fb 目录创建 awtk_config_define.py 文件

```
# awtk_config_define.py
TOOLS_PREFIX = "/opt/28x/gcc-4.4.4-glibc-2.11.1-multilib-1.0/arm-fsl-linux-gnueabi/bin/arm-linux-"
TSLIB_LIB_DIR = "/opt/28x/tslib/lib"
TSLIB_INC_DIR = "/opt/28x/tslib/include"
```

> 如果不需要 tslib，不定义 TSLIB\_LIB\_DIR 和 TSLIB\_INC\_DIR 即可。如：

```
# awtk_config_define.py
TSLIB_LIB_DIR = ""
TSLIB_INC_DIR = ""
```

> 详情可以看 scons help，或者可以通过 scons EXPORT_DEFINE_FILE=./awtk_config_define.py 导出一个完整的 awtk_config_define.py 文件。

* 3. 编辑 awtk-linux-fb 目录 config/devices.json 修改输入设备的文件名，该配置文件会随后续打包命令一起部署

```json
{
    "/dev/fb0" : {
        "type" : "fb"
    },
	"/dev/dri/card0" : {
        "type" : "drm"
    },
	"/dev/input/event0" : {
        "type" : "ts"
    },
	"/dev/input/event1" : {
        "type" : "input"
    },
	"/dev/input/mouse0" : {
        "type" : "mouse"
    }
}
```

可通过 "hexdump  /dev/input/xx" 命令识别正确的触摸或鼠标设备文件名。触摸设备也可以通过tslib自带的命令测试，如 "ts_test"、"ts_print"。

> 注意：在有些平台下，如果设置 "/dev/input/mice"，会出现触摸不灵的问题。通过 hexdump /dev/input/mice 命令发现，该设备文件会同时接收触摸和鼠标事件，这种情况请不要使用该设备。

#### 生成并部署AWTK内置DemoUI

1. 请先安装 scons，并完成上面的**准备工作**
2. 在命令行输入：

```bash
cd /home/user/awtk-linux-fb
scons
```

3. 等待编译成功，生成发布压缩包，在命令行输入：

```bash
sh ./release.sh
```

4. 等待发布完成后，在 awtk-linux-fb 目录下会出现 release.tar.gz 的压缩包，该压缩包就是发布包
5. 运行

把 release.tar.gz （发布包）上传到开发板，并解压，然后运行：

```bash
# 程序运行可能要依赖awtk的so文件，如运行失败请尝试设置so文件的绝对路径
# export LD_LIBRARY_PATH=/path/to/bin
sudo ./release/bin/demoui
```

#### 生成并部署用户自己的应用程序

1. 请先安装 scons，并完成上面的**准备工作**
2.  awtk 与用户的程序目录结构如下所示

```bash
/home/user/
	|-- awtk/
	|-- awtk-linux-fb/
	|-- user_apps/
```

输入命令行命令：

```bash
cd /home/user/awtk-linux-fb
scons APP=../user_apps/HelloDesigner-Demo
```

3.  等待编译成功，生成发布压缩包，在命令行输入：
```bash
./release.sh ../user_apps/HelloDesigner-Demo/res demo
```

4. 等待发布完成后，在 awtk-linux-fb 目录下会出现 release.tar.gz 的压缩包，该压缩包就是发布包
5. 运行

把 release.tar.gz （发布包）上传到开发板，并解压，然后运行：

```bash
# 程序运行可能要依赖awtk的so文件，如运行失败请尝试设置so文件的绝对路径
# export LD_LIBRARY_PATH=/path/to/bin
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
  |-- awtk-linux-fb
```

#### 2. 使用 Direct Rendering Manager (DRM)

缺省使用 framebuffer，如果使用 DRM，请修改 awtk_config_define.py，指定 LCD_DEVICES 和 drm 的路径。

```
LCD_DEVICES = 'drm'
```
> DRM 目前只在虚拟机中测试过，如果有问题请参考 awtk-port/lcd\_linux\_drm.c 进行调试。

#### 3. 使用 EGL 硬件加速

缺省使用 framebuffer，如果使用 EGL，请参考文档 [how_to_use_egl.md](docs/how_to_use_egl.md)。

#### 4. 上传文件到开发板的方法

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

