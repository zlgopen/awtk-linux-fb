import os
import sys
import platform
import shutil
import json
import collections
import fnmatch

PRJ_DIR = os.getcwd();
OUTPUT_DIR = os.path.join(PRJ_DIR, 'release')
FILE_DIR_PATH = os.path.abspath(os.path.dirname(__file__))
BIN_DIR = os.path.join(os.getcwd(), 'bin')
AWTK_ROOT = os.path.join(FILE_DIR_PATH, '../../../awtk')

def release():
  import imp
  awtk_scripts_root = os.path.abspath(os.path.join(AWTK_ROOT, 'scripts'))
  print(awtk_scripts_root)
  sys.path.insert(0, awtk_scripts_root)
  file, path, desc = imp.find_module('release', [os.path.join(awtk_scripts_root, 'project_scripts')])
  release_helper = imp.load_module('release', file, path, desc)
  copyLinuxFBConfigFiles()

def ignore_patterns_list(patterns_list):
  def _ignore_patterns(path, names):
    ignored_names = []
    for pattern in patterns_list:
      ignored_names.extend(fnmatch.filter(names, pattern))
    return set(ignored_names)
  return _ignore_patterns

def read_file(filename):
  content = ''
  if sys.version_info >= (3, 0):
    with open(filename, 'r', encoding='utf8') as f:
      content = f.read()
  else:
    with open(filename, 'r') as f:
      content = f.read()
  return content

def copyLinuxFBConfigFiles() :
  uses_sdk_path = os.path.join(BIN_DIR, 'uses_sdk.json')
  content = read_file(uses_sdk_path)
  uses_sdk = json.loads(content, object_pairs_hook=collections.OrderedDict)
  if 'linux_fb' in uses_sdk['compileSDK']['awtk'] :
    if uses_sdk['compileSDK']['awtk']['linux_fb'] :
      dst_dir =  os.path.join(OUTPUT_DIR, 'config')
      config_dir = os.path.join(uses_sdk['compileSDK']['awtk']['path'], 'config');
      if os.path.exists(config_dir) :
        shutil.rmtree(dst_dir, True)
        ignore_files = ['*.md', '*.in']
        shutil.copytree(config_dir, dst_dir, ignore=ignore_patterns_list(ignore_files))

release()
