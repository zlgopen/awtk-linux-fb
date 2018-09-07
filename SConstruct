import os
import platform

def joinPath(root, subdir):
  return os.path.normpath(os.path.join(root, subdir))

OS_NAME=platform.system()

TK_ROOT = joinPath(os.getcwd(), 'awtk')
TK_SRC = joinPath(TK_ROOT, 'src')
TK_3RD_ROOT = joinPath(TK_ROOT, '3rd')
TK_TOOLS_ROOT = joinPath(TK_ROOT, 'tools')
GTEST_ROOT = joinPath(TK_ROOT, '3rd/gtest/googletest')

TK_LINUX_FB_ROOT=os.path.normpath(os.getcwd())
BIN_DIR=joinPath(TK_LINUX_FB_ROOT, 'bin')
LIB_DIR=joinPath(TK_LINUX_FB_ROOT, 'lib')

LCD='SDL'
LCD='NANOVG'
LCD='LINUX_FB'
VGCANVAS="CAIRO"
INPUT_ENGINE='pinyin'
FRAME_BUFFER_FORMAT='rgb565'

COMMON_CCFLAGS='-g '
if FRAME_BUFFER_FORMAT=='rgba8888':
  COMMON_CCFLAGS=COMMON_CCFLAGS+' -DWITH_FB_8888=1';
else:
  COMMON_CCFLAGS=COMMON_CCFLAGS+' -DWITH_FB_565=1';

COMMON_CCFLAGS=' -DHAS_STD_MALLOC -DWITH_FS_RES -DHAS_STDIO -DWITH_VGCANVAS -DWITH_UNICODE_BREAK '
COMMON_CCFLAGS=COMMON_CCFLAGS+' -DSTBTT_STATIC -DSTB_IMAGE_STATIC -DWITH_STB_IMAGE -DWITH_STB_FONT '

TSLIB_INC_DIR=joinPath(TK_LINUX_FB_ROOT, '3rd/tslib/src')
TSLIB_LIB_DIR=joinPath(TK_LINUX_FB_ROOT, '3rd/tslib/src/.libs')


os.environ['LCD'] = LCD
os.environ['VGCANVAS'] = VGCANVAS 
os.environ['BIN_DIR'] = BIN_DIR;
os.environ['LIB_DIR'] = LIB_DIR;
os.environ['TK_ROOT'] = TK_ROOT;
os.environ['INPUT_ENGINE'] = INPUT_ENGINE;
os.environ['TSLIB_LIB_DIR'] = TSLIB_LIB_DIR;
os.environ['FRAME_BUFFER_FORMAT'] = FRAME_BUFFER_FORMAT;

OS_LIBS=[]
OS_LIBPATH=[]
OS_CPPPATH=[]
OS_LINKFLAGS=''
OS_FLAGS='-g -Wall'
OS_SUBSYSTEM_CONSOLE=''
OS_SUBSYSTEM_WINDOWS=''

OS_LIBS = ['GL'] + OS_LIBS + ['stdc++', 'pthread', 'm', 'dl']
COMMON_CCFLAGS = COMMON_CCFLAGS + ' -DLINUX -DHAS_PTHREAD'
  
LINKFLAGS=OS_LINKFLAGS;
CCFLAGS=OS_FLAGS + COMMON_CCFLAGS 
LIBPATH=[LIB_DIR] + OS_LIBPATH
LIBS=['awtk', 'gpinyin', 'awtk', 'cairo', 'pixman', 'linebreak'] + OS_LIBS

CPPPATH=[TK_ROOT, 
  TK_SRC, 
  TK_3RD_ROOT, 
  joinPath(TK_SRC, 'ext_widgets'), 
  joinPath(TK_3RD_ROOT, 'cairo/cairo'), 
  joinPath(TK_3RD_ROOT, 'pixman/pixman'), 
  joinPath(TK_3RD_ROOT, 'gpinyin/include'), 
  joinPath(TK_3RD_ROOT, 'libunibreak/src'), 
  ] + OS_CPPPATH

if TSLIB_LIB_DIR:
  LIBS = ['ts'] + LIBS
  LIBPATH = [TSLIB_LIB_DIR] + LIBPATH;
  CPPPATH = [TSLIB_INC_DIR] + CPPPATH;

DefaultEnvironment(CCFLAGS = CCFLAGS, 
  LIBS = LIBS,
  LIBPATH = LIBPATH,
  CPPPATH = CPPPATH,
  LINKFLAGS = LINKFLAGS,
  OS_SUBSYSTEM_CONSOLE=OS_SUBSYSTEM_CONSOLE,
  OS_SUBSYSTEM_WINDOWS=OS_SUBSYSTEM_WINDOWS
)

SConscriptFiles=[
  'awtk/3rd/pixman/SConscript',
  'awtk/3rd/cairo/SConscript',
  'awtk/3rd/gpinyin/SConscript', 
  'awtk/3rd/libunibreak/SConscript',
  'awtk-port/SConscript',
  ]
  
SConscript(SConscriptFiles)

