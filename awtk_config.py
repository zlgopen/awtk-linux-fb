import os
import platform
import shutil
import scons_argv
compile_helper = scons_argv.get_compile_config()

from awtk_config_common import TKC_STATIC_LIBS
from awtk_config_common import joinPath, toWholeArchive, genIdlAndDefEx, setEnvSpawn, genDllLinkFlags, copySharedLib, cleanSharedLib, scons_db_check_and_remove

OS_NAME = platform.system()

def joinPath(root, subdir):
  return os.path.normpath(os.path.join(root, subdir))

def lcd_devices_is_egl(lcd_devices):
  if lcd_devices == 'egl_for_fsl' or lcd_devices == 'egl_for_x11' or lcd_devices == 'egl_for_gbm' or lcd_devices == 'egl_for_wayland':
    return True
  return False

CWD=os.path.normpath(os.path.abspath(os.path.dirname(__file__)));

TK_LINUX_FB_ROOT = CWD
TK_ROOT          = joinPath(TK_LINUX_FB_ROOT, '../awtk')
TK_SRC           = joinPath(TK_ROOT, 'src')
TK_3RD_ROOT      = joinPath(TK_ROOT, '3rd')
GTEST_ROOT       = joinPath(TK_ROOT, '3rd/gtest/googletest')

BUILD_DIR        = joinPath(TK_LINUX_FB_ROOT, 'build')
BIN_DIR          = joinPath(TK_LINUX_FB_ROOT, 'bin')
LIB_DIR          = joinPath(TK_LINUX_FB_ROOT, 'lib')
VAR_DIR          = joinPath(BUILD_DIR, 'var')
TK_DEMO_ROOT     = joinPath(TK_ROOT, 'demos')

LCD_DIR          = joinPath(TK_LINUX_FB_ROOT, 'awtk-port/lcd_linux')
INPUT_DIR        = joinPath(TK_LINUX_FB_ROOT, 'awtk-port/input_thread')

WAYLAND_DIR      = joinPath(TK_LINUX_FB_ROOT, 'awtk-wayland/wayland')
EGL_WAYLAND_DIR  = joinPath(TK_LINUX_FB_ROOT, 'awtk-wayland/egl_for_wayland')

# lcd devices
LCD_DEVICES='fb'
# LCD_DEVICES='drm'
# LCD_DEVICES='wayland'
# LCD_DEVICES='egl_for_fsl'
# LCD_DEVICES='egl_for_x11'
# LCD_DEVICES='egl_for_gbm'
# LCD_DEVICES='egl_for_wayland'
LCD_DEVICES = compile_helper.get_value('LCD_DEVICES', LCD_DEVICES)

NANOVG_BACKEND=''
VGCANVAS='NANOVG'
#VGCANVAS='CAIRO'
# if enable cairo, you can enable the following option according to your condition.
#os.environ['USE_ARM_NEON'] = 'true'
#os.environ['USE_ARM_SIMD'] = 'true'
#os.environ['USE_X86_MMX'] = 'true'
#os.environ['USE_SSE2'] = 'true'
#os.environ['USE_SSSE3'] = 'true'
#VGCANVAS='NANOVG_PLUS'
VGCANVAS = compile_helper.get_value('VGCANVAS', VGCANVAS)

if LCD_DEVICES == 'fb' or LCD_DEVICES == 'drm' or LCD_DEVICES == 'wayland':
  LCD='LINUX_FB'
  NANOVG_BACKEND='AGGE'
elif lcd_devices_is_egl(LCD_DEVICES) :
  LCD='FB_GL'
  NANOVG_BACKEND='GLES2'

#INPUT_ENGINE='null'
#INPUT_ENGINE='spinyin'
#INPUT_ENGINE='t9'
#INPUT_ENGINE='t9ext'
INPUT_ENGINE='pinyin'
INPUT_ENGINE = compile_helper.get_value('INPUT_ENGINE', INPUT_ENGINE)

