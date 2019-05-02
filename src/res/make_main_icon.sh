#!/usr/bin/bash
convert +antialias -background none -density 384 lc.svg -define icon:auto-resize lc.ico
convert +antialias -background none -density 384 lc.svg lc.xpm
sed -i -E 's/^static char \*lc\[\] = \{$/static const char *c4x_xpm[] = {/' lc.xpm

mkdir -p icns_src
pushd icns_src
for density in 24 48 192 384 767 1536; do
	convert +antialias -background none -density $density ../lc.svg lc_$density.png
done
png2icns ../lc.icns *.png
popd
rm -r icns_src