#!/usr/bin/bash

# example usage: OBJVERSION=4.9.10.9 VERSION=339 C4GROUP=~/clonk/c4group ./make_combined_c4us.sh

set -e

if [ -z "$C4GROUP" ]; then
	echo "Please define the environment variable C4GROUP to point to your c4group binary" >&2
	exit 1
fi

if [ -z "$VERSION" ]; then
	echo "Please define the environment variable VERSION and tell what version this will be (e.g. 339)" >&2
	exit 1
fi

if [ -z "$OBJVERSION" ]; then
	echo "Please define the environment variable OBJVERSION and tell what version the new definitions are (e.g. 4.9.10.9)" >&2
	exit 1
fi

SCRIPT_DIR="$(realpath $(dirname $0))"

TARGET_DIR="$(realpath .)"

curl -L -O "https://github.com/legacyclonk/content/releases/download/v$OBJVERSION/lc_${OBJVERSION//./}.c4u"

function win32() {
	"$C4GROUP" "$2" -et c4group.exe "$1"
}

function win64() {
	win32 "$@"
}

function linux() {
	"$C4GROUP" "$2" -et c4group "$1"
}

function linux64() {
	linux "$@"
}

function mac() {
	linux "$@"
}

for PLATFORM in "win32" "win64" "linux64" "mac"; do
	if [ -d "$TARGET_DIR/$PLATFORM" ]; then
		rm -r "$TARGET_DIR/$PLATFORM"
	fi
	mkdir -p "$TARGET_DIR/$PLATFORM"
	pushd "$TARGET_DIR/$PLATFORM"
	UPDATE=lc_${OBJVERSION//./}_${VERSION}_$PLATFORM.c4u
	mkdir $UPDATE
	cp "$TARGET_DIR/lc_${VERSION}_$PLATFORM.c4u" "$UPDATE"
	cp "$TARGET_DIR/lc_${OBJVERSION//./}.c4u" "$UPDATE"
	"$PLATFORM" "$UPDATE" "$TARGET_DIR/lc_${VERSION}_$PLATFORM.c4u"
	"$C4GROUP" "$UPDATE" -p
	mv "$UPDATE" "$TARGET_DIR"
	popd
done
