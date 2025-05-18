#!/bin/bash
echo "CMAKE_CONFIGURE_ARGS=-DWITH_DEVELOPER_MODE=On -DCMAKE_TOOLCHAIN_FILE=$PWD/autobuild/custom_gcc.cmake" >> $GITHUB_ENV
echo "CMAKE_BUILD_ARGS=-- -j$(nproc)" >> $GITHUB_ENV