COMMON_CCFLAGS=' -DHAS_STD_MALLOC -DHAS_STDIO -DHAS_FAST_MEMCPY -DWITH_VGCANVAS -DWITH_UNICODE_BREAK '
COMMON_CCFLAGS=COMMON_CCFLAGS+' -DLOAD_ASSET_WITH_MMAP=1 -DWITH_SOCKET=1 '
COMMON_CCFLAGS=COMMON_CCFLAGS+' -DWITH_ASSET_LOADER -DWITH_FS_RES -DHAS_GET_TIME_US64=1 ' 
COMMON_CCFLAGS=COMMON_CCFLAGS+' -DSTBTT_STATIC -DSTB_IMAGE_STATIC -DWITH_STB_IMAGE -DWITH_STB_FONT -DWITH_TEXT_BIDI=1 '
COMMON_CCFLAGS=COMMON_CCFLAGS+' -DAPP_TYPE=APP_MOBILE -DLINUX_FB '
#COMMON_CCFLAGS=COMMON_CCFLAGS+' -DENABLE_CUSTOM_KEYS=1 '
#COMMON_CCFLAGS=COMMON_CCFLAGS+' -DCUSTOM_KEYS_FILEPATH=\"asset://custom_keys.json\" '

if LCD_DEVICES == 'fb' or LCD_DEVICES == 'wayland':
  COMMON_CCFLAGS=COMMON_CCFLAGS+' -DWITH_NANOVG_AGGE -DWITH_LINUX_FB -DWITH_FAST_LCD_PORTRAIT '
elif LCD_DEVICES == 'drm' :
  COMMON_CCFLAGS=COMMON_CCFLAGS+' -DWITH_NANOVG_AGGE -DWITH_LINUX_DRM -DWITH_FAST_LCD_PORTRAIT '
elif lcd_devices_is_egl(LCD_DEVICES) :
  COMMON_CCFLAGS=COMMON_CCFLAGS+' -DWITH_GPU_GL -DWITH_GPU_GLES2 -DWITH_GPU -DWITH_LINUX_EGL '

if LCD_DEVICES == 'egl_for_wayland' :
  COMMON_CCFLAGS=COMMON_CCFLAGS+' -DWL_EGL_PLATFORM '

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
if compile_helper.get_value('WITH_CUSTOM_GRAPHIC_BUFFER', False) :
  GRAPHIC_BUFFER='custom'
  #GRAPHIC_BUFFER='jzgpu'
#if GRAPHIC_BUFFER == 'jzgpu':
#  COMMON_CCFLAGS=COMMON_CCFLAGS+' -DWITH_JZGPU'

#only for c compiler flags
COMMON_CFLAGS=''
COMMON_CFLAGS=COMMON_CFLAGS+' -std=gnu11 '

OS_LIBS=[]
OS_LIBPATH=[]
OS_CPPPATH=[]
OS_LINKFLAGS = ''
OS_SUBSYSTEM_CONSOLE=''
OS_SUBSYSTEM_WINDOWS=''
OS_FLAGS = ' -Wall -fno-strict-aliasing '

TSLIB_LIB_DIR=''
TSLIB_INC_DIR=''

#for build tslib
#TSLIB_INC_DIR=joinPath(TK_LINUX_FB_ROOT, '3rd/tslib/src')
#TSLIB_LIB_DIR=joinPath(TK_LINUX_FB_ROOT, '3rd/tslib/src/.libs')

#for prebuild tslib
#TSLIB_LIB_DIR='/opt/tslib/lib'
#TSLIB_INC_DIR='/opt/tslib/include'

#for qemu
TOOLS_PREFIX='/opt/qemu/buildroot-2021.02.2/output/host/bin/arm-linux-'

#for poky cortex-a9
#TOOLS_PREFIX='/opt/poky/1.7/sysroots/x86_64-pokysdk-linux/usr/bin/arm-poky-linux-gnueabi/arm-poky-linux-gnueabi-'
#OS_FLAGS=' -Wall -mfloat-abi=hard -fno-strict-aliasing '

#for v3s mango
#TOOLS_PREFIX='/opt/v3s/mango/tools/external-toolchain/bin/arm-linux-gnueabi-'
#OS_FLAGS='-std=gnu99 -mthumb -mabi=aapcs-linux -mlittle-endian -fdata-sections -ffunction-sections -mcpu=cortex-a7 -mtune=cortex-a7 -mfpu=fpv4-sp-d16 -mfloat-abi=softfp '

#for pc build
#TOOLS_PREFIX=''

#for android
#TOOLS_PREFIX='/opt/android-ndk-r20b/toolchains/llvm/prebuilt/linux-x86_64/bin/'
#TOOLS_PREFIX='/Users/jim/android/android-ndk-r21d/toolchains/llvm/prebuilt/darwin-x86_64/bin/'

