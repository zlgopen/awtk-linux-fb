import os
import sys

global INIT
INIT = False

COMPILE_CONFIG = {
  'OUTPUT_DIR' : { 'value' : None, 'type' : str.__name__, 'desc' : ['compiled export directory '], 'help_info' : 'set awtk-linux-fb compiled export directory, default value is None, None is system\'s value'},
  'OS_FLAGS' : { 'value' : None, 'type' : str.__name__, 'desc' : ['compile flags', '..example: OS_FLAGS = " -flag1 -flag2 "'], 'help_info' : 'set compile\'s flags, so care of system and compile tools'},
  'OS_LINKFLAGS' : { 'value' : None, 'type' : str.__name__, 'desc' : ['link flags', '..example: OS_LINKFLAGS = " -flag1 -flag2 "'], 'help_info' : 'set compile\'s link flags, so care of system and compile tools'},
  'OS_LIBS' : { 'value' : [], 'type' : list.__name__, 'desc' : ['compile libs', '..example: OS_LIBS = ["lib1", "lib2"]'], 'help_info' : 'set compile\'s libs, so care of system and compile tools, use \',\' split muliple libraries '},
  'OS_LIBPATH' : { 'value' : [], 'type' : list.__name__, 'desc' : ['compile lib paths', '..example: OS_LIBPATH = ["/path/to/libdir1", "/path/to/libdir2"]'], 'help_info' : 'set compile\'s lib paths, so care of system and compile tools, use \',\' split muliple librarie\'s paths '},
  'OS_CPPPATH' : { 'value' : [], 'type' : list.__name__, 'desc' : ['compile include paths', '..example: OS_CPPPATH = ["/path/to/incdir1", "/path/to/incdir2"]'], 'help_info' : 'set compile\'s include paths, so care of system and compile tools, use \',\' split muliple include path '},
  'TOOLS_CC' : { 'value' : None, 'type' : str.__name__, 'desc' : ['compile tools prefix CC\'s name'], 'help_info' : 'set compile tools prefix CC\'s name, TOOLS_CC=XXXXX, CC=TOOLS_PREFIX+TOOLS_CC '},
  'TOOLS_CXX' : { 'value' : None, 'type' : str.__name__, 'desc' : ['compile tools prefix CXX\'s name'], 'help_info' : 'set compile tools prefix CXX\'s name, TOOLS_CXX=XXXXX, CXX=TOOLS_PREFIX+TOOLS_CXX '},
  'TOOLS_LD' : { 'value' : None, 'type' : str.__name__, 'desc' : ['compile tools prefix LD\'s name'], 'help_info' : 'set compile tools prefix LD\'s name, TOOLS_LD=XXXXX, LD=TOOLS_PREFIX+TOOLS_LD '},
  'TOOLS_AR' : { 'value' : None, 'type' : str.__name__, 'desc' : ['compile tools prefix AR\'s name'], 'help_info' : 'set compile tools prefix AR\'s name, TOOLS_AR=XXXXX, AR=TOOLS_PREFIX+TOOLS_AR '},
  'TOOLS_STRIP' : { 'value' : None, 'type' : str.__name__, 'desc' : ['compile tools prefix STRIP\'s name'], 'help_info' : 'set compile tools prefix STRIP\'s name, TOOLS_STRIP=XXXXX, STRIP=TOOLS_PREFIX+TOOLS_STRIP '},
  'TOOLS_RANLIB' : { 'value' : None, 'type' : str.__name__, 'desc' : ['compile tools prefix RANLIB\'s name'], 'help_info' : 'set compile tools prefix RANLIB\'s name, TOOLS_RANLIB=XXXXX, RANLIB=TOOLS_PREFIX+TOOLS_RANLIB '},
  'TOOLS_PREFIX' : { 'value' : None, 'type' : str.__name__, 'desc' : ['compile tools prefix path', '..example: TOOLS_PREFIX = "/path/to/arm-linux-gnueabihf-"'], 'help_info' : 'set compile tools prefix path, value allow set \'\''},
  'TSLIB_LIB_DIR' : { 'value' : None, 'type' : str.__name__, 'desc' : ['tslib lib path', '..example: TSLIB_LIB_DIR = "/path/to/tslib/lib"'], 'help_info' : 'set use tslib lib path, value allow set \'\''},
  'TSLIB_INC_DIR' : { 'value' : None, 'type' : str.__name__, 'desc' : ['tslib include path', '..example: TSLIB_INC_DIR = "/path/to/tslib/include"'], 'help_info' : 'set use tslib lib path, value allow set \'\''},
  'ENABLE_CURSOR' : { 'value' : True, 'type' : bool.__name__, 'desc' : ['enable cursor mouse'], 'help_info' : 'set enable cursor mouse, value is true or false'},
  'INPUT_ENGINE' : { 'value' : None, 'type' : str.__name__, 'desc' : ['null/spinyin/t9/t9ext/pinyin', '..example: INPUT_ENGINE = "pinyin"'], 'help_info' : 'set awtk use input engine' },
  'VGCANVAS' : { 'value' : None, 'type' : str.__name__, 'desc' : ['NANOVG/NANOVG_PLUS/CAIRO', '..example: VGCANVAS = "NANOVG"'], 'help_info' : 'set awtk use render vgcanvas type' },
  'DEBUG' : { 'value' : False, 'type' : bool.__name__, 'desc' : ['awtk\'s compile is debug'], 'help_info' : 'awtk\'s compile is debug, value is true or false' },
  'APP' : { 'value' : None, 'type' : str.__name__, 'save_file' : False, 'desc' : ['build this app'], 'help_info' : 'set app is build, value is app\'s root' },
  'LCD_DEVICES' : { 'value' : None, 'type' : str.__name__, 'desc' : ['linux\'s lcd devices type, value is fb/drm/wayland/egl_for_fsl/egl_for_x11/egl_for_gbm/egl_for_wayland', '..example: LCD_DEVICES = "fb"'], 'help_info' : 'when building, use linux\'s lcd devices type, value is fb/drm/wayland/egl_for_fsl/egl_for_x11/egl_for_gbm/egl_for_wayland' },
  'BUILD_TOOLS' : { 'value' : True, 'type' : bool.__name__, 'desc' : ['build awtk\'s linux-fb\'s tools'], 'help_info' : 'build awtk\'s linux-fb\'s tools, value is true or false' },
  'BUILD_DEMOS' : { 'value' : True, 'type' : bool.__name__, 'desc' : ['build awtk\'s linux-fb\'s demos'], 'help_info' : 'build awtk\'s linux-fb\'s demos, value is true or false' },
  'PLATFORM' : { 'value' : None, 'type' : str.__name__, 'desc' : ['build awtk\'s linux-fb\'s operation platform'], 'help_info' : 'build awtk\'s linux-fb\'s operation platform value is linux/android, value default is linux' },
  'EXTERN_CODE' : { 'value' : None, 'type' : str.__name__, 'desc' : ['add extern code list'], 'help_info' : 'when build awtk\'s linux-fb\, user add extern code list, example is EXTERN_CODE=XXXXX/*.c,XXXXX/*.c  , use \',\' split muliple libraries' },
  'WITH_G2D' : { 'value' : False, 'type' : bool.__name__, 'desc' : ['enable g2d model '], 'help_info' : 'enable awtk\'s g2d model, value is true or false' },
  "WITH_CUSTOM_GRAPHIC_BUFFER" : { 'value' : False, 'type' : bool.__name__, 'desc' : ['use custom graphic_buffer '], 'help_info' : 'disable awtk default graphic_buffer and use custom graphic_buffer, value is true or false' },
  'WAYLAND_SCANNER_PATH' : { 'value' : None, 'type' : str.__name__, 'desc' : ['wayland_scanner path'], 'help_info' : 'set the path of wayland_scanner' },
}

CWD = os.path.normpath(os.path.abspath(os.path.dirname(__file__)));
SCRIPT_ROOT = os.path.normpath(os.path.join(CWD, '../awtk/scripts'))
sys.path.append(SCRIPT_ROOT)

if not os.path.exists(os.path.normpath(os.path.join(SCRIPT_ROOT, 'compile_config.py'))) :
  sys.exit("this is linux-fb must use new AWTK version !!!!!!!!!!")

import compile_config

def get_compile_config() :
  global INIT
  if INIT :
    return compile_config.get_curr_config()
  else :
    INIT = True
    compile_helper = compile_config.get_curr_config_for_awtk()
    return compile_helper


def init(ARGUMENTS) :
  global INIT
  INIT = True
  compile_helper = compile_config.compile_helper()
  compile_helper.set_compile_config(COMPILE_CONFIG)
  compile_helper.try_load_default_config()
  compile_helper.scons_user_sopt(ARGUMENTS)
  compile_config.set_curr_config(compile_helper)
