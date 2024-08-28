import os
import shutil
import atexit
import scons_argv
scons_argv.init(ARGUMENTS)

import compile_config
import awtk_config as awtk

awtk.scons_db_check_and_remove()
complie_helper = compile_config.get_curr_config()

APP_CCFLAGS = ' '

AWTK_LIB_PATH = awtk.joinPath(awtk.BIN_DIR, "libawtk.so");
if os.path.exists(AWTK_LIB_PATH) :
  os.remove(AWTK_LIB_PATH)

APP_ROOT = complie_helper.get_value('APP', '')
if len (APP_ROOT) > 0:
  app_sconstruct = awtk.joinPath(APP_ROOT, 'SConstruct')
  if not os.path.exists(APP_ROOT) or not os.path.exists(app_sconstruct):
    print('APP: ' + APP_ROOT + ' not found!')
    exit(0)

env = DefaultEnvironment(CCFLAGS = awtk.CCFLAGS + APP_CCFLAGS, 
  ENV = os.environ,
  CFLAGS = awtk.CFLAGS,
  CC=awtk.CC,
  CXX=awtk.CXX,
  LD=awtk.LD,
  AR=awtk.AR,
  RANLIB=awtk.RANLIB,
  STRIP=awtk.STRIP,
  LIBS = awtk.LIBS,
  LIBPATH = awtk.LIBPATH,
  CPPPATH = awtk.CPPPATH + [awtk.joinPath(awtk.TK_ROOT, 'res')],
  LINKFLAGS = awtk.LINKFLAGS,
  OS_SUBSYSTEM_CONSOLE=awtk.OS_SUBSYSTEM_CONSOLE,
  OS_SUBSYSTEM_WINDOWS=awtk.OS_SUBSYSTEM_WINDOWS
)

TK_ROOT_VAR = awtk.joinPath(awtk.VAR_DIR, 'awtk')
VariantDir(TK_ROOT_VAR, awtk.TK_ROOT)

if APP_ROOT == '' and complie_helper.get_value('BUILD_DEMOS', True):
  APP_PROJ_VAR = [awtk.joinPath(TK_ROOT_VAR, 'demos/SConscript')]
else:
  APP_PROJ_VAR = []

if awtk.lcd_devices_is_egl(os.environ['LCD_DEVICES']) :
  APP_PROJ_VAR += [awtk.joinPath(TK_ROOT_VAR, '3rd/glad/SConscript')]

SConscriptFiles=[
  awtk.joinPath(TK_ROOT_VAR, '3rd/mbedtls/SConscript'),
  awtk.joinPath(TK_ROOT_VAR, '3rd/cjson/SConscript'),
  awtk.joinPath(TK_ROOT_VAR, '3rd/agg/SConscript'),
  awtk.joinPath(TK_ROOT_VAR, '3rd/agge/SConscript'),
  awtk.joinPath(TK_ROOT_VAR, '3rd/fribidi/SConscript'),
  awtk.joinPath(TK_ROOT_VAR, '3rd/gpinyin/SConscript'), 
  awtk.joinPath(TK_ROOT_VAR, '3rd/libunibreak/SConscript'),
  awtk.joinPath(TK_ROOT_VAR, '3rd/miniz/SConscript'),
  awtk.joinPath(TK_ROOT_VAR, 'src/SConscript'),
  awtk.joinPath(TK_ROOT_VAR, 'src/xml/SConscript'),
  awtk.joinPath(TK_ROOT_VAR, 'src/charset/SConscript'),
  awtk.joinPath(TK_ROOT_VAR, 'src/fscript_ext/SConscript'),
  awtk.joinPath(TK_ROOT_VAR, 'src/streams/SConscript'),
  awtk.joinPath(TK_ROOT_VAR, 'src/csv/SConscript'),
  awtk.joinPath(TK_ROOT_VAR, 'src/conf_io/SConscript'),
  awtk.joinPath(TK_ROOT_VAR, 'src/hal/SConscript'),
  awtk.joinPath(TK_ROOT_VAR, 'src/debugger/SConscript'),
  awtk.joinPath(TK_ROOT_VAR, 'src/ubjson/SConscript'),
  awtk.joinPath(TK_ROOT_VAR, 'src/compressors/SConscript'),
  awtk.joinPath(TK_ROOT_VAR, 'src/romfs/SConscript'),
  ] + APP_PROJ_VAR + awtk.OS_PROJECTS;

if os.environ['LCD_DEVICES'] == 'wayland' or os.environ['LCD_DEVICES'] == 'egl_for_wayland':
  SConscriptFiles += [ 'awtk-wayland/SConscript' ]
else :
  SConscriptFiles += [ 'awtk-port/SConscript' ]

os.environ['BUILD_TOOLS'] = str(complie_helper.get_value('BUILD_TOOLS', True))
if complie_helper.get_value('BUILD_TOOLS', True) :
  SConscriptFiles += [
    awtk.joinPath(TK_ROOT_VAR, 'tools/common/SConscript'), 
    awtk.joinPath(TK_ROOT_VAR, 'tools/ui_gen/xml_to_ui/SConscript'),
  ]

SConscript(SConscriptFiles)


def build_app():
  if not os.path.exists(AWTK_LIB_PATH) :
    return
  if APP_ROOT == '':
    return

  print('======================== build app ========================')

  app_bin = awtk.joinPath(APP_ROOT, 'bin')
  linux_fb_bin = os.environ['BIN_DIR'];

  if env.GetOption('clean'):
    cmd = 'cd ' + APP_ROOT + ' && scons -c AWTK_ROOT=' + awtk.TK_ROOT
    print(cmd)
    os.system(cmd)
  else:
    cmd = 'cd ' + APP_ROOT + ' && scons LINUX_FB=true AWTK_ROOT=' + awtk.TK_ROOT
    print(cmd)
    os.system(cmd)

    files = os.listdir(app_bin)
    for file in files:
      print('copy ' + awtk.joinPath(app_bin, file) + ' to ' + linux_fb_bin)
      shutil.copy(awtk.joinPath(app_bin, file), linux_fb_bin)

def compile_end() :
  complie_helper.save_last_complie_argv()
  complie_helper.output_compile_data(awtk.TK_ROOT)
  build_app()

atexit.register(compile_end)
