#!/usr/bin/bash
set -e

if [ -z "$C4GROUP" ]; then
	echo "Please define the environment variable C4GROUP to point to your c4group(64) binary" >&2
	exit 1
fi

SCRIPT_DIR="$(realpath $(dirname $0))"

if [ -z "$SRCDIR" ]; then
	SRCDIR="$SCRIPT_DIR/../planet/System.c4g"
fi

TARGET_DIR="Language.c4g"

if [ -f "$TARGET_DIR" ]; then
	rm "$TARGET_DIR"
fi

function generateLanguage {
	echo "Generating $2..."
	local THIS_TARGET_DIR
	THIS_TARGET_DIR="$TARGET_DIR/LanguageLC_$1.c4g/System.c4g"
	local THIS_TARGET_FILE
	THIS_TARGET_FILE="$THIS_TARGET_DIR/Language$2.txt"
	mkdir -p "$THIS_TARGET_DIR"
	cp "$SRCDIR/Language$1.txt" "$THIS_TARGET_FILE"
	sed -i -E -e 's/^(IDS_LANG_NAME=)(.*)$/\1LC-\2/' -e "s/^(IDS_LANG_INFO=).*\$/\\1$3/" -e "s/^(IDS_LANG_FALLBACK=).*\$/\1$4/" "$THIS_TARGET_FILE"
}

generateLanguage DE LD "Sprachpaket-Erweiterung für LegacyClonk." DE,LU,US
generateLanguage US LU "Language pack extension for LegacyClonk." US,LD,DE

"$C4GROUP" "$TARGET_DIR" -p