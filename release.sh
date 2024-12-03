#!/bin/bash

#example: ./release.sh ../awtk-examples/HelloWorld-Demo exename

# 在应用程序目录下运行:
# ~/work/awtk-root/awtk-linux-fb/release.sh . demo

AWTK_LINUX_FB_DIR="$(dirname "${BASH_SOURCE[0]}")"
AWTK_LINUX_FB_DIR="$(cd "$AWTK_LINUX_FB_DIR" && pwd)"
AWTK_DIR=$AWTK_LINUX_FB_DIR"/../awtk"

EXE_NAME=demoui
APP_ROOT=$AWTK_DIR

if [ $1 ]; then
  if [ ! -d $1 ]; then
    echo "input dir : $1 is not exist!"
	  exit
  fi
  APP_ROOT=$1
fi

if [ $2 ]; then
  EXE_NAME=$2
fi

echo "EXE_NAME = ${EXE_NAME}" 
echo "APP_ROOT = ${APP_ROOT}" 

rm -rf release release.tar.gz
python3 $AWTK_DIR/scripts/release.py ${EXE_NAME} ${APP_ROOT}
cp -rvf $AWTK_LINUX_FB_DIR/config release/config

tar -czf release.tar.gz release/