TOOLS_PREFIX = compile_helper.get_unique_value('TOOLS_PREFIX', TOOLS_PREFIX)
TSLIB_LIB_DIR = compile_helper.get_unique_value('TSLIB_LIB_DIR', TSLIB_LIB_DIR)
TSLIB_INC_DIR = compile_helper.get_unique_value('TSLIB_INC_DIR', TSLIB_INC_DIR)
OS_FLAGS = compile_helper.get_value('OS_FLAGS', OS_FLAGS)
OS_LINKFLAGS = compile_helper.get_value('OS_LINKFLAGS', OS_LINKFLAGS)

TARGET_ARCH = platform.architecture();

if compile_helper.get_value('PLATFORM', 'linux') == 'android' :
  # for android
  TSLIB_LIB_DIR=''
  TSLIB_INC_DIR=''
  CC = TOOLS_PREFIX + compile_helper.get_value('TOOLS_CC', 'armv7a-linux-androideabi16-clang')
  CXX = TOOLS_PREFIX + compile_helper.get_value('TOOLS_CXX', 'armv7a-linux-androideabi16-clang++')
  LD = TOOLS_PREFIX + compile_helper.get_value('TOOLS_LD', 'arm-linux-androideabi-ld')
  AR = TOOLS_PREFIX + compile_helper.get_value('TOOLS_AR', 'arm-linux-androideabi-ar')
  STRIP = TOOLS_PREFIX + compile_helper.get_value('TOOLS_STRIP', 'arm-linux-androideabi-strip')
  RANLIB = TOOLS_PREFIX + compile_helper.get_value('TOOLS_RANLIB', 'arm-linux-androideabi-ranlib') 
  OS_LINKFLAGS=' -Wl,--allow-multiple-definition '
  OS_LIBS = compile_helper.get_value('OS_LIBS', []) + ['stdc++', 'm']
  OS_FLAGS='-Wall -Os -DFB_DEVICE_FILENAME=\\\"\"/dev/graphics/fb0\\\"\" '
else :
  CC = TOOLS_PREFIX + compile_helper.get_value('TOOLS_CC', 'gcc') 
  CXX = TOOLS_PREFIX + compile_helper.get_value('TOOLS_CXX', 'g++')
  LD = TOOLS_PREFIX + compile_helper.get_value('TOOLS_LD', 'g++')
  AR = TOOLS_PREFIX + compile_helper.get_value('TOOLS_AR', 'ar')
  RANLIB = TOOLS_PREFIX + compile_helper.get_value('TOOLS_RANLIB', 'ranlib')
  STRIP = TOOLS_PREFIX + compile_helper.get_value('TOOLS_STRIP', 'strip')
  OS_LIBS = compile_helper.get_value('OS_LIBS', []) + ['atomic', 'stdc++', 'pthread', 'rt', 'm', 'dl']

OS_DEBUG = compile_helper.get_value('DEBUG', False)
if OS_DEBUG :
  OS_FLAGS += ' -g -O0 '
else :
  OS_FLAGS += ' -Os '

OS_LINKFLAGS= OS_LINKFLAGS + ' -Wl,-rpath=./bin -Wl,-rpath=./ ' 

if LCD_DEVICES =='drm' :
  #for drm
  #OS_CPPPATH += ['/usr/include/libdrm']
  OS_LIBS = ['drm'] + OS_LIBS
elif LCD_DEVICES =='wayland' :
  OS_LIBS = [ 'xkbcommon', 'wayland-client', 'wayland-cursor' ] + OS_LIBS
elif LCD_DEVICES =='egl_for_fsl':
  #for egl for fsl
  OS_FLAGS=OS_FLAGS + ' -DEGL_API_FB '
  OS_LIBS = [ 'GLESv2', 'EGL' ] + OS_LIBS
elif LCD_DEVICES =='egl_for_x11' :
  #for egl for fsl
  OS_LIBS = [ 'X11', 'EGL', 'GLESv2' ] + OS_LIBS
elif LCD_DEVICES =='egl_for_gbm' :
  #for egl for gbm
  #OS_CPPPATH += ['/usr/include/libdrm', '/usr/include/GLES2']
  OS_LIBS = [ 'drm', 'gbm', 'EGL', 'GLESv2' ] + OS_LIBS
elif LCD_DEVICES =='egl_for_wayland' :
  OS_LIBS = [ 'xkbcommon', 'wayland-client', 'wayland-cursor', 'GLESv2', 'EGL', 'wayland-egl' ] + OS_LIBS

COMMON_CCFLAGS = COMMON_CCFLAGS + ' -DLINUX -DHAS_PTHREAD -fPIC '
COMMON_CCFLAGS = COMMON_CCFLAGS+' -DWITH_DATA_READER_WRITER=1 '
COMMON_CCFLAGS = COMMON_CCFLAGS+' -DWITH_EVENT_RECORDER_PLAYER=1 '
COMMON_CCFLAGS = COMMON_CCFLAGS + ' -DWITH_WIDGET_TYPE_CHECK=1 '

