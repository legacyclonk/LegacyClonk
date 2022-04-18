#!/bin/bash

set -e

SCRIPT_DIR="$(realpath $(dirname $0))"
SRC_ROOT=$SCRIPT_DIR/..

function check_adjust()
{
	f="$1"
	YEAR="$2"
	if not grep -q "$YEAR, The LegacyClonk Team" "$f"; then
		echo "Adjusting $f to $YEAR"
		 sed -i -E -e 's/(\(c\) )([[:digit:]]{4})-([[:digit:]]{4})(, The LegacyClonk Team)/\1\2-'$YEAR'\4/' -e 's/(\(c\) )([[:digit:]]{4})(, The LegacyClonk Team)/\1\2-'$YEAR'\3/' "$f"
	fi
}

for f in "$SRC_ROOT"/src/*.* "$SRC_ROOT"/CMakeLists.txt; do
	YEAR=$(git log -n 1 --pretty='%cd' --date='format:%Y' "$f")

	if [ -z "$YEAR" ]; then
		echo "Skipping $f (Unknown to git)"
		continue
	fi

	check_adjust "$f" "$YEAR"
done
check_adjust "$SRC_ROOT"/COPYING $(date '+%Y')
