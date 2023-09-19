cmake_minimum_required(VERSION 3.20)
set(CMAKE_C_COMPILER "/usr/local/opt/llvm/bin/clang")
set(CMAKE_CXX_COMPILER "/usr/local/opt/llvm/bin/clang++")

set(CMAKE_C_FLAGS_INIT "-stdlib=libc++ -fexperimental-library -Wno-parentheses")
set(CMAKE_CXX_FLAGS_INIT "${CMAKE_C_FLAGS_INIT}")
set(CMAKE_OBJCXX_FLAGS_INIT "${CMAKE_CXX_FLAGS_INIT}")

set(CMAKE_EXE_LINKER_FLAGS_INIT "-L/usr/local/opt/llvm/lib/c++")
set(CMAKE_SHARED_LINKER_FLAGS_INIT "-L/usr/local/opt/llvm/lib/c++")
