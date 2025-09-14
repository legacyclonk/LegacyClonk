#!/bin/bash
export HOMEBREW_NO_INSTALL_CLEANUP=1
export HOMEBREW_NO_INSTALLED_DEPENDENTS_CHECK=1
brew update
brew install llvm@21 ninja
echo "CMAKE_CONFIGURE_ARGS=-DCMAKE_OSX_DEPLOYMENT_TARGET=10.9 -DCMAKE_TOOLCHAIN_FILE=$PWD/autobuild/clang_mac.cmake" >> $GITHUB_ENV

cat autobuild/mac_any_cast.h >> src/C4InteractiveThread.h

echo "target_include_directories(clonk BEFORE PUBLIC /usr/local/opt/llvm/include/c++/v1)" >> CMakeLists.txt
