import os
import platform

OS_NAME = platform.system()

def joinPath(root, subdir):
  return os.path.normpath(os.path.join(root, subdir))

CWD=os.path.normpath(os.path.abspath(os.path.dirname(__file__)));

TK_LINUX_FB_ROOT = CWD
TK_ROOT          = joinPath(TK_LINUX_FB_ROOT, '../awtk')
TK_SRC           = joinPath(TK_ROOT, 'src')
TK_3RD_ROOT      = joinPath(TK_ROOT, '3rd')
GTEST_ROOT       = joinPath(TK_ROOT, '3rd/gtest/googletest')

BUILD_DIR        = joinPath(TK_LINUX_FB_ROOT, 'build')
BIN_DIR          = joinPath(BUILD_DIR, 'bin')
LIB_DIR          = joinPath(BUILD_DIR, 'lib')
VAR_DIR          = joinPath(BUILD_DIR, 'var')
TK_DEMO_ROOT     = joinPath(TK_ROOT, 'demos')

LCD='LINUX_FB'
NANOVG_BACKEND='AGGE'

#INPUT_ENGINE='null'
#INPUT_ENGINE='spinyin'
#INPUT_ENGINE='t9'
#INPUT_ENGINE='t9ext'
INPUT_ENGINE='pinyin'

COMMON_CCFLAGS=' -DHAS_STD_MALLOC -DHAS_STDIO -DWITH_VGCANVAS -DWITH_UNICODE_BREAK -DLINUX'
COMMON_CCFLAGS=COMMON_CCFLAGS+' -DWITH_ASSET_LOADER -DWITH_FS_RES ' 
COMMON_CCFLAGS=COMMON_CCFLAGS+' -DSTBTT_STATIC -DSTB_IMAGE_STATIC -DWITH_STB_IMAGE -DWITH_STB_FONT  -DWITH_TEXT_BIDI=1 '
COMMON_CCFLAGS=COMMON_CCFLAGS+' -DWITH_NANOVG_AGGE -DWITH_WIDGET_TYPE_CHECK'

if INPUT_ENGINE == 't9':
    COMMON_CCFLAGS = COMMON_CCFLAGS + ' -DWITH_IME_T9 '
elif INPUT_ENGINE == 't9ext' :
    COMMON_CCFLAGS = COMMON_CCFLAGS + ' -DWITH_IME_T9EXT'
elif INPUT_ENGINE == 'pinyin' :
    COMMON_CCFLAGS = COMMON_CCFLAGS + ' -DWITH_IME_PINYIN '
elif INPUT_ENGINE == 'spinyin' :
    COMMON_CCFLAGS = COMMON_CCFLAGS + ' -DWITH_IME_SPINYIN '
elif INPUT_ENGINE == 'null' :
    COMMON_CCFLAGS = COMMON_CCFLAGS + ' -DWITH_IME_NULL '

GRAPHIC_BUFFER='default'
#GRAPHIC_BUFFER='jzgpu'
#if GRAPHIC_BUFFER == 'jzgpu':
#  COMMON_CCFLAGS=COMMON_CCFLAGS+' -DWITH_JZGPU'

#only for c compiler flags
COMMON_CFLAGS=''
COMMON_CFLAGS=COMMON_CFLAGS+' -std=gnu99 '

OS_LIBS=[]
OS_LIBPATH=[]
OS_CPPPATH=[]
OS_LINKFLAGS=''
OS_SUBSYSTEM_CONSOLE=''
OS_SUBSYSTEM_WINDOWS=''
OS_FLAGS='-Wall -Os '
#OS_FLAGS='-g -Wall -Os -mfloat-abi=hard '

#for build tslib
#TSLIB_INC_DIR=joinPath(TK_LINUX_FB_ROOT, '3rd/tslib/src')
#TSLIB_LIB_DIR=joinPath(TK_LINUX_FB_ROOT, '3rd/tslib/src/.libs')

#for prebuild tslib
TSLIB_LIB_DIR='/opt/28x/tslib/lib'
TSLIB_INC_DIR='/opt/28x/tslib/include'
TOOLS_PREFIX='/opt/28x/gcc-4.4.4-glibc-2.11.1-multilib-1.0/arm-fsl-linux-gnueabi/bin/arm-linux-'
#TOOLS_PREFIX='/opt/poky/1.7/sysroots/x86_64-pokysdk-linux/usr/bin/arm-poky-linux-gnueabi/arm-poky-linux-gnueabi-'

#for pc build
#TOOLS_PREFIX=''
#TSLIB_LIB_DIR=''
TARGET_ARCH = platform.architecture();

CC=TOOLS_PREFIX+'gcc',
CXX=TOOLS_PREFIX+'g++',
LD=TOOLS_PREFIX+'g++',
AR=TOOLS_PREFIX+'ar',
RANLIB=TOOLS_PREFIX+'ranlib',
STRIP=TOOLS_PREFIX+'strip',
OS_LIBS = ['stdc++', 'pthread', 'rt', 'm', 'dl']

