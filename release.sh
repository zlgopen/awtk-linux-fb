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
mkdir -p release/assets/default/raw/data
mkdir -p release/assets/default/raw/fonts
mkdir -p release/assets/default/raw/images
mkdir -p release/assets/default/raw/images/svg
mkdir -p release/assets/default/raw/scripts
mkdir -p release/assets/default/raw/strings
mkdir -p release/assets/default/raw/styles
mkdir -p release/assets/default/raw/ui
mkdir -p release/assets/default/raw/xml

cp -rvf build/bin/* release/bin
rm -fv  release/bin/*test*
rm -fv  release/bin/agg**
rm -fv  release/bin/demo1
rm -fv  release/bin/demovg
rm -fv  release/bin/demotr
rm -fv  release/bin/demo_animator
rm -fv  release/bin/demo_thread
rm -fv  release/bin/demo_desktop

cp -rvf ${APP_ROOT}/assets/default/raw/data/* release/assets/default/raw/data
cp -rvf ${APP_ROOT}/assets/default/raw/fonts/* release/assets/default/raw/fonts
cp -rvf ${APP_ROOT}/assets/default/raw/images/x1 release/assets/default/raw/images/
cp -rvf ${APP_ROOT}/assets/default/raw/images/xx release/assets/default/raw/images/
cp -rvf ${APP_ROOT}/assets/default/raw/images/svg/*.bsvg release/assets/default/raw/images/svg
cp -rvf ${APP_ROOT}/assets/default/raw/scripts/* release/assets/default/raw/scripts/
cp -rvf ${APP_ROOT}/assets/default/raw/strings/*.bin release/assets/default/raw/strings/
cp -rvf ${APP_ROOT}/assets/default/raw/styles/*.bin release/assets/default/raw/styles/
cp -rvf ${APP_ROOT}/assets/default/raw/ui/*.bin release/assets/default/raw/ui/
cp -rvf ${APP_ROOT}/assets/default/raw/xml/* release/assets/default/raw/xml/

tar -czvf release.tar.gz release/
