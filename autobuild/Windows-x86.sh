#!/bin/bash
echo "CMAKE_CONFIGURE_ARGS=-A Win32 -C $PWD/config.txt -DUSE_FMOD=ON" >> $GITHUB_ENV
echo "VS_ARCH=x86" >> $GITHUB_ENV
