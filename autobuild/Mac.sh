#!/bin/bash
export HOMEBREW_NO_INSTALL_CLEANUP=1
export HOMEBREW_NO_INSTALLED_DEPENDENTS_CHECK=1
brew update
brew install llvm@22 ninja

# Detect Homebrew prefix (ARM vs Intel)
HOMEBREW_PREFIX="$(brew --prefix)"

echo "CMAKE_CONFIGURE_ARGS=-DCMAKE_OSX_DEPLOYMENT_TARGET=10.9 -DCMAKE_TOOLCHAIN_FILE=$PWD/autobuild/clang_mac.cmake" >> $GITHUB_ENV

# Detect versioned LLVM (e.g. llvm@22) or plain llvm
LLVM_DIR="$(brew --prefix llvm@22 2>/dev/null || brew --prefix llvm)"

# Only apply the any_cast workaround on Intel (needed for OS X < 10.14 libc++)
if [ "$(uname -m)" != "arm64" ]; then
	cat autobuild/mac_any_cast.h >> src/C4InteractiveThread.h
fi

echo "target_include_directories(clonk BEFORE PUBLIC ${LLVM_DIR}/include/c++/v1)" >> CMakeLists.txt
cat CMakeLists.txt
