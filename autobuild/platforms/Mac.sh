#!/bin/bash

export HOMEBREW_NO_INSTALL_CLEANUP=1
export HOMEBREW_NO_INSTALLED_DEPENDENTS_CHECK=1
brew update
brew install "llvm@$LLVM_VERSION" ninja
LLVM_PREFIX="$(brew --prefix "llvm@$LLVM_VERSION")"
NINJA_PREFIX="$(brew --prefix ninja)"

echo "LLVM_PREFIX=$LLVM_PREFIX" >> $GITHUB_ENV
echo "NINJA_PREFIX=$NINJA_PREFIX" >> $GITHUB_ENV

TARGET_OSX_VERSION=11.3
echo "CMAKE_CONFIGURE_ARGS=-DCMAKE_TOOLCHAIN_FILE=$PWD/autobuild/platforms/clang_mac.cmake  -DCMAKE_OSX_DEPLOYMENT_TARGET=$TARGET_OSX_VERSION" >> $GITHUB_ENV

cat autobuild/platforms/mac_any_cast.h >> src/C4InteractiveThread.h

# Work around libpng mess
if [ -d /Library/Frameworks/Mono.framework/Headers ]; then
	sudo rm -r /Library/Frameworks/Mono.framework/Headers
else
	echo "Mono headers not found, skipping removal."
fi

BUILD_TOOL_URL="https://github.com/legacyclonk/deps/releases/download/2026-05-31-tools"

mkdir "clang-$LLVM_VERSION"
pushd "clang-$LLVM_VERSION"

curl -L "$BUILD_TOOL_URL/clang-22-22.1.6_0+analyzer.darwin_20.x86_64.tbz2" | tar --strip-components 3 -x -j

LLVM_CXX_INCLUDE="$PWD/libexec/llvm-$LLVM_VERSION/include/c++/v1"
LLVM_CXX_LIB="$PWD/libexec/llvm-$LLVM_VERSION/lib/libc++"

echo "LLVM_CXX_INCLUDE=$LLVM_CXX_INCLUDE" >> $GITHUB_ENV
echo "LLVM_CXX_LIB=$LLVM_CXX_LIB" >> $GITHUB_ENV

LIBCLANG_DIR="$PWD/libexec/llvm-$LLVM_VERSION/lib/clang/$LLVM_VERSION/lib/darwin"
echo "LIBCLANG_RT=$LIBCLANG_DIR/libclang_rt.osx.a" >> $GITHUB_ENV

popd
