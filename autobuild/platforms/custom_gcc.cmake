cmake_minimum_required(VERSION 3.20)
set(CMAKE_C_COMPILER "$ENV{GCC_PREFIX}-gcc-$ENV{GCC_MAJOR}")
set(CMAKE_CXX_COMPILER "$ENV{GCC_PREFIX}-g++-$ENV{GCC_MAJOR}")

set(CMAKE_C_FLAGS_INIT "-static-libgcc")
set(CMAKE_CXX_FLAGS_INIT "-static-libgcc -static-libstdc++")
set(CMAKE_EXE_LINKER_FLAGS_INIT "-static-libgcc -static-libstdc++")
