#!/bin/bash

echo "CMAKE_CONFIGURE_ARGS=$CMAKE_CONFIGURE_ARGS -DCMAKE_OSX_DEPLOYMENT_TARGET=10.13" >> $GITHUB_ENV

curl -L https://github.com/phracker/MacOSX-SDKs/releases/download/11.3/MacOSX10.13.sdk.tar.xz | tar xJ
echo "OSX_SDK=$PWD/MacOSX10.13.sdk" >> $GITHUB_ENV

mkdir clang-21
pushd clang-21

curl -L https://packages.macports.org/clang-21/clang-21-21.1.8_1+analyzer.darwin_17.x86_64.tbz2 | tar --strip-components 3 -x -j

echo "LLVM_CXX_INCLUDE=$PWD/libexec/llvm-21/include/c++/v1" >> $GITHUB_ENV
echo "LLVM_CXX_LIB=$PWD/libexec/llvm-21/lib/libc++" >> $GITHUB_ENV

popd
