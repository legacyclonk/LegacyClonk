#!/bin/bash
brew update
brew install llvm@16
echo "CMAKE_CONFIGURE_ARGS=-GNinja -DCMAKE_BUILD_TYPE=RelWithDebInfo -DCMAKE_OSX_DEPLOYMENT_TARGET=10.9 -DCMAKE_TOOLCHAIN_FILE=$PWD/autobuild/clang16_mac.cmake" >> $GITHUB_ENV

cat autobuild/mac_any_cast.h >> src/C4InteractiveThread.h
