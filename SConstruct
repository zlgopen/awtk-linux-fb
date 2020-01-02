import os
import platform

import awtk_config as awtk

DefaultEnvironment(CCFLAGS = awtk.CCFLAGS, 
  CFLAGS = awtk.CFLAGS,
  CC=awtk.CC,
  CXX=awtk.CXX,
  LD=awtk.LD,
  AR=awtk.AR,
  STRIP=awtk.STRIP,
  LIBS = awtk.LIBS,
  LIBPATH = awtk.LIBPATH,
  CPPPATH = awtk.CPPPATH,
  LINKFLAGS = awtk.LINKFLAGS,
  OS_SUBSYSTEM_CONSOLE=awtk.OS_SUBSYSTEM_CONSOLE,
  OS_SUBSYSTEM_WINDOWS=awtk.OS_SUBSYSTEM_WINDOWS
)

APP_ROOT=ARGUMENTS.get('APP', '')
TK_ROOT_VAR = awtk.joinPath(awtk.VAR_DIR, 'awtk')
VariantDir(TK_ROOT_VAR, awtk.TK_ROOT)

if APP_ROOT == '':
  APP_PROJ_VAR = [awtk.joinPath(TK_ROOT_VAR, 'demos/SConscript')]
else:
  (APP_PATH, APP_NAME) = os.path.split(APP_ROOT)
  APP_ROOT_VAR = awtk.joinPath(awtk.VAR_DIR, APP_NAME)
  APP_PROJ_VAR = [awtk.joinPath(APP_ROOT_VAR, 'src/SConscript')]
  VariantDir(APP_ROOT_VAR, APP_ROOT)

SConscriptFiles=[
  awtk.joinPath(TK_ROOT_VAR, '3rd/nanovg/SConscript'),
  awtk.joinPath(TK_ROOT_VAR, '3rd/agg/SConscript'),
  awtk.joinPath(TK_ROOT_VAR, '3rd/agge/SConscript'),
  awtk.joinPath(TK_ROOT_VAR, '3rd/gpinyin/SConscript'), 
  awtk.joinPath(TK_ROOT_VAR, '3rd/libunibreak/SConscript'),
  awtk.joinPath(TK_ROOT_VAR, '3rd/miniz/SConscript'),
  awtk.joinPath(TK_ROOT_VAR, 'src/SConscript'),
  awtk.joinPath(TK_ROOT_VAR, 'src/streams/SConscript'),
  awtk.joinPath(TK_ROOT_VAR, 'src/ubjson/SConscript'),
  awtk.joinPath(TK_ROOT_VAR, 'src/compressors/SConscript'),
  awtk.joinPath(TK_ROOT_VAR, 'tools/common/SConscript'), 
  awtk.joinPath(TK_ROOT_VAR, 'tools/ui_gen/xml_to_ui/SConscript'),
  'awtk-port/SConscript',
  ] + APP_PROJ_VAR;

SConscript(SConscriptFiles)
