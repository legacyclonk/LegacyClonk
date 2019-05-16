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

if [ -f System.c4g ]; then
	rm System.c4g
fi

cp -r "$SRCDIR" .

"$C4GROUP" System.c4g -p