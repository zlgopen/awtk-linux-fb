# awtk-linux-fb 如何适配 wayland 平台

## 1.生成协议文件

在进行编译之前，需要先生成协议文件，这些文件会参与到项目的编译当中，使用 wayland-scanner 进行协议的生成，在使用前请保证 wayland-scanner 的版本与设备上的一致，使用以下命令查看版本：

```shell
wayland-scanner -v
```

比如在 M3568 的工具链中就自带了 wayland-scanner 工具，生成协议文件的命令如下（文件名请勿更改）：

```shell
cd awtk-linux-fb/awtk-wayland/protocol/
/opt/m3568-sdk-v1.0.0-ga/host/bin/wayland-scanner client-header xdg-shell.xml xdg-shell-protocol.h
/opt/m3568-sdk-v1.0.0-ga/host/bin/wayland-scanner private-code xdg-shell.xml xdg-shell-protocol.c
```

## 2.如何编译 awtk-wayland

在 awtk-linux-fb 下先参照 README.md 生成 awtk_config_define.py，配置好 TOOLS_PREFIX 和 LCD_DEVICES 等参数。

配置 awtk_config_define.py：

```python
TOOLS_PREFIX = "/opt/m3568-sdk-v1.0.0-ga/host/usr/bin/aarch64-linux-" # 按自己的板子配置交叉编译工具路径
# 设置LCD_DEVICES二选一
LCD_DEVICES = "wayland"           # 使用软件渲染
LCD_DEVICES = "egl_for_wayland"   # 使用OpenGL渲染
OS_LINKFLAGS = " -Wl,--copy-dt-needed-entries "  # 解决部分工具链的DSO missing错误
```

使用以下命令编译 awtk-linux-fb：

```shell
cd awtk-linux-fb
scons
```

如果不想使用 awtk_config_define.py，也可以在 scons 编译时直接指定参数：

```shell
cd awtk-linux-fb
#不使用 OpenGL
scons LCD_DEVICES='wayland'
#使用 OpenGL
scons LCD_DEVICES='egl_for_wayland'
```

若在编译时出现未找到动态库等错误，并根据实际情况添加 wayland 动态库和头文件的路径，请添加在 OS_LIBPATH 和 OS_CPPPATH 中。