#for android
#TSLIB_LIB_DIR=''
#TSLIB_INC_DIR=''
#TOOLS_PREFIX='/opt/android-ndk-r20b/toolchains/llvm/prebuilt/linux-x86_64/bin/'
#TOOLS_PREFIX='/Users/jim/android/android-ndk-r21d/toolchains/llvm/prebuilt/darwin-x86_64/bin/'
#CC=TOOLS_PREFIX+'armv7a-linux-androideabi16-clang'
#CXX=TOOLS_PREFIX+'armv7a-linux-androideabi16-clang++'
#LD=TOOLS_PREFIX+'arm-linux-androideabi-ld'
#AR=TOOLS_PREFIX+'arm-linux-androideabi-ar'
#STRIP=TOOLS_PREFIX+'arm-linux-androideabi-strip'
#RANLIB=TOOLS_PREFIX+"arm-linux-androideabi-ranlib"
#OS_LINKFLAGS='-static -Wl,--allow-multiple-definition '
#OS_LIBS = ['stdc++', 'm']
#OS_FLAGS='-Wall -Os -DFB_DEVICE_FILENAME=\\\"\"/dev/graphics/fb0\\\"\" '

#for drm
#OS_FLAGS=OS_FLAGS + ' -DWITH_LINUX_DRM=1 -I/usr/include/libdrm '
#OS_LIBS = OS_LIBS + ['drm']

COMMON_CCFLAGS = COMMON_CCFLAGS + ' -DLINUX -DHAS_PTHREAD -DENABLE_CURSOR -fPIC '
COMMON_CCFLAGS=COMMON_CCFLAGS+' -DWITH_DATA_READER_WRITER=1 '
COMMON_CCFLAGS=COMMON_CCFLAGS+' -DWITH_EVENT_RECORDER_PLAYER=1 '
COMMON_CCFLAGS = COMMON_CCFLAGS + ' -DWITH_WIDGET_TYPE_CHECK=1 '

if TSLIB_LIB_DIR != '':
  COMMON_CCFLAGS = COMMON_CCFLAGS + ' -DHAS_TSLIB '

CFLAGS=COMMON_CFLAGS
LINKFLAGS=OS_LINKFLAGS;
LIBPATH=[LIB_DIR, BIN_DIR] + OS_LIBPATH
CCFLAGS=OS_FLAGS + COMMON_CCFLAGS 

STATIC_LIBS =['awtk_global', 'extwidgets', 'widgets', 'awtk_linux_fb', 'base', 'gpinyin', 'streams', 'conf_io', 'compressors', 'miniz', 'ubjson', 'tkc_static', 'nanovg-agge', 'agge', 'nanovg', 'linebreak', 'fribidi'] + OS_LIBS
if TSLIB_LIB_DIR != '':
  SHARED_LIBS=['awtk', 'ts'] + OS_LIBS;
else:
  SHARED_LIBS=['awtk'] + OS_LIBS;

AWTK_DLL_DEPS_LIBS = ['nanovg-agge', 'agge', 'nanovg'] + OS_LIBS
OS_WHOLE_ARCHIVE =' -Wl,--whole-archive -lfribidi -lawtk_global -lextwidgets -lwidgets -lawtk_linux_fb -lbase -lgpinyin -ltkc_static -lstreams -lconf_io -lubjson -lcompressors -lminiz -llinebreak -Wl,--no-whole-archive'

LIBS=STATIC_LIBS

CPPPATH=[TK_ROOT, 
  TK_SRC, 
  TK_3RD_ROOT, 
  joinPath(TK_SRC, 'ext_widgets'), 
  joinPath(TK_ROOT, 'tools'), 
  joinPath(TK_3RD_ROOT, 'agge'), 
  joinPath(TK_3RD_ROOT, 'agg/include'), 
  joinPath(TK_3RD_ROOT, 'nanovg'), 
  joinPath(TK_3RD_ROOT, 'fribidi'), 
  joinPath(TK_3RD_ROOT, 'nanovg/base'), 
  joinPath(TK_3RD_ROOT, 'libunibreak'), 
  joinPath(TK_3RD_ROOT, 'gpinyin/include'), 
  joinPath(TK_3RD_ROOT, 'gtest/googletest'), 
  joinPath(TK_3RD_ROOT, 'gtest/googletest/include'), 
  ] + OS_CPPPATH

if TSLIB_LIB_DIR != '':
  LIBS = ['ts'] + LIBS
  LIBPATH = [TSLIB_LIB_DIR] + LIBPATH;
  CPPPATH = [TSLIB_INC_DIR] + CPPPATH;

os.environ['LCD'] = LCD
os.environ['TARGET_ARCH'] = 'arm'
os.environ['BIN_DIR'] = BIN_DIR;
os.environ['LIB_DIR'] = LIB_DIR;
os.environ['TK_ROOT'] = TK_ROOT;
os.environ['CCFLAGS'] = CCFLAGS;
os.environ['VGCANVAS'] = 'NANOVG'
os.environ['INPUT_ENGINE'] = INPUT_ENGINE;
os.environ['TSLIB_LIB_DIR'] = TSLIB_LIB_DIR;
os.environ['NANOVG_BACKEND'] = NANOVG_BACKEND;
os.environ['TK_3RD_ROOT'] = TK_3RD_ROOT;
os.environ['GTEST_ROOT'] = GTEST_ROOT;
os.environ['NATIVE_WINDOW'] = 'raw';
os.environ['TOOLS_NAME'] = '';
os.environ['GRAPHIC_BUFFER'] = GRAPHIC_BUFFER;

os.environ['OS_WHOLE_ARCHIVE'] = OS_WHOLE_ARCHIVE;
os.environ['AWTK_DLL_DEPS_LIBS'] = ';'.join(AWTK_DLL_DEPS_LIBS)
os.environ['STATIC_LIBS'] = ';'.join(STATIC_LIBS)

def has_custom_cc():
    return True

