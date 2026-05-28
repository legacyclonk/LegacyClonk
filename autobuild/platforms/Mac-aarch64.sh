#!/bin/bash

echo "CMAKE_CONFIGURE_ARGS=$CMAKE_CONFIGURE_ARGS -DCMAKE_OSX_DEPLOYMENT_TARGET=11.3" >> $GITHUB_ENV

mkdir clang-21
pushd clang-21

curl -L "$BUILD_TOOL_URL/clang-21-21.1.8_1+analyzer.darwin_20.arm64.tbz2" | tar --strip-components 3 -x -j

LLVM_CXX_INCLUDE=$PWD/libexec/llvm-21/include/c++/v1
LLVM_CXX_LIB=$PWD/libexec/llvm-21/lib/libc++

echo "LLVM_CXX_INCLUDE=$LLVM_CXX_INCLUDE" >> $GITHUB_ENV
echo "LLVM_CXX_LIB=$LLVM_CXX_LIB" >> $GITHUB_ENV

LIBCLANG_DIR="$PWD/libexec/llvm-21/lib/clang/21/lib/darwin"
echo "LIBCLANG_RT=$LIBCLANG_DIR/libclang_rt.profile_osx.a $LIBCLANG_DIR/libclang_rt.fuzzer_interceptors_osx.a $LIBCLANG_DIR/libclang_rt.fuzzer_osx.a $LIBCLANG_DIR/libclang_rt.osx.a $LIBCLANG_DIR/libclang_rt.cc_kext.a $LIBCLANG_DIR/libclang_rt.fuzzer_no_main_osx.a" >> $GITHUB_ENV

popd
