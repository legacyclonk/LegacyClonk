#!/usr/bin/bash
convert +antialias -background none -density 384 lc.svg -define icon:auto-resize lc.ico
convert +antialias -background none -density 384 lc.svg lc.xpm
sed -i -E 's/^static char \*lc\[\] = \{$/static const char *c4x_xpm[] = {/' lc.xpm