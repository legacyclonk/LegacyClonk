#!/bin/bash
echo "CMAKE_CONFIGURE_ARGS=$CMAKE_CONFIGURE_ARGS -A x64 -C $PWD/config.txt" >> $GITHUB_ENV
echo "VS_ARCH=amd64" >> $GITHUB_ENV
