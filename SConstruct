import os
import platform

def joinPath(root, subdir):
  return os.path.normpath(os.path.join(root, subdir))

TK_ROOT = joinPath(os.getcwd(), '../awtk')
TK_SRC = joinPath(TK_ROOT, 'src')
TK_3RD_ROOT = joinPath(TK_ROOT, '3rd')
TK_LINUX_FB_ROOT=os.path.normpath(os.getcwd())
BIN_DIR=joinPath(TK_LINUX_FB_ROOT, 'build/bin')
LIB_DIR=joinPath(TK_LINUX_FB_ROOT, 'build/lib')

APP_NAME=''
APP_NAME=joinPath(os.getcwd(), '../awtk-examples/Ventilator-Demo')
if APP_NAME == '':
  APP_PROJ = [joinPath(TK_ROOT, 'demos/SConscript')]
else:
  APP_PROJ = [joinPath(APP_NAME, 'src/SConscript')]

#for build tslib
#TSLIB_INC_DIR=joinPath(TK_LINUX_FB_ROOT, '3rd/tslib/src')
#TSLIB_LIB_DIR=joinPath(TK_LINUX_FB_ROOT, '3rd/tslib/src/.libs')

#for prebuild tslib
TSLIB_LIB_DIR='/opt/28x/tslib/lib'
TSLIB_INC_DIR='/opt/28x/tslib/include'
TOOLS_PREFIX='/opt/poky/1.7/sysroots/x86_64-pokysdk-linux/usr/bin/arm-poky-linux-gnueabi/arm-poky-linux-gnueabi-'

#for pc build
#TOOLS_PREFIX=''
#TSLIB_LIB_DIR=''

LCD='LINUX_FB'
INPUT_ENGINE='pinyin'
NANOVG_BACKEND='AGGE'

COMMON_CCFLAGS=' -DHAS_STD_MALLOC -DWITH_FS_RES -DHAS_STDIO -DWITH_VGCANVAS -DWITH_UNICODE_BREAK -DLINUX'
COMMON_CCFLAGS=COMMON_CCFLAGS+' -DSTBTT_STATIC -DSTB_IMAGE_STATIC -DWITH_STB_IMAGE -DWITH_STB_FONT '
COMMON_CCFLAGS=COMMON_CCFLAGS+' -DWITH_NANOVG_AGGE -DWITH_WIDGET_TYPE_CHECK'

OS_LIBS=[]
OS_LIBPATH=[]
OS_CPPPATH=[]
OS_LINKFLAGS=''
OS_FLAGS='-g -Wall -Os -mfloat-abi=hard '
OS_SUBSYSTEM_CONSOLE=''
OS_SUBSYSTEM_WINDOWS=''

OS_LIBS = OS_LIBS + ['stdc++', 'pthread', 'm', 'dl']
COMMON_CCFLAGS = COMMON_CCFLAGS + ' -DLINUX -DHAS_PTHREAD -DENABLE_CURSOR '

if TSLIB_LIB_DIR != '':
  COMMON_CCFLAGS = COMMON_CCFLAGS + ' -DHAS_TSLIB '

LINKFLAGS=OS_LINKFLAGS;
LIBPATH=[LIB_DIR] + OS_LIBPATH
CCFLAGS=OS_FLAGS + COMMON_CCFLAGS 
LIBS=['awtk', 'gpinyin', 'awtk_linux_fb', 'awtk', 'nanovg-agge', 'agge', 'nanovg', 'linebreak'] + OS_LIBS

CPPPATH=[TK_ROOT, 
  TK_SRC, 
  TK_3RD_ROOT, 
  joinPath(TK_SRC, 'ext_widgets'), 
  joinPath(TK_ROOT, 'tools'), 
  joinPath(TK_3RD_ROOT, 'agge'), 
  joinPath(TK_3RD_ROOT, 'agg/include'), 
  joinPath(TK_3RD_ROOT, 'nanovg'), 
  joinPath(TK_3RD_ROOT, 'nanovg/base'), 
  joinPath(TK_3RD_ROOT, 'libunibreak'), 
  joinPath(TK_3RD_ROOT, 'gpinyin/include'), 
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

DefaultEnvironment(CCFLAGS = CCFLAGS, 
  CFLAGS='-std=gnu11',
  CC=TOOLS_PREFIX+'gcc',
  CXX=TOOLS_PREFIX+'g++',
  LD=TOOLS_PREFIX+'g++',
  AR=TOOLS_PREFIX+'ar',
  STRIP=TOOLS_PREFIX+'strip',
  LIBS = LIBS,
  LIBPATH = LIBPATH,
  CPPPATH = CPPPATH,
  LINKFLAGS = LINKFLAGS,
  OS_SUBSYSTEM_CONSOLE=OS_SUBSYSTEM_CONSOLE,
  OS_SUBSYSTEM_WINDOWS=OS_SUBSYSTEM_WINDOWS
)

SConscriptFiles=[
  joinPath(TK_ROOT, '3rd/nanovg/SConscript'),
  joinPath(TK_ROOT, '3rd/agg/SConscript'),
  joinPath(TK_ROOT, '3rd/agge/SConscript'),
  joinPath(TK_ROOT, '3rd/gpinyin/SConscript'), 
  joinPath(TK_ROOT, '3rd/libunibreak/SConscript'),
  joinPath(TK_ROOT, 'src/SConscript'),
  joinPath(TK_ROOT, 'tools/common/SConscript'), 
  joinPath(TK_ROOT, 'tools/ui_gen/xml_to_ui/SConscript'),
  'awtk-port/SConscript',
  ] + APP_PROJ;
  
SConscript(SConscriptFiles)

