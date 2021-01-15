#!/bin/bash

#example: ./release.sh ../awtk-examples/HelloWorld-Demo exename

EXE_NAME=demoui
APP_ROOT="../awtk"

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
python3 ../awtk/scripts/release.py ${EXE_NAME} ${APP_ROOT}
tar -czf release.tar.gz release/
