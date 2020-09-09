# 如何搭建 Ubuntu 调试环境

awtk-linux-fb 是为嵌入式 linux 准备的，但为了调试方便，可以让 awtk-linux-fb 运行在 PC 机的 Ubuntu 环境中。需要让 Ubuntu 开机时不要启动图形桌面，而是启动普通的字符模式，目的是让 /dev/fb0 设备不被抢占，这样 awtk 程序才可以正常显示到 framebuffer 上。

## 操作步骤

##### 1. 在 grub 启动项增加字符模式启动

配置 Ubuntu 启动时显示 grub 菜单，先修改 **/etc/default/grub** 文件，把下面两行代码删除或注释掉：

```
#GRUB_HIDDEN_TIMEOUT=0
#GRUB_HIDDEN_TIMEOUT_QUIET=true
```

更新 grub 配置，在命令输入：

```
cd /etc/default
sudo update-grub
```

然后把字符模式启动项加入 grub 菜单，分辨率设置为 800x600，修改 **/etc/grub.d/40_custom** 文件，在文件末尾添加下面代码：

```
menuentry 'Ubuntu Console' --class ubuntu --class gnu-linux --class gnu --class os $menuentry_id_option 'gnulinux-simple-1c9bae2b-e598-4cb5-8bda-5b6ea039b13f' {
	recordfail
	load_video
	gfxmode $linux_gfx_mode
	insmod gzio
	if [ x$grub_platform = xxen ]; then insmod xzio; insmod lzopio; fi
	insmod part_msdos
	insmod ext2
	set root='hd0,msdos1'
	if [ x$feature_platform_search_hint = xy ]; then
	  search --no-floppy --fs-uuid --set=root --hint-bios=hd0,msdos1 --hint-efi=hd0,msdos1 --hint-baremetal=ahci0,msdos1  1c9bae2b-e598-4cb5-8bda-5b6ea039b13f
	else
	  search --no-floppy --fs-uuid --set=root 1c9bae2b-e598-4cb5-8bda-5b6ea039b13f
	fi
        linux	/boot/vmlinuz-4.15.0-62-generic root=UUID=1c9bae2b-e598-4cb5-8bda-5b6ea039b13f ro find_preseed=/preseed.cfg auto noprompt priority=critical locale=en_US quiet 3 vga=788
	initrd	/boot/initrd.img-4.15.0-62-generic
}
```

> 可以从 boot/grub/grub.cfg 中拷贝 menuentry 'Ubuntu' 的代码段作为模板，然后修改为 menuentry 'Ubuntu Console'，并找到 quiet 后面加入 3 vga=788

更新 grub 配置，在命令输入：

```
cd /etc/grub.d
sudo grub-mkconfig -o /boot/grub/grub.cfg
```

##### 2. 设置编译器

修改 **awtk-linux-fb/awtk_config.py** 使用 Ubuntu 自带的 GCC 编译器，把下面两行代码的注释去掉：

```
#for pc build
TOOLS_PREFIX=''
TSLIB_LIB_DIR=''
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