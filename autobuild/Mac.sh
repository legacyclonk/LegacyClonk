#!/bin/bash
export HOMEBREW_NO_INSTALL_CLEANUP=1
export HOMEBREW_NO_INSTALLED_DEPENDENTS_CHECK=1
brew update
brew install llvm@16 ninja
echo "CMAKE_CONFIGURE_ARGS=-DCMAKE_OSX_DEPLOYMENT_TARGET=10.9 -DCMAKE_TOOLCHAIN_FILE=$PWD/autobuild/clang16_mac.cmake" >> $GITHUB_ENV

cat autobuild/mac_any_cast.h >> src/C4InteractiveThread.h
