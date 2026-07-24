#!/bin/bash
set -e

LINUXDEPLOY_VERSION=1-alpha-20251107-1

curl -L -o linuxdeploy "https://github.com/linuxdeploy/linuxdeploy/releases/download/$LINUXDEPLOY_VERSION/linuxdeploy-$GCC_ARCH.AppImage"
curl -L -O "https://raw.githubusercontent.com/linuxdeploy/linuxdeploy-plugin-gtk/master/linuxdeploy-plugin-gtk.sh"
chmod +x linuxdeploy linuxdeploy-plugin-gtk.sh

convert -resize 512x512 src/res/lc.png legacyclonk.png

WITH_DEVELOPER_MODE=$(cmake -B build -LA -N | grep WITH_DEVELOPER_MODE | cut -d'=' -f 2)
if [ "$WITH_DEVELOPER_MODE" = "ON" ]; then
	GTK_PLUGIN="--plugin=gtk"
else
	GTK_PLUGIN=""
fi

OUTPUT=output/clonk.AppImage ./linuxdeploy $GTK_PLUGIN --desktop-file=src/res/io.github.legacyclonk.LegacyClonk.desktop --icon-file=legacyclonk.png --appdir=AppDir --executable=build/clonk --output=appimage
