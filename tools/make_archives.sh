#!/usr/bin/bash

# example usage: OBJVERSION=4.9.10.8 VERSION=338 ./make_archives.sh

set -e

if [ -z "$VERSION" ]; then
	echo "Please define the environment variable VERSION and tell what version this will be (e.g. 338)" >&2
	exit 1
fi

if [ -z "$OBJVERSION" ]; then
	echo "Please define the environment variable OBJVERSION and tell what version the new definitions are (e.g. 4.9.10.8)" >&2
	exit 1
fi

SCRIPT_DIR="$(realpath $(dirname $0))"
SRC=$SCRIPT_DIR

TARGET_DIR="$(realpath .)"

TAG="v$VERSION"

curl -L -o "lc_${OBJVERSION//./}.zip" "https://github.com/legacyclonk/content/releases/download/v$OBJVERSION/lc_content.zip"

if [ -z "$GET_win32" ]; then
	GET_win32="curl -L -o /tmp/LegacyClonk.zip https://github.com/legacyclonk/LegacyClonk/releases/download/$TAG/LegacyClonk-Windows-x86.zip && unzip /tmp/LegacyClonk.zip && rm /tmp/LegacyClonk.zip"
fi

if [ -z "$GET_win64" ]; then
	GET_win64="curl -L -o /tmp/LegacyClonk.zip https://github.com/legacyclonk/LegacyClonk/releases/download/$TAG/LegacyClonk-Windows-x64.zip && unzip /tmp/LegacyClonk.zip && rm /tmp/LegacyClonk.zip"
fi

if [ -z "$GET_linux" ]; then
	GET_linux="curl -L https://github.com/legacyclonk/LegacyClonk/releases/download/$TAG/LegacyClonk-Linux-x86.tar.gz | tar xz"
fi

if [ -z "$GET_linux64" ]; then
	GET_linux64="curl -L https://github.com/legacyclonk/LegacyClonk/releases/download/$TAG/LegacyClonk-Linux-x64.tar.gz | tar xz"
fi

if [ -z "$GET_mac" ]; then
	GET_mac="curl -L -o /tmp/LegacyClonk.zip https://github.com/legacyclonk/LegacyClonk/releases/download/$TAG/LegacyClonk-Mac-x64.zip && unzip /tmp/LegacyClonk.zip && rm /tmp/LegacyClonk.zip"
fi

curl -O -L https://github.com/legacyclonk/LegacyClonk/releases/download/$TAG/System.c4g
curl -O -L https://github.com/legacyclonk/LegacyClonk/releases/download/$TAG/Graphics.c4g

for PLATFORM in "win32" "win64" "linux64" "mac"; do
	rm -rf "$TARGET_DIR/$PLATFORM"
	mkdir -p "$TARGET_DIR/$PLATFORM"
	pushd "$TARGET_DIR/$PLATFORM"
	ARCHIVE_ENGINE=lc_${VERSION}_$PLATFORM.zip
	ARCHIVE_FULL=lc_full_${VERSION}_$PLATFORM.zip
	rm -f "$ARCHIVE_ENGINE" "$ARCHIVE_FULL"
	cp "$SRC/"{System,Graphics}.c4g .
	CMD="GET_$PLATFORM"
	bash -c "${!CMD}"
	zip -r -n c4f:c4g:c4d:c4s "$TARGET_DIR/$ARCHIVE_ENGINE" *
	unzip "$SRC"/lc_${OBJVERSION//./}.zip
	zip -r -n c4f:c4g:c4d:c4s "$TARGET_DIR/$ARCHIVE_FULL" *
	popd
	rm -r "$TARGET_DIR/$PLATFORM"
done
