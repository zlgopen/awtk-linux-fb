import os
import os.path
import platform
import shutil
import compile_config
compile_helper = compile_config.get_curr_config_for_awtk()

import sys
if sys.version_info.major == 2:
  import cPickle as pickle
else:
  import pickle

OS_NAME = 'Linux'

import subprocess

def is_raspberrypi():
  try:
    result = str(subprocess.check_output(["uname", "-a"]))
    return result.find('Linux raspberrypi') >= 0
  except Exception as e:
    return False

#######################################################
# XXX: This file can be edited only in tkc project
#######################################################

TOOLS_PREFIX = ''
MACH = platform.machine()
ARCH = platform.architecture()
is32bit = (ARCH[0] == '32bit')

if is32bit:
    if MACH == 'i686' or MACH == 'i386' or MACH == 'x86':
        TARGET_ARCH = 'x86'
    else:
        TARGET_ARCH = ''
else:
    TARGET_ARCH = ''

print('MACH=' + MACH + ' ARCH=' + str(ARCH) + ' TARGET_ARCH=' + TARGET_ARCH)


def addLibPrefix(iter):
    print(iter)
    return '-l'+iter


def toWholeArchive(libs):
    wa = ' '.join(list(map(addLibPrefix, libs)))
    return ' -Wl,--whole-archive ' + wa + ' -Wl,--no-whole-archive'


def getTkcOnly():
    env = os.environ
    if 'TKC_ONLY' in env:
        return env['TKC_ONLY'] == 'True'
    else:
        return False


def joinPath(root, subdir):
    return os.path.normpath(os.path.join(root, subdir))


TK_ROOT = os.path.dirname(os.path.normpath(os.path.abspath(__file__)))

print('TK_ROOT: ' + TK_ROOT)

TK_SRC = joinPath(TK_ROOT, 'src')
TK_BIN_DIR = joinPath(TK_ROOT, 'bin')
TK_LIB_DIR = joinPath(TK_ROOT, 'lib')
TK_3RD_ROOT = joinPath(TK_ROOT, '3rd')
TK_TOOLS_ROOT = joinPath(TK_ROOT, 'tools')
TK_DEMO_ROOT = joinPath(TK_ROOT, 'demos')
GTEST_ROOT = joinPath(TK_ROOT, '3rd/gtest/googletest')
TKC_STATIC_LIBS = ['debugger', 'fscript_ext', 'romfs', 'conf_io', 'hal', 'xml', 'charset',
                   'csv', 'streams', 'ubjson', 'compressors', 'miniz', 'tkc_core', 'mbedtls']

TOOLS_NAME = ''
NANOVG_BACKEND = ''
NATIVE_WINDOW = ''
#TOOLS_NAME = 'mingw'
TOOLS_NAME = compile_helper.get_value('TOOLS_NAME', TOOLS_NAME)

if OS_NAME == 'Windows':
    TK_ROOT = TK_ROOT.replace('\\', '\\\\')

OS_FLAGS = ''
OS_LIBS = []
OS_LIBPATH = []
OS_CPPPATH = []
OS_LINKFLAGS = ''
OS_SUBSYSTEM_CONSOLE = ''
OS_SUBSYSTEM_WINDOWS = ''
OS_PROJECTS = []
# only for c compiler flags
COMMON_CFLAGS = ''
OS_DEBUG = compile_helper.get_value('DEBUG', True)

