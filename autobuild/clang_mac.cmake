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

# Disable automatic stdlib selection
set(CMAKE_C_FLAGS_INIT "-fexperimental-library -Wno-parentheses")
set(CMAKE_CXX_FLAGS_INIT "-nostdlib++ ${CMAKE_C_FLAGS_INIT}")
set(CMAKE_OBJCXX_FLAGS_INIT "-nostdlib++ ${CMAKE_CXX_FLAGS_INIT}")

# Force static libc++ linkage
set(STDLIB_FLAGS "-nostdlib++ ${LLVM_CXX_LIB}/libc++.a ${LLVM_CXX_LIB}/libc++abi.a ${LLVM_CXX_LIB}/libc++experimental.a")

set(CMAKE_EXE_LINKER_FLAGS_INIT "${STDLIB_FLAGS}")
set(CMAKE_SHARED_LINKER_FLAGS_INIT "${STDLIB_FLAGS}")

# Provide libc++ headers explicitly
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -I${LLVM_CXX_INCLUDE}")

# -------------------------------------
# Avoid accidental Xcode SDK usage
# -------------------------------------
set(CMAKE_FIND_USE_SYSTEM_ENVIRONMENT_PATH OFF)
set(CMAKE_FIND_FRAMEWORK LAST)
set(CMAKE_FIND_APPBUNDLE NEVER)
