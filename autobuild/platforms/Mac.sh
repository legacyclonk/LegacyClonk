#!/bin/bash

export HOMEBREW_NO_INSTALL_CLEANUP=1
export HOMEBREW_NO_INSTALLED_DEPENDENTS_CHECK=1
brew update
brew install "llvm@$LLVM_VERSION" ninja
LLVM_PREFIX="$(brew --prefix "llvm@$LLVM_VERSION")"

echo "LLVM_PREFIX=$LLVM_PREFIX" >> $GITHUB_ENV

echo "CMAKE_CONFIGURE_ARGS=-DCMAKE_TOOLCHAIN_FILE=$PWD/autobuild/platforms/clang_mac.cmake" >> $GITHUB_ENV

cat autobuild/platforms/mac_any_cast.h >> src/C4InteractiveThread.h

echo "target_include_directories(clonk BEFORE PUBLIC /usr/local/opt/llvm/include/c++/v1)" >> CMakeLists.txt
cat CMakeLists.txt

# Work around libpng mess
if [ -d /Library/Frameworks/Mono.framework/Headers ]; then
	sudo rm -r /Library/Frameworks/Mono.framework/Headers
else
	echo "Mono headers not found, skipping removal."
fi
