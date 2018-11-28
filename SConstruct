import os
import platform

def joinPath(root, subdir):
  return os.path.normpath(os.path.join(root, subdir))

OS_NAME=platform.system()

TK_ROOT = joinPath(os.getcwd(), 'awtk')
TK_SRC = joinPath(TK_ROOT, 'src')
TK_3RD_ROOT = joinPath(TK_ROOT, '3rd')

TK_LINUX_FB_ROOT=os.path.normpath(os.getcwd())
BIN_DIR=joinPath(TK_ROOT, 'bin')
LIB_DIR=joinPath(TK_ROOT, 'lib')

#TSLIB_INC_DIR=joinPath(TK_LINUX_FB_ROOT, '3rd/tslib/src')
#TSLIB_LIB_DIR=joinPath(TK_LINUX_FB_ROOT, '3rd/tslib/src/.libs')

#TOOLS_PREFIX=''
#TSLIB_LIB_DIR=''

TSLIB_LIB_DIR='/opt/28x/tslib/lib'
TSLIB_INC_DIR='/opt/28x/tslib/include'
TOOLS_PREFIX='/opt/28x/gcc-4.4.4-glibc-2.11.1-multilib-1.0/arm-fsl-linux-gnueabi/bin/arm-linux-'

LCD='SDL'
LCD='NANOVG'
LCD='LINUX_FB'
INPUT_ENGINE='pinyin'
NANOVG_BACKEND='AGGE'

COMMON_CCFLAGS='-g '
COMMON_CCFLAGS=' -DHAS_STD_MALLOC -DWITH_FS_RES -DHAS_STDIO -DWITH_VGCANVAS -DWITH_UNICODE_BREAK -DLINUX'
COMMON_CCFLAGS=COMMON_CCFLAGS+' -DSTBTT_STATIC -DSTB_IMAGE_STATIC -DWITH_STB_IMAGE -DWITH_STB_FONT '
COMMON_CCFLAGS=COMMON_CCFLAGS+' -DWITH_NANOVG_AGGE '

OS_LIBS=[]
OS_LIBPATH=[]
OS_CPPPATH=[]
OS_LINKFLAGS=''
OS_FLAGS='-g -Wall -Os '
OS_SUBSYSTEM_CONSOLE=''
OS_SUBSYSTEM_WINDOWS=''

OS_LIBS = OS_LIBS + ['stdc++', 'pthread', 'm', 'dl']
COMMON_CCFLAGS = COMMON_CCFLAGS + ' -DLINUX -DHAS_PTHREAD -DENABLE_CURSOR '

if TSLIB_LIB_DIR != '':
  COMMON_CCFLAGS = COMMON_CCFLAGS + ' -DHAS_TSLIB '

LINKFLAGS=OS_LINKFLAGS;
CCFLAGS=OS_FLAGS + COMMON_CCFLAGS 
LIBPATH=[LIB_DIR] + OS_LIBPATH
LIBS=['awtk', 'gpinyin', 'awtk_linux_fb', 'awtk', 'nanovg-agge', 'agge', 'nanovg', 'linebreak'] + OS_LIBS

CPPPATH=[TK_ROOT, 
  TK_SRC, 
  TK_3RD_ROOT, 
  joinPath(TK_3RD_ROOT, 'agge'), 
  joinPath(TK_3RD_ROOT, 'nanovg'), 
  joinPath(TK_SRC, 'ext_widgets'), 
  joinPath(TK_3RD_ROOT, 'nanovg/base'), 
  joinPath(TK_3RD_ROOT, 'libunibreak'), 
  joinPath(TK_3RD_ROOT, 'gpinyin/include'), 
  ] + OS_CPPPATH

if TSLIB_LIB_DIR != '':
  LIBS = ['ts'] + LIBS
  LIBPATH = [TSLIB_LIB_DIR] + LIBPATH;
  CPPPATH = [TSLIB_INC_DIR] + CPPPATH;

DefaultEnvironment(CCFLAGS = CCFLAGS, 
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
  'awtk/3rd/nanovg/SConscript',
  'awtk/3rd/agge/SConscript',
  'awtk/3rd/gpinyin/SConscript', 
  'awtk/3rd/libunibreak/SConscript',
  'awtk/src/SConscript',
  'awtk/demos/SConscript',
  'awtk-port/SConscript',
  ]

os.environ['LCD'] = LCD
os.environ['BIN_DIR'] = BIN_DIR;
os.environ['LIB_DIR'] = LIB_DIR;
os.environ['TK_ROOT'] = TK_ROOT;
os.environ['INPUT_ENGINE'] = INPUT_ENGINE;
os.environ['TSLIB_LIB_DIR'] = TSLIB_LIB_DIR;
os.environ['NANOVG_BACKEND'] = NANOVG_BACKEND;
os.environ['CCFLAGS'] = CCFLAGS;
os.environ['TK_3RD_ROOT'] = TK_3RD_ROOT;
  
SConscript(SConscriptFiles)

