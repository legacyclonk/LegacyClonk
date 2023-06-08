#!/bin/bash
set -e

LINUXDEPLOY_VERSION=1-alpha-20220822-1

curl -L -o linuxdeploy "https://github.com/linuxdeploy/linuxdeploy/releases/download/$LINUXDEPLOY_VERSION/linuxdeploy-x86_64.AppImage"
curl -L -O "https://raw.githubusercontent.com/linuxdeploy/linuxdeploy-plugin-gtk/master/linuxdeploy-plugin-gtk.sh"
chmod +x linuxdeploy linuxdeploy-plugin-gtk.sh

convert -resize 512x512 src/res/lc.png legacyclonk.png

OUTPUT=output/clonk.AppImage ./linuxdeploy --plugin gtk --desktop-file=src/res/io.github.legacyclonk.LegacyClonk.desktop --icon-file=legacyclonk.png --appdir=AppDir --executable=build/clonk --output=appimage
