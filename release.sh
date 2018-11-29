#!/bin/bash

APP_ROOT='awtk/demos'
#APP_ROOT='awtk-hello'
rm -rf release release.zip

mkdir -p release/bin
mkdir -p release/assets/raw/fonts
mkdir -p release/assets/raw/images
mkdir -p release/assets/raw/images/svg
mkdir -p release/assets/raw/strings
mkdir -p release/assets/raw/styles
mkdir -p release/assets/raw/ui

cp -rvf build/bin/* release/bin
rm -fv  release/bin/*test*
rm -fv  release/bin/agg**
cp -rvf ${APP_ROOT}/assets/raw/fonts/* release/assets/raw/fonts
cp -rvf ${APP_ROOT}/assets/raw/images/x1 release/assets/raw/images/
cp -rvf ${APP_ROOT}/assets/raw/images/svg/*.bsvg release/assets/raw/images/svg
cp -rvf ${APP_ROOT}/assets/raw/strings/*.bin release/assets/raw/strings/
cp -rvf ${APP_ROOT}/assets/raw/styles/*.bin release/assets/raw/styles/
cp -rvf ${APP_ROOT}/assets/raw/ui/*.bin release/assets/raw/ui/

zip -r release.zip release/

