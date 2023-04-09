cmake_minimum_required(VERSION 3.20)
set(CMAKE_C_COMPILER x86_64-linux-gnu-x86_64-linux-gnu-gcc-13)
set(CMAKE_CXX_COMPILER x86_64-linux-gnu-x86_64-linux-gnu-g++-13)

set(CMAKE_C_FLAGS_INIT "-static-libgcc")
set(CMAKE_CXX_FLAGS_INIT "-static-libgcc -static-libstdc++")
set(CMAKE_EXE_LINKER_FLAGS_INIT "-static-libgcc -static-libstdc++")
