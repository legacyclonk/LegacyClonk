# LegacyClonk
#
# Copyright (c) 2017-2022, The LegacyClonk Team and contributors
#
# Distributed under the terms of the ISC license; see accompanying file
# "COPYING" for details.
#
# "Clonk" is a registered trademark of Matthes Bender, used with permission.
# See accompanying file "TRADEMARK" for details.
#
# To redistribute this file separately, substitute the full license texts
# for the above references.

cmake_minimum_required(VERSION 3.9)
cmake_policy(SET CMP0042 NEW) # enable MACOSX_RPATH by default

if (${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.12.0")
	cmake_policy(SET CMP0074 NEW) # enable <PackageName>_ROOT for find_package
endif ()

if (${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.11.0")
	cmake_policy(SET CMP0072 NEW) # prefer GLVND OpenGL libraries
endif ()

if (WIN32)
	set(CMAKE_SYSTEM_VERSION 6.1) # Windows 7
endif ()

project("LegacyClonk")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

include(CheckCXXSymbolExists)
include(CheckIncludeFileCXX)
include(CMakeDependentOption)

# Make sure headers of the project are found
include_directories(src)

# Additional directory to search for library files
set(EXTRA_DEPS_DIR "deps" CACHE PATH "Additional directory to search for libraries and headers")
include_directories(${EXTRA_DEPS_DIR}/include)
list(APPEND CMAKE_PREFIX_PATH ${EXTRA_DEPS_DIR})

# Add cmake directory to module path
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

# Check which header files and functions are present

CHECK_INCLUDE_FILE_CXX(direct.h           HAVE_DIRECT_H)
CHECK_INCLUDE_FILE_CXX(execinfo.h         HAVE_EXECINFO_H)
if (NOT WIN32)
	CHECK_INCLUDE_FILE_CXX(iconv.h            HAVE_ICONV)
endif ()
CHECK_INCLUDE_FILE_CXX(io.h               HAVE_IO_H)
CHECK_INCLUDE_FILE_CXX(langinfo.h         HAVE_LANGINFO_H)
CHECK_INCLUDE_FILE_CXX(locale.h           HAVE_LOCALE_H)
CHECK_INCLUDE_FILE_CXX(share.h            HAVE_SHARE_H)
CHECK_INCLUDE_FILE_CXX(signal.h           HAVE_SIGNAL_H)
CHECK_INCLUDE_FILE_CXX(stdint.h           HAVE_STDINT_H)
CHECK_INCLUDE_FILE_CXX(sys/inotify.h      HAVE_SYS_INOTIFY_H)
CHECK_INCLUDE_FILE_CXX(sys/socket.h       HAVE_SYS_SOCKET_H)
CHECK_INCLUDE_FILE_CXX(sys/stat.h         HAVE_SYS_STAT_H)
CHECK_INCLUDE_FILE_CXX(sys/syscall.h      HAVE_SYS_SYSCALL_H)
CHECK_INCLUDE_FILE_CXX(sys/types.h        HAVE_SYS_TYPES_H)
CHECK_INCLUDE_FILE_CXX(unistd.h           HAVE_UNISTD_H)
CHECK_CXX_SYMBOL_EXISTS(vasprintf stdio.h HAVE_VASPRINTF)

# Define options

option(DEBUGREC "Write additional debug control to records" OFF)
option(USE_CONSOLE "Dedicated server mode (compile as pure console application)" OFF)
option(USE_STAT "Enable internal performance statistics for developers" OFF)

# ENABLE_SOUND
CMAKE_DEPENDENT_OPTION(ENABLE_SOUND "Compile with sound support" ON
	"NOT USE_CONSOLE" OFF)
# USE_FMOD
CMAKE_DEPENDENT_OPTION(USE_FMOD "Whether FMod should be tried to be loaded dynamically for sound" OFF
	"ENABLE_SOUND AND WIN32" OFF)
# USE_SDL_MIXER
CMAKE_DEPENDENT_OPTION(USE_SDL_MIXER "Whether SDL2_mixer shall be used for sound" ON
	"ENABLE_SOUND" OFF)
# USE_SDL_MAINLOOP
if (APPLE)
	set(USE_SDL_MAINLOOP_DEFAULT ON)
else ()
	set(USE_SDL_MAINLOOP_DEFAULT OFF)
endif ()
CMAKE_DEPENDENT_OPTION(USE_SDL_MAINLOOP "Use SDL instead of X11 or Win32" ${USE_SDL_MAINLOOP_DEFAULT}
	"NOT USE_CONSOLE AND NOT WIN32" OFF)
# USE_X11
CMAKE_DEPENDENT_OPTION(USE_X11 "Use X11" ON
	"NOT USE_CONSOLE AND NOT USE_SDL_MAINLOOP AND NOT APPLE AND NOT WIN32" OFF)
# USE_WIC
CMAKE_DEPENDENT_OPTION(USE_WIC "Use WIC instead of libjpeg and libpng" ON
	"WIN32" OFF)
# WITH_DEVELOPER_MODE
CMAKE_DEPENDENT_OPTION(WITH_DEVELOPER_MODE "Use GTK for the developer mode" OFF
	"NOT USE_CONSOLE" OFF)
# USE_LIBNOTIFY
CMAKE_DEPENDENT_OPTION(USE_LIBNOTIFY "Use libnotify for desktop notifications" ON "WITH_DEVELOPER_MODE AND NOT USE_CONSOLE AND NOT WIN32" OFF)
# USE_WINTOAST
CMAKE_DEPENDENT_OPTION(USE_WINTOAST "Use WinToast for desktop notifications" ON "NOT USE_CONSOLE AND WIN32" OFF)

# Use Microsoft CryptoAPI instead of OpenSSL when compiling for Windows
if (WIN32)
	set(USE_MSCAPI ON)
endif ()

# Disable some Visual C++ warnings
if (MSVC)
	add_definitions(-D_CRT_NONSTDC_NO_DEPRECATE -D_CRT_SECURE_NO_WARNINGS)
	# "multiple copy constructors specified"
	add_compile_options(/wd4521)
endif ()

# Adds the lines of file LIST_FILE to variable TARGET_LIST
function(append_filelist TARGET_LIST LIST_FILE)
	file(STRINGS "cmake/filelists/${LIST_FILE}" FILE_CONTENTS)
	list(APPEND ${TARGET_LIST} ${FILE_CONTENTS})
	set(${TARGET_LIST} "${${TARGET_LIST}}" PARENT_SCOPE)
endfunction()

# Makes a text file includable
# https://stackoverflow.com/a/47801116

function(make_includable INPUT_FILE OUTPUT_VAR)
	set(DELIM "for_c++_include")
	set(CHUNK_SIZE 2048)

	file(READ "${INPUT_FILE}" CONTENT)
	set(RESULT "")

	string(LENGTH "${CONTENT}" LENGTH)
	foreach (BEGIN RANGE 0 ${LENGTH} ${CHUNK_SIZE})
		string(SUBSTRING "${CONTENT}" ${BEGIN} ${CHUNK_SIZE} PART)
		set(PART "R\"${DELIM}(${PART})${DELIM}\" ")
		string(APPEND RESULT "${PART}")
	endforeach ()

	set(${OUTPUT_VAR} "${RESULT}" PARENT_SCOPE)
endfunction()

# Add engine target

append_filelist(CLONK_SOURCES Engine.txt)

set(LICENSES
	"COPYING@LegacyClonk@ISC"
	"TRADEMARK@Clonk Trademark")

include(deps/licenses.cmake OPTIONAL)

file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/generated")
set(LICENSES_TEXT "")
set(FIRST TRUE)
foreach (LICENSE IN LISTS LICENSES)
	if (NOT ${FIRST})
		string(APPEND LICENSES_TEXT ",\n")
	endif ()
	set(FIRST FALSE)

	string(REGEX MATCH "^([^@]+)@([^@]+)(@([^@]+))?$" PARTS "${LICENSE}")
	make_includable("${CMAKE_MATCH_1}" THIS_LICENSE_TEXT)
	string(APPEND LICENSES_TEXT "{\"${CMAKE_MATCH_2}\", R\"(${CMAKE_MATCH_4})\", ${THIS_LICENSE_TEXT}}")
endforeach ()
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/generated/licenses.h" "${LICENSES_TEXT}")

if (ENABLE_SOUND)
	if (USE_FMOD)
		list(APPEND CLONK_SOURCES src/C4AudioSystemFmodRuntime.cpp)
		list(APPEND CLONK_SOURCES src/C4AudioSystemFmodRuntime.h)
	endif ()
	if (USE_SDL_MIXER)
		list(APPEND CLONK_SOURCES src/C4AudioSystemSdl.cpp)
		list(APPEND CLONK_SOURCES src/C4AudioSystemSdl.h)
	endif ()
endif ()

if (WIN32)
	append_filelist(CLONK_SOURCES EngineWin32.txt)
elseif (APPLE)
	append_filelist(CLONK_SOURCES EngineApple.txt)
endif ()

if (USE_SDL_MAINLOOP OR USE_SDL_MIXER)
	append_filelist(CLONK_SOURCES EngineSdl.txt)
endif ()

if (USE_SDL_MAINLOOP)
	append_filelist(CLONK_SOURCES EngineSdlMainloop.txt)
endif ()

if (USE_X11)
	append_filelist(CLONK_SOURCES EngineX.txt)
endif ()

if (USE_WIC)
	list(APPEND CLONK_SOURCES
		src/StdJpegWic.cpp src/StdPNGWic.cpp src/StdWic.cpp src/StdWic.h)
else ()
	list(APPEND CLONK_SOURCES
		src/StdJpegLibjpeg.cpp src/StdPNGLibpng.cpp)
endif ()

if (WITH_DEVELOPER_MODE)
	append_filelist(CLONK_SOURCES EngineGtk.txt)
endif ()

if (USE_CONSOLE AND NOT WIN32)
	append_filelist(CLONK_SOURCES EngineConsoleNonWin32.txt)
endif ()

if (USE_LIBNOTIFY)
	list(APPEND CLONK_SOURCES
		src/C4ToastLibNotify.cpp src/C4ToastLibNotify.h)
endif ()

if (USE_WINTOAST)
	list(APPEND CLONK_SOURCES
		src/C4ToastWinToastLib.cpp src/C4ToastWinToastLib.h
		src/WinToast/src/wintoastlib.cpp src/WinToast/src/wintoastlib.h)
endif ()

if (USE_CONSOLE)
	add_executable(clonk                     ${CLONK_SOURCES})
else ()
	add_executable(clonk WIN32 MACOSX_BUNDLE ${CLONK_SOURCES})
endif ()
target_compile_definitions(clonk PRIVATE C4ENGINE)

# Add c4group target

append_filelist(C4GROUP_SOURCES C4Group.txt)
if (WIN32)
	list(APPEND C4GROUP_SOURCES src/c4group_cmdl.cpp)
else ()
	list(APPEND C4GROUP_SOURCES src/c4group_ng.cpp)
endif ()
add_executable(c4group ${C4GROUP_SOURCES})
target_link_libraries(c4group standard)
target_compile_definitions(c4group PRIVATE USE_CONSOLE)

# Add libstandard target

append_filelist(LIBSTANDARD_SOURCES Std.txt)

if (USE_MSCAPI)
	list(APPEND LIBSTANDARD_SOURCES src/StdSha1MSCapi.cpp)
else ()
	list(APPEND LIBSTANDARD_SOURCES src/StdSha1OpenSSL.cpp)
endif ()

add_library(standard STATIC ${LIBSTANDARD_SOURCES})
target_compile_definitions(standard PRIVATE C4ENGINE HAVE_CONFIG_H)

# Link OpenSSL crypto library
if (NOT USE_MSCAPI)
	find_package(OpenSSL)
	if (NOT TARGET OpenSSL::Crypto)
		message(FATAL_ERROR "Cannot find OpenSSL crypto library")
	endif ()
	target_link_libraries(standard OpenSSL::Crypto)
endif ()

target_link_libraries(clonk standard)

# Link libraries

# Link Freetype
if (NOT USE_CONSOLE)
	find_package(Freetype REQUIRED)
	target_include_directories(clonk PRIVATE ${FREETYPE_INCLUDE_DIRS})
	target_link_libraries(clonk ${FREETYPE_LIBRARIES})
	set(HAVE_FREETYPE ${FREETYPE_FOUND})
endif ()

# Link GTK2
if (WITH_DEVELOPER_MODE)
	set(WITH_GLIB 1)
	find_package(GTK2 2.4 REQUIRED gtk)
	target_include_directories(clonk PRIVATE ${GTK2_INCLUDE_DIRS})
	target_link_libraries(clonk ${GTK2_LIBRARIES})
endif ()

# Link iconv
if (HAVE_ICONV AND APPLE)
	target_link_libraries(clonk iconv)
endif ()

# Link libjpeg
if (NOT USE_WIC)
	find_package(JPEG REQUIRED)
	target_include_directories(clonk PRIVATE ${JPEG_INCLUDE_DIR})
	target_link_libraries(clonk ${JPEG_LIBRARIES})
endif ()

# Link OpenGL and GLEW
if (NOT USE_CONSOLE)
	find_package(OpenGL REQUIRED)
	if (NOT TARGET OpenGL::GLU)
		message(FATAL_ERROR "Cannot find GLU library")
	endif ()
	find_package(GLEW REQUIRED)
	target_link_libraries(clonk OpenGL::GL OpenGL::GLU GLEW::GLEW)
endif ()

# Link libpng
if (NOT USE_WIC)
	find_package(PNG REQUIRED)
	target_link_libraries(clonk PNG::PNG)
endif ()

# Link SDL2
if (USE_SDL_MAINLOOP OR USE_SDL_MIXER)
	find_package(SDL2 REQUIRED)
	if (SDL2_FOUND AND USE_SDL_MAINLOOP)
		set(USE_SDL_FOR_GAMEPAD ON)
	endif ()
	target_include_directories(clonk PRIVATE ${SDL2_INCLUDE_DIRS})
	target_link_libraries(clonk ${SDL2_LIBRARIES})
endif ()

# Link SDL2_mixer
if (USE_SDL_MIXER)
	find_package(SDL2Mixer REQUIRED)
	target_include_directories(clonk PRIVATE ${SDL2Mixer_INCLUDE_DIRS})
	target_link_libraries(clonk ${SDL2Mixer_LIBRARIES})
endif ()

# Link Windows Imaging Component
if (USE_WIC)
	target_link_libraries(clonk windowscodecs)
endif ()

# Link thread library
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads)
if (THREADS_FOUND)
	target_link_libraries(clonk Threads::Threads)
endif ()

# Link libnotify
if (USE_LIBNOTIFY)
	find_package(PkgConfig REQUIRED)
	pkg_check_modules(libnotify REQUIRED IMPORTED_TARGET libnotify)
	target_include_directories(clonk PRIVATE ${libnotify_INCLUDE_DIRS})
	target_link_libraries(clonk PkgConfig::libnotify)
endif ()

if (USE_WINTOAST)
	target_include_directories(clonk PRIVATE src/WinToast/src)
endif ()

# Link Windows libraries
if (WIN32)
	target_link_libraries(clonk dbghelp iphlpapi winmm ws2_32)

	if (USE_SDL_MIXER)
		target_link_libraries(clonk imm32.lib setupapi.lib version.lib)
	endif ()
endif ()

if (APPLE)
	target_link_libraries(clonk "-framework AppKit")
endif ()

target_link_libraries(clonk ${EXTRA_LINK_LIBS})

# Link X11
if (USE_X11)
	find_package(X11 REQUIRED)
	if (NOT X11_FOUND)
		message(FATAL_ERROR "libX11 not found.")
	endif ()
	if (NOT X11_Xpm_FOUND)
		message(FATAL_ERROR "libXpm not found.")
	endif ()
	if (NOT X11_xf86vmode_FOUND)
		message(FATAL_ERROR "XF86VidMode not found.")
	endif ()
	target_include_directories(clonk PRIVATE
		${X11_X11_INCLUDE_PATH} ${X11_Xpm_INCLUDE_PATH} ${X11_xf86vmode_INCLUDE_PATH})
	target_link_libraries(clonk
		${X11_X11_LIB}          ${X11_Xpm_LIB}          ${X11_Xxf86vm_LIB})
endif ()

# Link zlib
find_package(ZLIB REQUIRED)
target_link_libraries(standard ZLIB::ZLIB)

# Define _DEBUG or NDEBUG depending on project configuration (Already defined by MSVC)
if (NOT MSVC)
	set_property(DIRECTORY APPEND PROPERTY COMPILE_DEFINITIONS
		$<$<CONFIG:Debug>:_DEBUG>
		$<$<NOT:$<CONFIG:Debug>>:NDEBUG>)
endif ()

# Get non-standard <cmath> constants (M_PI etc.)
if (MSVC)
	set(_USE_MATH_DEFINES ON)
endif ()

# Create config.h and make sure it will be used and found
target_compile_definitions(clonk PRIVATE HAVE_CONFIG_H)
configure_file(config.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config.h)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

if (APPLE AND NOT USE_CONSOLE)
	set(BUNDLE_RESOURCES
		src/res/lc.icns
		src/res/C4U.icns
		src/res/C4P.icns
		src/res/C4S.icns
		src/res/C4F.icns
		src/res/C4G.icns
		src/res/C4D.icns
	)

	set_target_properties(clonk PROPERTIES RESOURCE "${BUNDLE_RESOURCES}")
	set_target_properties(clonk PROPERTIES MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/src/res/Info.plist")
	set_target_properties(clonk PROPERTIES INSTALL_RPATH "@loader_path/../Frameworks")

	install(TARGETS clonk BUNDLE DESTINATION . RESOURCE DESTINATION ./clonk.app/Contents/Resources/)
	install(TARGETS c4group RUNTIME DESTINATION .)
endif ()