if OS_NAME == 'Darwin':

    # Find the correct SDL2 version
    sdl_path = "/opt/homebrew/Cellar/sdl2/" 
    if not os.path.exists(sdl_path) :
        # Compatible with old systems
        sdl_path = '/usr/local/Cellar/sdl2/'
    sdl_versions = os.listdir(sdl_path)
    if len(sdl_versions)==0:
        print("Can not find the SDL version")
        exit(-1)
    sdl_lib = sdl_path + sdl_versions[0] + "/lib"

    TOOLS_NAME = ''
    OS_FLAGS = '-Wall -Wno-unused-function -fPIC -DWITHOUT_GLAD=1 '
    OS_LIBS = ['stdc++', 'iconv', 'pthread', 'm', 'dl']
    OS_LINKFLAGS = '-framework IOKit -framework Cocoa -framework QuartzCore -framework OpenGL -weak_framework Metal -weak_framework MetalKit'
    OS_FLAGS = OS_FLAGS + ' -DHAS_SEM_OPEN '
    OS_FLAGS = OS_FLAGS + ' -D__APPLE__ -DHAS_PTHREAD -DMACOS '
    OS_FLAGS = OS_FLAGS + ' -D__STDC_LIMIT_MACROS -D__STDC_FORMAT_MACROS -D__STDC_CONSTANT_MACROS  -DBGFX_CONFIG_RENDERER_METAL=1 '
    OS_LIBPATH = ['/usr/local/lib/', sdl_lib]

elif OS_NAME == 'Linux':
    TOOLS_NAME = ''
    OS_FLAGS = ' -Wall -Wno-unused-function -fPIC '
    OS_LIBS = ['sndio', 'stdc++', 'asound', 'pthread', 'm', 'dl']
    COMMON_CFLAGS = COMMON_CFLAGS+' -std=gnu99 '
    OS_FLAGS = OS_FLAGS + ' -DLINUX -DHAS_PTHREAD'

    OS_PROJECTS = ['3rd/SDL/SConscript']
    if TARGET_ARCH == 'x86':
        OS_FLAGS = OS_FLAGS + ' -U__FLT_EVAL_METHOD__ -D__FLT_EVAL_METHOD__=0 '
    else:
        OS_FLAGS = OS_FLAGS + ' -DWITH_64BIT_CPU '

    OS_LINKFLAGS = ' -Wl,-rpath=./bin -Wl,-rpath=./ '
    if is_raspberrypi():
      OS_FLAGS = OS_FLAGS + ' -DRASPBERRYPI '
      os.environ['RASPBERRYPI'] = 'true'
    
    SDL_VIDEODRIVER=os.getenv('SDL_VIDEODRIVER');
    if SDL_VIDEODRIVER is None:
      SDL_VIDEODRIVER = 'x11'
  
    if SDL_VIDEODRIVER == 'wayland':
      OS_LIBS = ['GL', 'xkbcommon', 'wayland-cursor', 'wayland-egl', 'wayland-client'] + OS_LIBS;
      OS_FLAGS += '-DWITHOUT_NATIVE_FILE_DIALOG '
    else:
      OS_LIBS = ['GL', 'gtk-3', 'gdk-3', 'glib-2.0', 'gobject-2.0', 'Xext', 'X11'] + OS_LIBS;


