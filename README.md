


# AWTK针对arm-linux平台的移植。

[AWTK](https://github.com/zlgopen/awtk)是为嵌入式系统开发的GUI引擎库。

[awtk-linux-fb](https://github.com/zlgopen/awtk-linux-fb)是AWTK在arm-linux上的移植。

本项目以[ZLG周立功 linux开发套件 AWork平台iMX287A 入门级ARM9开发板](https://item.taobao.com/item.htm?spm=a230r.1.14.1.29c8b3f8qxjYf7&id=536334628394&ns=1&abbucket=17#detail) 为载体移植，其它开发板可能要做些修改，有问题请请创建issue。 

## 使用方法

* 1.获取源码

```
git clone https://github.com/zlgopen/awtk-linux-fb.git
cd awtk-linux-fb
git clone https://github.com/zlgopen/awtk.git

```

* 2.编辑SConstruct设置工具链的路径

```
TSLIB_LIB_DIR='/opt/28x/tslib/lib'
TSLIB_INC_DIR='/opt/28x/tslib/include'
TOOLS_PREFIX='/opt/28x/gcc-4.4.4-glibc-2.11.1-multilib-1.0/arm-fsl-linux-gnueabi/bin/arm-linux-'
```

* 3.编辑awtk-port/main\_loop\_linux.c修改输入设备的文件名

```
#define FB_DEVICE_FILENAME "/dev/fb0"
#define TS_DEVICE_FILENAME "/dev/input/event0"
#define KB_DEVICE_FILENAME "/dev/input/event1"
```

* 4.编译(请先安装scons)

```
scons
```

* 5.生成发布包

```
./release.sh
```

* 6.运行

把release.zip上传到开发板，并解压，然后运行：

```
./release/bin/demoui
```