if compile_helper.get_value('ENABLE_CURSOR', True) :
  COMMON_CCFLAGS = COMMON_CCFLAGS + ' -DENABLE_CURSOR '

if compile_helper.get_value('WITH_G2D', True) :
  COMMON_CCFLAGS = COMMON_CCFLAGS + ' -DWITH_G2D '

if TSLIB_LIB_DIR != '':
  COMMON_CCFLAGS = COMMON_CCFLAGS + ' -DHAS_TSLIB '

OS_PROJECTS=[]
CFLAGS=COMMON_CFLAGS
LINKFLAGS=OS_LINKFLAGS;
LIBPATH=[LIB_DIR, BIN_DIR] + OS_LIBPATH
CCFLAGS=OS_FLAGS + COMMON_CCFLAGS 

if compile_helper.get_value('EXTERN_CODE', None) != None :
  LINKFLAGS=OS_LINKFLAGS + toWholeArchive(['__extern_code'])

AWTK_STATIC_LIBS =['awtk_global', 'fscript_ext_widgets', 'extwidgets', 'widgets', 'awtk_linux_fb', 'base', 'gpinyin', 'linebreak', 'fribidi']
AWTK_STATIC_LIBS += TKC_STATIC_LIBS
if TSLIB_LIB_DIR != '':
  SHARED_LIBS=['awtk'] + OS_LIBS + ['ts'];
else:
  SHARED_LIBS=['awtk'] + OS_LIBS;

if VGCANVAS == 'NANOVG':
  TK_ROOT_VAR = joinPath(VAR_DIR, 'awtk')
  OS_PROJECTS = [joinPath(TK_ROOT_VAR, '3rd/nanovg/SConscript') ]
  OS_CPPPATH += [joinPath(TK_3RD_ROOT, 'nanovg'), joinPath(TK_3RD_ROOT, 'nanovg/gl'), joinPath(TK_3RD_ROOT, 'nanovg/base') ]
  if LCD_DEVICES =='fb' or LCD_DEVICES =='drm' or LCD_DEVICES =='wayland':
    AWTK_STATIC_LIBS = AWTK_STATIC_LIBS + ['nanovg-agge', 'agge', 'nanovg']
    AWTK_DLL_DEPS_LIBS = ['nanovg-agge', 'agge', 'nanovg']
  elif lcd_devices_is_egl(LCD_DEVICES) :
    CCFLAGS += ' -DWITH_NANOVG_GLES2 -DWITH_NANOVG_GL -DWITH_NANOVG_GPU '
    AWTK_STATIC_LIBS = AWTK_STATIC_LIBS + ['glad', 'nanovg']
    AWTK_DLL_DEPS_LIBS = ['glad', 'nanovg']
elif VGCANVAS == 'CAIRO':
  TK_ROOT_VAR = joinPath(VAR_DIR, 'awtk')
  OS_PROJECTS = [joinPath(TK_ROOT_VAR, '3rd/cairo/SConscript'), joinPath(TK_ROOT_VAR, '3rd/pixman/SConscript')]
  OS_CPPPATH += [joinPath(TK_3RD_ROOT, 'cairo'), joinPath(TK_3RD_ROOT, 'pixman') ]
  AWTK_STATIC_LIBS = AWTK_STATIC_LIBS + ['cairo', 'pixman']
  AWTK_DLL_DEPS_LIBS= ['cairo', 'pixman']
  CCFLAGS += ' -DWITH_VGCANVAS_CAIRO -DHAVE_CONFIG_H -DCAIRO_WIN32_STATIC_BUILD '

elif VGCANVAS == 'NANOVG_PLUS':
  TK_ROOT_VAR = joinPath(VAR_DIR, 'awtk')
  OS_PROJECTS = [ joinPath(TK_ROOT_VAR, '3rd/nanovg_plus/SConscript') ]
  OS_CPPPATH += [joinPath(TK_3RD_ROOT, 'nanovg_plus/gl'), joinPath(TK_3RD_ROOT, 'nanovg_plus/base') ]
  if lcd_devices_is_egl(LCD_DEVICES) :
    CCFLAGS += ' -DWITH_NANOVG_PLUS_GPU -DWITH_NANOVG_GPU -DWITH_GPU_GL -DWITH_GPU_GLES2 -DNVGP_GLES2 '
    AWTK_STATIC_LIBS = AWTK_STATIC_LIBS + ['glad', 'nanovg_plus']
    AWTK_DLL_DEPS_LIBS = ['glad', 'nanovg_plus'] 