elif OS_NAME == 'Windows':
    if not os.path.exists(os.path.abspath(TK_BIN_DIR)):
        os.makedirs(os.path.abspath(TK_BIN_DIR))
    if not os.path.exists(os.path.abspath(TK_LIB_DIR)):
        os.makedirs(os.path.abspath(TK_LIB_DIR))
    if TOOLS_NAME == '':
        OS_LIBS = ['gdi32', 'user32', 'winmm.lib', 'imm32.lib', 'version.lib', 'shell32.lib', 'Setupapi',
                   'ole32.lib', 'Oleaut32.lib', 'Advapi32.lib', 'DelayImp.lib', 'psapi.lib', "ws2_32"]
        OS_FLAGS = '-DWIN32 -D_WIN32 -DWINDOWS /EHsc -D_CONSOLE   /FS /Z7 /utf-8 '
        if TARGET_ARCH == 'x86':
            OS_LINKFLAGS = '/MACHINE:X86 '
            OS_SUBSYSTEM_CONSOLE = '/SUBSYSTEM:CONSOLE,5.01  '
            OS_SUBSYSTEM_WINDOWS = '/SUBSYSTEM:WINDOWS,5.01  '
            OS_FLAGS = OS_FLAGS + ' -D_WIN32 '
        else:
            OS_FLAGS = OS_FLAGS + ' -DWITH_64BIT_CPU '
            OS_LINKFLAGS = '/MACHINE:X64 '
            OS_SUBSYSTEM_CONSOLE = '/SUBSYSTEM:CONSOLE  '
            OS_SUBSYSTEM_WINDOWS = '/SUBSYSTEM:WINDOWS  '
            OS_FLAGS = OS_FLAGS + ' -D_WIN64 '
        OS_FLAGS = OS_FLAGS + ' -DHAVE_LIBC '

    elif TOOLS_NAME == 'mingw':
        OS_LIBS = ['kernel32', 'gdi32', 'user32', 'winmm', 'imm32', 'version', 'shell32', 'Setupapi',
                   'ole32', 'Oleaut32', 'Advapi32', 'oleaut32', 'uuid', 'stdc++', "ws2_32"]
        OS_FLAGS = '-DMINGW -DWINDOWS -D_CONSOLE  -Wall'
        OS_LINKFLAGS = ' -Wl,-rpath=./bin -Wl,-rpath=./ '
        OS_SUBSYSTEM_CONSOLE = ' -mconsole  '
        OS_SUBSYSTEM_WINDOWS = ' -mwindows  '
        COMMON_CFLAGS = COMMON_CFLAGS+' -std=gnu99 '
        OS_FLAGS = OS_FLAGS+' -U__FLT_EVAL_METHOD__ -D__FLT_EVAL_METHOD__=0 -DDECLSPEC=  '

    #OS_FLAGS='-DWIN32 -D_WIN32 -DWINDOWS /EHsc -D_CONSOLE  /DEBUG /Od  /FS /Z7 -D_DEBUG /MDd '
    OS_FLAGS = OS_FLAGS + '-D__STDC_LIMIT_MACROS -D__STDC_FORMAT_MACROS -D__STDC_CONSTANT_MACROS -D_HAS_EXCEPTIONS=0 -D_HAS_ITERATOR_DEBUGGING=0 -D_ITERATOR_DEBUG_LEVEL=0 -D_SCL_SECURE=0'
    OS_FLAGS = OS_FLAGS + \
        '-D_SECURE_SCL=0 -D_SCL_SECURE_NO_WARNINGS -D_CRT_SECURE_NO_WARNINGS -D_CRT_SECURE_NO_DEPRECATE '
    OS_PROJECTS = ['3rd/SDL/SConscript']


def has_custom_cc():
    return False


def cleanSharedLib(dst, name):
    if OS_NAME == 'Darwin':
        dst = os.path.join(dst, 'lib'+name+'.dylib')
    elif OS_NAME == 'Linux':
        dst = os.path.join(dst, 'lib'+name+'.so')
    elif OS_NAME == 'Windows':
        dst = os.path.join(dst, name+'.dll')
    else:
        print('not support ' + OS_NAME)
        return

    dst = os.path.normpath(dst)

    if os.path.exists(dst):
        os.remove(dst)
        print('Removed ' + dst)

    if OS_NAME == 'Windows':
        dst=dst.replace('.dll', '.lib')
        if os.path.exists(dst):
            os.remove(dst)
            print('Removed ' + dst)


