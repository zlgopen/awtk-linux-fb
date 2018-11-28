#!/bin/bash

rm -rf release release.zip

mkdir -p release/bin
mkdir -p release/demos/assets/raw/fonts
mkdir -p release/demos/assets/raw/images
mkdir -p release/demos/assets/raw/strings
mkdir -p release/demos/assets/raw/styles
mkdir -p release/demos/assets/raw/ui

#cp -rvf awtk/bin/demo* release/bin
#cp -rvf awtk/bin/*test release/bin

cp -rvf awtk/bin/demoui release/bin
cp -rvf awtk/demos/assets/raw/fonts/default.ttf release/demos/assets/raw/fonts
cp -rvf awtk/demos/assets/raw/images/x1 release/demos/assets/raw/images/
cp -rvf awtk/demos/assets/raw/images/svg release/demos/assets/raw/images/svg
cp -rvf awtk/demos/assets/raw/strings/*.bin release/demos/assets/raw/strings/
cp -rvf awtk/demos/assets/raw/styles/*.bin release/demos/assets/raw/styles/
cp -rvf awtk/demos/assets/raw/ui/*.bin release/demos/assets/raw/ui/

zip -r release.zip release/