OS_WHOLE_ARCHIVE =toWholeArchive(AWTK_STATIC_LIBS)
STATIC_LIBS = AWTK_STATIC_LIBS + OS_LIBS
LIBS = AWTK_STATIC_LIBS + OS_LIBS
AWTK_DLL_DEPS_LIBS = AWTK_DLL_DEPS_LIBS + OS_LIBS

CPPPATH=[TK_ROOT, 
  TK_SRC, 
  TK_3RD_ROOT, 
  LCD_DIR, 
  INPUT_DIR, 
  WAYLAND_DIR, 
  EGL_WAYLAND_DIR,
  joinPath(TK_SRC, 'ext_widgets'), 
  joinPath(TK_SRC, 'custom_widgets'), 
  joinPath(TK_ROOT, 'tools'), 
  joinPath(TK_3RD_ROOT, 'agge'), 
  joinPath(TK_3RD_ROOT, 'agg/include'), 
  joinPath(TK_3RD_ROOT, 'mbedtls/include'), 
  joinPath(TK_3RD_ROOT, 'mbedtls/3rdparty/everest/include'), 
  joinPath(TK_3RD_ROOT, 'fribidi'), 
  joinPath(TK_3RD_ROOT, 'libunibreak'), 
  joinPath(TK_3RD_ROOT, 'gpinyin/include'), 
  joinPath(TK_3RD_ROOT, 'gtest/googletest'), 
  joinPath(TK_3RD_ROOT, 'gtest/googletest/include'), 
  ] + OS_CPPPATH

if TSLIB_LIB_DIR != '':
  LIBS = LIBS + ['ts']
  LIBPATH = [TSLIB_LIB_DIR] + LIBPATH;
  CPPPATH = [TSLIB_INC_DIR] + CPPPATH;

LIBPATH += compile_helper.get_value('OS_LIBPATH', [])
CPPPATH += compile_helper.get_value('OS_CPPPATH', [])

os.environ['LCD'] = LCD
os.environ['LCD_DEVICES'] = LCD_DEVICES
os.environ['TARGET_ARCH'] = 'arm'
os.environ['BIN_DIR'] = BIN_DIR;
os.environ['LIB_DIR'] = LIB_DIR;
os.environ['TK_ROOT'] = TK_ROOT;
os.environ['CCFLAGS'] = CCFLAGS;
os.environ['VGCANVAS'] = VGCANVAS
os.environ['INPUT_ENGINE'] = INPUT_ENGINE;
os.environ['TSLIB_LIB_DIR'] = TSLIB_LIB_DIR;
os.environ['NANOVG_BACKEND'] = NANOVG_BACKEND;
os.environ['TK_3RD_ROOT'] = TK_3RD_ROOT;
os.environ['GTEST_ROOT'] = GTEST_ROOT;
os.environ['TOOLS_NAME'] = '';
os.environ['GRAPHIC_BUFFER'] = GRAPHIC_BUFFER;
os.environ['WITH_AWTK_SO'] = 'true'

if LCD_DEVICES =='fb' or LCD_DEVICES =='drm' or LCD_DEVICES =='wayland':
  os.environ['NATIVE_WINDOW'] = 'raw';
elif lcd_devices_is_egl(LCD_DEVICES) :
  os.environ['NATIVE_WINDOW'] = 'fb_gl';


os.environ['OS_WHOLE_ARCHIVE'] = OS_WHOLE_ARCHIVE;
os.environ['AWTK_DLL_DEPS_LIBS'] = ';'.join(AWTK_DLL_DEPS_LIBS)
os.environ['STATIC_LIBS'] = ';'.join(STATIC_LIBS)
os.environ['CROSS_COMPILE'] = str(not TOOLS_PREFIX == '')

def has_custom_cc():
    return True

def copySharedLib(src, dst, name):
  src = os.path.join(src, 'bin/lib'+name+'.so')
  src = os.path.normpath(src);
  dst = os.path.normpath(dst);

  if os.path.dirname(src) == dst:
      return

  if not os.path.exists(src):
    print('Can\'t find ' + src + '. Please build '+name+'before!')
  else:
    if not os.path.exists(dst):
        os.makedirs(dst)
    shutil.copy(src, dst)
    print(src + '==>' + dst);