def copySharedLib(src, dst, name):
    mingw_src = src
    if OS_NAME == 'Darwin':
        src = os.path.join(src, 'bin/lib'+name+'.dylib')
    elif OS_NAME == 'Linux':
        src = os.path.join(src, 'bin/lib'+name+'.so')
    elif OS_NAME == 'Windows':
        src = os.path.join(src, 'bin/'+name+'.dll')
    else:
        print('not support ' + OS_NAME)
        return

    src = os.path.normpath(src)
    dst = os.path.normpath(dst)

    if os.path.dirname(src) == dst:
        return

    if not os.path.exists(src):
        print('Can\'t find ' + src + '. Please build '+name+' before!')
    else:
        if not os.path.exists(dst):
            os.makedirs(dst)
        shutil.copy(src, dst)
        print(src + '==>' + dst)
    if OS_NAME == 'Windows':
        if TOOLS_NAME == 'mingw':
            src = os.path.join(mingw_src, 'bin/lib'+name+'.a')
        else:
            src = src.replace('dll', 'lib')
        if os.path.exists(src):
            shutil.copy(src, dst)
            print(src + '==>' + dst)


def isBuildShared():
    return 'WITH_AWTK_SO' in os.environ and os.environ['WITH_AWTK_SO'] == 'true'


def genIdlAndDefEx(withAWTK):
    cmds = [
        'node tools/idl_gen/tkc.js tools/idl_gen/tkc.json',
        'node tools/dll_def_gen/index.js tools/idl_gen/tkc.json  dllexports/tkc.def false'
    ]
    if withAWTK:
        cmds += [
            'node tools/idl_gen/index.js tools/idl_gen/idl.json',
            'node tools/dll_def_gen/index.js tools/idl_gen/idl.json  dllexports/awtk.def false',
        ]

    for cmd in cmds:
        print(cmd)
        if os.system(cmd) != 0:
            print('exe cmd: ' + cmd + ' failed.')


os.environ['TK_ROOT'] = TK_ROOT
os.environ['TOOLS_NAME'] = TOOLS_NAME
os.environ['GTEST_ROOT'] = GTEST_ROOT
os.environ['TK_3RD_ROOT'] = TK_3RD_ROOT


def setEnvSpawn(env):
    if env['PLATFORM'] == 'win32' and os.environ['TOOLS_NAME'] != '':
        import subprocess

        class ourSpawn:
            def ourspawn(self, sh, escape, cmd, args, env):
                newargs = ' '.join(args[1:])
                cmdline = cmd + " " + newargs
                startupinfo = subprocess.STARTUPINFO()
                startupinfo.dwFlags |= subprocess.STARTF_USESHOWWINDOW
                proc = subprocess.Popen(cmdline, stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                                        stderr=subprocess.PIPE, startupinfo=startupinfo, shell=False, env=env)
                data, err = proc.communicate()
                rv = proc.wait()
                if rv:
                    print("=====")
                    print(err)
                    print("=====")
                return rv

        buf = ourSpawn()
        buf.ourenv = env
        env['SPAWN'] = buf.ourspawn


def genDllLinkFlags(libs, defFile):
    linkFlags = ''
    wholeArch = toWholeArchive(libs)

    if OS_NAME == 'Windows':
        if TOOLS_NAME == '':
            linkFlags += ' /DEF:"dllexports/'+defFile+'.def" '
        elif TOOLS_NAME == 'mingw':
            linkFlags += wholeArch
    elif OS_NAME == 'Darwin':
        linkFlags += ' -all_load '
    elif OS_NAME == 'Linux':
        linkFlags += wholeArch

    return linkFlags


def get_scons_db_files(root):
  scons_db_files = []
  scons_db_filename = ".sconsign.dblite"

  for f in os.listdir(root):
    full_path = joinPath(root, f)
    if os.path.isfile(full_path) and f == scons_db_filename:
      scons_db_files.append(full_path)
    elif os.path.isdir(full_path) and f != "." and f != "..":
      get_scons_db_files(full_path)

  return scons_db_files


def scons_db_check_and_remove():
  scons_db_files = []
  scons_db_files = get_scons_db_files(TK_ROOT)

  for f in scons_db_files:
    try:
      with open(f, "rb") as fs:
        pickle.load(fs)
        fs.close()
    except Exception as e :
        fs.close()
        print(e)
        try:
            os.remove(f)
        except Exception as e :
            print(e)
