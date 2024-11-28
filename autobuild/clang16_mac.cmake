cmake_minimum_required(VERSION 3.20)
set(CMAKE_C_COMPILER "/usr/local/opt/llvm/bin/clang")
set(CMAKE_CXX_COMPILER "/usr/local/opt/llvm/bin/clang++")

set(CMAKE_C_FLAGS_INIT "-fexperimental-library -Wno-parentheses")
set(CMAKE_CXX_FLAGS_INIT "-nostdlib++ ${CMAKE_C_FLAGS_INIT}")
set(CMAKE_OBJCXX_FLAGS_INIT "-nostdlib++ ${CMAKE_CXX_FLAGS_INIT}")

set(CMAKE_EXE_LINKER_FLAGS_INIT "/usr/local/opt/llvm/lib/c++/libc++.a /usr/local/opt/llvm/lib/c++/libc++abi.a /usr/local/opt/llvm/lib/c++/libc++experimental.a")
set(CMAKE_SHARED_LINKER_FLAGS_INIT "/usr/local/opt/llvm/lib/c++/libc++.a /usr/local/opt/llvm/lib/c++/libc++abi.a /usr/local/opt/llvm/lib/c++/libc++experimental.a")
