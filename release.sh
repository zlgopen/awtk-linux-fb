#!/bin/bash

#example: ./release.sh ../awtk-examples/HelloWorld-Demo/res
if [ $1 ]; then
  if [ ! -d $1 ]; then
    echo "input dir : $1 is not exist!"
	exit
  fi
  APP_ROOT=$1
else
  APP_ROOT="../awtk/demos"
fi
echo "APP_ROOT = ${APP_ROOT}" 

rm -rf release release.tar.gz

mkdir -p release/bin
mkdir -p release/assets/raw/data
mkdir -p release/assets/raw/fonts
mkdir -p release/assets/raw/images
mkdir -p release/assets/raw/images/svg
mkdir -p release/assets/raw/scripts
mkdir -p release/assets/raw/strings
mkdir -p release/assets/raw/styles
mkdir -p release/assets/raw/ui
mkdir -p release/assets/raw/xml

cp -rvf build/bin/* release/bin
rm -fv  release/bin/*test*
rm -fv  release/bin/agg**
rm -fv  release/bin/demo1
rm -fv  release/bin/demovg
rm -fv  release/bin/demotr
rm -fv  release/bin/demo_animator
rm -fv  release/bin/demo_thread
rm -fv  release/bin/demo_desktop

cp -rvf ${APP_ROOT}/assets/raw/data/* release/assets/raw/data
cp -rvf ${APP_ROOT}/assets/raw/fonts/* release/assets/raw/fonts
cp -rvf ${APP_ROOT}/assets/raw/images/x1 release/assets/raw/images/
cp -rvf ${APP_ROOT}/assets/raw/images/xx release/assets/raw/images/
cp -rvf ${APP_ROOT}/assets/raw/images/svg/*.bsvg release/assets/raw/images/svg
cp -rvf ${APP_ROOT}/assets/raw/scripts/* release/assets/raw/scripts/
cp -rvf ${APP_ROOT}/assets/raw/strings/*.bin release/assets/raw/strings/
cp -rvf ${APP_ROOT}/assets/raw/styles/*.bin release/assets/raw/styles/
cp -rvf ${APP_ROOT}/assets/raw/ui/*.bin release/assets/raw/ui/
cp -rvf ${APP_ROOT}/assets/raw/xml/* release/assets/raw/xml/

tar -czvf release.tar.gz release/
