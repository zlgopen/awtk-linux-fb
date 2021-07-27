# 如何使用 AWTK 静态/动态库以及自定义 main 函数

## 1 生成 AWTK 静态/动态库

生成 AWTK 静态/动态库 的步骤如下：

1. 确保 awtk 与 awtk-linux-fb 的目录结构如下：

```
xxxx
  |-- awtk
  |-- awtk-linux-fb
```

2. 参照 awtk-linux-fb/README.md 文档配置好交叉编译工具链与输入设备文件名。

3. 在 awtk-linux-fb 目录下打开终端，执行以下命令进行编译：

```
scons
```

AWTK 动态库放在 awtk-linux-fb/bin 目录，包含 libawtk.so 和 libtkc.so 两个文件；AWTK 静态库的功能比较零散，文件较多，都放在 awtk-linux-fb/lib 目录下，此处就不逐一列出了。

## 2 使用 gcc 编译项目源文件

使用 AWTK 静态库或动态库通常需要用户自己编译项目，这里列举出编译所需的宏定义与包含路径，方便用户参考。

### 2.1 宏定义

| 宏定义                        | 用途                               | 是否必须 | 缺省值  | 备注                                             |
| ----------------------------- | ---------------------------------- | -------- | ------- | ------------------------------------------------ |
| LCD_WIDTH=[number]            | LCD 的宽度                         | 否       | 320     |                                                  |
| LCD_HEIGHT=[number]           | LCD 的高度                         | 否       | 480     |                                                  |
| APP_DEFAULT_FONT=[string]     | 项目默认使用的字库名称             | 否       | default |                                                  |
| APP_THEME=[string]            | 项目默认使用的主题名称             | 否       | default |                                                  |
| APP_RES_ROOT=[string]         | 项目的资源根目录                   | 否       | NULL    | 缺省按照相对路径搜索                             |
| APP_DEFAULT_LANGUAGE=[string] | 项目默认使用的语言                 | 否       |         | 缺省语言环境为 en，即英文，常用值还有 zh，即中文 |
| APP_DEFAULT_COUNTRY=[string]  | 项目默认使用的国家                 | 否       |         | 缺省语言环境为 US，即美国，常用值还有 CN，即中国 |
| APP_ROOT=[string]             | 项目路径                           | 否       |         | 缺省按照相对路径搜索                             |
| **HAS_STDIO**                 | **存在标准输入输出**               | **是**   |         |                                                  |
| **LINUX**                     | **Linux 平台**                     | **是**   |         |                                                  |
| **LOAD_ASSET_WITH_MMAP**      | **支持内存映射文件**               | **是**   |         | **与宏 WITH_FS_RES 一起使用**                    |
| **WITH_FS_RES**               | **支持文件系统**                   | **是**   |         |                                                  |
| **WITH_WIDGET_TYPE_CHECK**    | **启用 widget 类型检查**           | **是**   |         |                                                  |
| WITH_VGCANVAS                 | 支持矢量画布                       | 否       |         |                                                  |
| WITH_SOCKET                   | 支持套接字                         | 否       |         |                                                  |
| WITH_STB_IMAGE                | 支持 png/jpeg 图片                 | 否       |         |                                                  |
| WITH_STB_FONT                 | 用 stb 支持 Truetype 字体          | 否       |         |                                                  |
| WITH_TEXT_BIDI                | 支持文字双向排版算法(如阿拉伯语言) | 否       |         |                                                  |
| ENABLE_CURSOR                 | 启用鼠标指针                       | 否       |         | 资源中需要有指针（cursor）图片                   |

### 2.2 包含路径

| 包含路径             | 说明              |
| -------------------- | ----------------- |
| awtk                 | awtk 根目录       |
| awtk/src             | awtk 源码目录     |
| awtk/src/ext_widgets | awtk 扩展控件目录 |
| %app%src             | 项目源码目录      |
| %app%res             | 项目资源目录      |


### 3.2 gcc 示例命令

此处，以为一个简单的项目源文件 app_main.c 为例，编译该文件的 gcc 命令如下，项目其他源文件的编译命令类似：

```bash
gcc -o src/app_main.o -c -std=gnu99 -DLCD_WIDTH=480 -DLCD_HEIGHT=272 -Wall -Os -DHAS_STDIO -DLINUX -DLOAD_ASSET_WITH_MMAP -DWITH_FS_RES -DWITH_WIDGET_TYPE_CHECK -fPIC -I/home/user/zlgopen/awtk -I/home/user/zlgopen/awtk/src -I/home/user/zlgopen/awtk/src/ext_widgets -Isrc -Ires src/app_main.c
```

## 3 链接静态库

编译好项目源文件后，只需将生成的 .o 文件与 AWTK 静态库链接生成可执行程序即可，链接时需要包含 awtk-linux-fb/lib 目录并链接该目录下的所有静态库，例如，此处链接生成可执行程序 bin/demo，指令如下：

```bash
gcc -o bin/demo -Wl,-rpath=./bin -Wl,-rpath=./ -Wl,-rpath=/home/user/zlgopen/AwtkApplication/bin src/app_main.o src/window_main.o -Llib -L/home/user/zlgopen/awtk-linux-fb/lib -lawtk_global -lextwidgets -lwidgets -lawtk_linux_fb -lbase -lgpinyin -lstreams -lconf_io -lhal -lcsv -lcompressors -lminiz -lubjson -ltkc_static -llinebreak -lmbedtls -lfribidi -lnanovg-agge -lagge -lnanovg -lstdc++ -lpthread -lrt -lm -ldl
```

> 备注：其中 app_main.o 和 window_main.o 为项目源文件的中间文件。

## 4 链接动态库

链接动态库的命令与链接静态库的命令相似，修改包含路径和库文件名即可，指令如下：

```bash
gcc -o bin/demo -Wl,-rpath=./bin -Wl,-rpath=./ -Wl,-rpath=/home/user/zlgopen/AwtkApplication/bin src/app_main.o src/window_main.o -L/home/user/zlgopen/awtk-linux-fb/bin -lawtk -ltkc -lstdc++ -lpthread -lrt -lm -ldl
```

> 备注：其中 app_main.o 和 window_main.o 为项目源文件的中间文件。

## 5 自定义 main 函数

AWTK 支持用户自定义 main 函数，通常只需在包含 awtk_main.inc 文件前，定义宏 USE_GUI_MAIN，然后在自定义的 main 函数中调用 gui_app_start 函数即可。

### 5.1 示例

例如，使用 Designer 新建一个 AWTK 项目，该项目的 app_main.c 文件中默认使用 AWTK 内置的 main 函数，代码如下，内置 main 函数的实现详见 awtk/src/awtk_main.inc。

```c
#include "awtk.h"

...

extern ret_t application_init(void);

extern ret_t application_exit(void);

#include "awtk_main.inc"
```

此时，在包含 awtk_main.inc 文件前定义宏 USE_GUI_MAIN，并在自定义 main 函数中调用gui_app_start 函数初始化 AWTK 并启动 GUI 线程（主循环），代码如下：
```c
#include "awtk.h"

...

/**
 * 嵌入式系统有自己的main函数时，请定义本宏。
 */
#define USE_GUI_MAIN 1

extern ret_t application_init(void);

extern ret_t application_exit(void);

#include "awtk_main.inc"

/**
 * 自定义 main 函数
 * 备注：此处将 main 函数放在 awtk_main.inc 文件后是为了声明 gui_app_start 函数。
 */
int main(void) {
  return gui_app_start(LCD_WIDTH, LCD_HEIGHT); /* 需要指定 LCD 的宽高 */
}
```
