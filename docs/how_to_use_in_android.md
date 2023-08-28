# 如何搭建 Android 调试环境

awtk-linux-fb 是为嵌入式 linux 准备的，但为了调试方便，可以让 awtk-linux-fb 运行在手机 Android 环境中。需要让 Android 不要启动图形桌面，这样 awtk 程序才可以正常显示到 framebuffer 上。

所有操作步骤在 PC Ubuntu 中进行。

## 操作步骤

##### 一、准备工作

1. 下载交叉编译工具链 android-ndk-r21e-linux-x86_64.zip

2. 安装 adb 调试工具

   ```
   sudo apt install adb android-tools-adb
   ```

##### 二、用 NDK 交叉编译 awtk-linux-fb

1. 解压交叉编译工具链 android-ndk-r21e-linux-x86_64.zip 到 /opt

2. scons 参数的可以配置交叉编译工具链，并设置正确的编译器路径

   ```
   scons TOOLS_PREFIX="/opt/android-ndk-r21e/toolchains/llvm/prebuilt/linux-x86_64/bin/" TOOLS_CC="armv7a-linux-androideabi16-clang" TOOLS_CXX="armv7a-linux-androideabi16-clang++" TOOLS_LD="arm-linux-androideabi-ld" TOOLS_AR="arm-linux-androideabi-ar" TOOLS_STRIP="arm-linux-androideabi-strip" TOOLS_RANLIB="arm-linux-androideabi-ranlib"
   ```

3. 编译 awtk 源码并部署到 release 文件夹

   ```
   cd awtk-linux-fb
   scons && sh release.sh
   ```

4. 拷贝 libc++_shared.so 动态库

   由于编译选项默认使用动态链接方式使用标准C++库，所以需要把相关的 so 文件也上传到 Android 设备。

   ```
   cp /opt/android-ndk-r21e/toolchains/llvm/prebuilt/linux-x86_64/sysroot/usr/lib/arm-linux-androideabi/libc++_shared.so ./release/bin
   ```

##### 三、连接 Android 设备

1. 手机启动开发者模式，把手机通过 USB 线连接到 PC

2. 把编译好的bin和资源文件上传到手机

   ```
   cd awtk-linux-fb
   adb push ./release /data/local/tmp
   ```

##### 四、进入手机 shell 环境操作

1. 进入手机 shell 并配置运行环境

   ```
   adb shell
   su
   chmod 777 /data/local/tmp/bin/demoui
   # 设置so文件搜索路径
   export LD_LIBRARY_PATH=$LD_LIBRARY:/data/local/tmp/bin
   ```

2. 关闭 Android 桌面准备运行程序

   ```
   # 关闭桌面
   setprop ctl.stop zygote
   setprop ctl.stop bootanim
   # 如果运行过程出现黑屏，可能是系统的其他进程在抢占 fb，用下面的命令关闭冲突的进程
   setprop ctl.stop servicemanager
   setprop ctl.start servicemanager
   # 运行程序
   /data/local/tmp/bin/demoui
   ```