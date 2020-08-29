#!/usr/bin/bash
# example usage: VERSION=340 OBJVERSION=4.9.10.9 C4GROUP=~/clonk/c4group ./make_group_updates.sh

set -e

if [ -z "$C4GROUP" ]; then
	echo "Please define the environment variable C4GROUP to point to your c4group binary" >&2
	exit 1
fi

if [ -z "$OBJVERSION" ]; then
	echo "Please define the environment variable OBJVERSION and tell what version the new definitions are (e.g. 4.9.10.9)" >&2
	exit 1
fi

if [ -z "$VERSION" ]; then
	echo "Please define the environment variable VERSION and tell what version the new engine is (e.g. 340)" >&2
	exit 1
fi

SCRIPT_DIR="$(realpath $(dirname $0))"
TARGET_DIR="$(realpath ./output)"

TMP=$(mktemp -d 2>/dev/null || mktemp -d -t 'make_group_updates') # https://unix.stackexchange.com/a/84980
function cleanup {
	rm -rf "$TMP"
}
trap cleanup EXIT

cd "$TMP"
for group in "$SCRIPT_DIR/parts"/*; do
	groupname=$(basename $group)
	update="$TARGET_DIR/$groupname.c4u"
	echo Generating $(basename "$update")...
	rm -f "$update"

	echo "	Downloading new $groupname..."
	curl -s -S -L -o "$SCRIPT_DIR/$groupname" "https://github.com/legacyclonk/LegacyClonk/releases/download/v$VERSION/$groupname"

	while read -r part || [[ -n "$part" ]]; do
		echo "	Downloading $part..."
		curl -s -S -L -o "$TMP/$groupname" $part
		echo "Adding support for $part..."
		"$C4GROUP" "$update" -g "$groupname" "$SCRIPT_DIR/$groupname" $OBJVERSION
	done < "$group"
	
	cp "$update" "$SCRIPT_DIR/../"
done
