#!/bin/bash

TARGET_OSX_VERSION=14.0
echo "CMAKE_CONFIGURE_ARGS=$CMAKE_CONFIGURE_ARGS -DCMAKE_OSX_DEPLOYMENT_TARGET=$TARGET_OSX_VERSION" >> $GITHUB_ENV

mkdir "clang-$LLVM_VERSION"
pushd "clang-$LLVM_VERSION"

curl -L "$BUILD_TOOL_URL/excerpt-llvm-22.1.6.arm64_sonoma.bottle.tbz2" | tar --strip-components 2 -x -j

LLVM_CXX_INCLUDE="$PWD/include/c++/v1"
LLVM_CXX_LIB="$PWD/lib/c++"

echo "LLVM_CXX_INCLUDE=$LLVM_CXX_INCLUDE" >> $GITHUB_ENV
echo "LLVM_CXX_LIB=$LLVM_CXX_LIB" >> $GITHUB_ENV

LIBCLANG_DIR="$PWD/lib/clang/$LLVM_VERSION/lib/darwin"
LIBCLANG_RT="$LIBCLANG_DIR/libclang_rt.osx.a"
echo "LIBCLANG_RT=$LIBCLANG_RT" >> $GITHUB_ENV

popd
