import os
import sys

global INIT
INIT = False

COMPILE_CONFIG = {
  'OS_FLAGS' : { 'value' : '', 'desc' : ['compile flags'], 'help_info' : 'set compile\'s flags, so care of system and compile tools'},
  'OS_LIBS' : { 'value' : [], 'desc' : ['compile libs'], 'help_info' : 'set compile\'s libs, so care of system and compile tools, use \',\' split muliple libraries '},
  'OS_LIBPATH' : { 'value' : [], 'desc' : ['compile lib paths'], 'help_info' : 'set compile\'s lib paths, so care of system and compile tools, use \',\' split muliple librarie\'s paths '},
  'OS_CPPPATH' : { 'value' : [], 'desc' : ['compile include paths'], 'help_info' : 'set compile\'s include paths, so care of system and compile tools, use \',\' split muliple include path '},
  'TOOLS_CC' : { 'value' : None, 'desc' : ['compile tools prefix CC\'s name'], 'help_info' : 'set compile tools prefix CC\'s name, TOOLS_CC=XXXXX, CC=TOOLS_PREFIX+TOOLS_CC '},
  'TOOLS_CXX' : { 'value' : None, 'desc' : ['compile tools prefix CXX\'s name'], 'help_info' : 'set compile tools prefix CXX\'s name, TOOLS_CXX=XXXXX, CXX=TOOLS_PREFIX+TOOLS_CXX '},
  'TOOLS_LD' : { 'value' : None, 'desc' : ['compile tools prefix LD\'s name'], 'help_info' : 'set compile tools prefix LD\'s name, TOOLS_LD=XXXXX, LD=TOOLS_PREFIX+TOOLS_LD '},
  'TOOLS_AR' : { 'value' : None, 'desc' : ['compile tools prefix AR\'s name'], 'help_info' : 'set compile tools prefix AR\'s name, TOOLS_AR=XXXXX, AR=TOOLS_PREFIX+TOOLS_AR '},
  'TOOLS_STRIP' : { 'value' : None, 'desc' : ['compile tools prefix STRIP\'s name'], 'help_info' : 'set compile tools prefix STRIP\'s name, TOOLS_STRIP=XXXXX, STRIP=TOOLS_PREFIX+TOOLS_STRIP '},
  'TOOLS_RANLIB' : { 'value' : None, 'desc' : ['compile tools prefix RANLIB\'s name'], 'help_info' : 'set compile tools prefix RANLIB\'s name, TOOLS_RANLIB=XXXXX, RANLIB=TOOLS_PREFIX+TOOLS_RANLIB '},
  'TOOLS_PREFIX' : { 'value' : None, 'desc' : ['compile tools prefix path'], 'help_info' : 'set compile tools prefix path, value allow set \'\''},
  'TSLIB_LIB_DIR' : { 'value' : None, 'desc' : ['tslib lib path'], 'help_info' : 'set use tslib lib path, value allow set \'\''},
  'TSLIB_INC_DIR' : { 'value' : None, 'desc' : ['tslib include path'], 'help_info' : 'set use tslib lib path, value allow set \'\''},
  'ENABLE_CURSOR' : { 'value' : True, 'desc' : ['enable cursor mouse'], 'help_info' : 'set enable cursor mouse, value is true or false'},
  'INPUT_ENGINE' : { 'value' : '', 'desc' : ['null/spinyin/t9/t9ext/pinyin'], 'help_info' : 'set awtk use input engine' },
  'VGCANVAS' : { 'value' : '', 'desc' : ['NANOVG/NANOVG_PLUS/CAIRO'], 'help_info' : 'set awtk use render vgcanvas type' },
  'DEBUG' : { 'value' : False, 'desc' : ['awtk\'s compile is debug'], 'help_info' : 'awtk\'s compile is debug, value is true or false' },
  'APP' : { 'value' : '', 'save_file' : False, 'desc' : ['build this app'], 'help_info' : 'set app is build, value is app\'s root' },
  'LCD_DEVICES' : { 'value' : 'fb', 'desc' : ['linux\'s lcd devices type, value is fb/drm/egl_for_fsl/egl_for_x11/egl_for_gbm'], 'help_info' : 'when building, use linux\'s lcd devices type, value is fb/drm/egl_for_fsl/egl_for_x11/egl_for_gbm' },
  'BUILD_TOOLS' : { 'value' : True, 'desc' : ['build awtk\'s linux-fb\'s tools'], 'help_info' : 'build awtk\'s linux-fb\'s tools, value is true or false' },
  'BUILD_DEMOS' : { 'value' : True, 'desc' : ['build awtk\'s linux-fb\'s demos'], 'help_info' : 'build awtk\'s linux-fb\'s demos, value is true or false' },
  'PLATFORM' : { 'value' : False, 'desc' : ['build awtk\'s linux-fb\'s operation platform'], 'help_info' : 'build awtk\'s linux-fb\'s operation platform value is linux/android, value default is linux' },
  'EXTERN_CODE' : { 'value' : None, 'desc' : ['add extern code'], 'help_info' : 'when build awtk\'s linux-fb\, user add extern code, example is EXTERN_CODE=XXXXX/*.c' },
  'WITH_G2D' : { 'value' : False, 'desc' : ['enable g2d model '], 'help_info' : 'enable awtk\'s g2d model, value is true or false' },
}

CWD = os.path.normpath(os.path.abspath(os.path.dirname(__file__)));
SCRIPT_ROOT = os.path.normpath(os.path.join(CWD, '../awtk/scripts'))
sys.path.append(SCRIPT_ROOT)

if not os.path.exists(os.path.normpath(os.path.join(SCRIPT_ROOT, 'compile_config.py'))) :
  sys.exit("this is linux-fb must use new AWTK version !!!!!!!!!!")

import compile_config

def set_compile_config() :
  global INIT
  if INIT :
    return compile_config.get_curr_config()
  else :
    os.chdir(CWD)
    INIT = True
    complie_helper = compile_config.complie_helper()
    complie_helper.set_compile_config(COMPILE_CONFIG)
    complie_helper.try_load_default_config()
    compile_config.set_curr_config(complie_helper)
    return complie_helper


def init(ARGUMENTS) :
  global INIT
  INIT = True
  complie_helper = compile_config.complie_helper()
  complie_helper.set_compile_config(COMPILE_CONFIG)
  complie_helper.try_load_default_config()
  complie_helper.scons_user_sopt(ARGUMENTS)
  compile_config.set_curr_config(complie_helper)
