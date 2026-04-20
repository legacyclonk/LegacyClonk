cmake_minimum_required(VERSION 3.20)

# -------------------------------
# Detect Homebrew prefix (ARM vs Intel)
# -------------------------------
execute_process(
	COMMAND brew --prefix
	OUTPUT_VARIABLE HOMEBREW_PREFIX
	OUTPUT_STRIP_TRAILING_WHITESPACE
	ERROR_QUIET
)

if (NOT HOMEBREW_PREFIX)
	# Fallback based on architecture
	if (CMAKE_HOST_SYSTEM_PROCESSOR STREQUAL "arm64")
		set(HOMEBREW_PREFIX "/opt/homebrew")
	else ()
		set(HOMEBREW_PREFIX "/usr/local")
	endif ()
endif ()

message(STATUS "Homebrew prefix: ${HOMEBREW_PREFIX}")

# Detect versioned LLVM (e.g. llvm@22) or plain llvm
file(GLOB LLVM_VERSIONED_DIRS "${HOMEBREW_PREFIX}/opt/llvm@*")
if (LLVM_VERSIONED_DIRS)
	# Use the latest versioned LLVM
	list(SORT LLVM_VERSIONED_DIRS)
	list(GET LLVM_VERSIONED_DIRS -1 LLVM_DIR)
else ()
	set(LLVM_DIR "${HOMEBREW_PREFIX}/opt/llvm")
endif ()

message(STATUS "LLVM directory: ${LLVM_DIR}")

# -------------------------------
# COMPILERS (from Homebrew LLVM)
# -------------------------------
set(CMAKE_C_COMPILER "${LLVM_DIR}/bin/clang" CACHE STRING "")
set(CMAKE_CXX_COMPILER "${LLVM_DIR}/bin/clang++" CACHE STRING "")

# explicit ninja path (helps CI)
find_program(NINJA_PROGRAM ninja
	PATHS "${HOMEBREW_PREFIX}/opt/ninja/bin" "${HOMEBREW_PREFIX}/bin"
	NO_DEFAULT_PATH
)
if (NINJA_PROGRAM)
	set(CMAKE_MAKE_PROGRAM "${NINJA_PROGRAM}" CACHE FILEPATH "Path to Ninja")
endif ()

# -------------------------------------
# macOS target + sysroot (no Xcode)
# -------------------------------------
# ARM Macs require macOS 11.0+, Intel supports back to 10.9
if (CMAKE_HOST_SYSTEM_PROCESSOR STREQUAL "arm64")
	set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "")
else ()
	set(CMAKE_OSX_DEPLOYMENT_TARGET "10.9" CACHE STRING "")
endif ()

set(CMAKE_OSX_SYSROOT "/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk" CACHE STRING "")

# -------------------------------------
# libc++ include + linker setup
# -------------------------------------
set(LLVM_CXX_INCLUDE "${LLVM_DIR}/include/c++/v1")
set(LLVM_CXX_LIB     "${LLVM_DIR}/lib/c++")

# Disable automatic stdlib selection
# _LIBCPP_DISABLE_AVAILABILITY: we statically link libc++, so availability
# annotations for the system dylib don't apply (needed for std::to_chars,
# std::format, etc. on older deployment targets)
set(COMMON_FLAGS "-fexperimental-library -Wno-parentheses -D_LIBCPP_DISABLE_AVAILABILITY")
set(CXX_STDLIB_FLAGS "-nostdlib++ -I${LLVM_CXX_INCLUDE}")

set(CMAKE_C_FLAGS_INIT "${COMMON_FLAGS}")
set(CMAKE_CXX_FLAGS_INIT "${CXX_STDLIB_FLAGS} ${COMMON_FLAGS}")
set(CMAKE_OBJCXX_FLAGS_INIT "${CXX_STDLIB_FLAGS} ${COMMON_FLAGS}")

# Force static libc++ linkage
set(STDLIB_FLAGS "-nostdlib++ ${LLVM_CXX_LIB}/libc++.a ${LLVM_CXX_LIB}/libc++abi.a ${LLVM_CXX_LIB}/libc++experimental.a")

set(CMAKE_EXE_LINKER_FLAGS_INIT "${STDLIB_FLAGS}")
set(CMAKE_SHARED_LINKER_FLAGS_INIT "${STDLIB_FLAGS}")

# -------------------------------------
# Avoid accidental Xcode SDK usage
# -------------------------------------
set(CMAKE_FIND_USE_SYSTEM_ENVIRONMENT_PATH OFF)
set(CMAKE_FIND_FRAMEWORK LAST)
set(CMAKE_FIND_APPBUNDLE NEVER)
