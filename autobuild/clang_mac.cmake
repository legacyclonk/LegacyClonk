cmake_minimum_required(VERSION 3.20)

# -------------------------------
# COMPILERS (from Homebrew LLVM)
# -------------------------------
set(CMAKE_C_COMPILER "/usr/local/opt/llvm/bin/clang" CACHE STRING "")
set(CMAKE_CXX_COMPILER "/usr/local/opt/llvm/bin/clang++" CACHE STRING "")

# explicit ninja path (helps CI)
set(CMAKE_MAKE_PROGRAM "/usr/local/opt/ninja/bin/ninja" CACHE FILEPATH "Path to Ninja")

# -------------------------------------
# macOS target + sysroot (no Xcode)
# -------------------------------------
set(CMAKE_OSX_DEPLOYMENT_TARGET "10.9" CACHE STRING "")
set(CMAKE_OSX_SYSROOT "/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk" CACHE STRING "")

# -------------------------------------
# libc++ include + linker setup
# -------------------------------------
set(LLVM_CXX_INCLUDE "/usr/local/opt/llvm/include/c++/v1")
set(LLVM_CXX_LIB     "/usr/local/opt/llvm/lib/c++")

# Add include paths
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++ -I${LLVM_CXX_INCLUDE}")

# Link libc++ libraries and set correct rpath
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} \
    -stdlib=libc++ \
    -L${LLVM_CXX_LIB} \
    -Wl,-rpath,${LLVM_CXX_LIB} \
")

# Same for shared libs
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} \
    -stdlib=libc++ \
    -L${LLVM_CXX_LIB} \
    -Wl,-rpath,${LLVM_CXX_LIB} \
")

# -------------------------------------
# Avoid accidental Xcode SDK usage
# -------------------------------------
set(CMAKE_FIND_USE_SYSTEM_ENVIRONMENT_PATH OFF)
set(CMAKE_FIND_FRAMEWORK LAST)
set(CMAKE_FIND_APPBUNDLE NEVER)
