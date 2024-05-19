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

cmake_minimum_required(VERSION 3.16)

# Enable MACOSX_RPATH by default
cmake_policy(SET CMP0042 NEW)

# Prefer GLVND OpenGL libraries
cmake_policy(SET CMP0072 NEW)

# Enable <PackageName>_ROOT for find_package
cmake_policy(SET CMP0074 NEW)

if (APPLE)
	set(LANGUAGE_OBJCXX OBJCXX)
endif ()

# Target Windows 7
if (WIN32)
	set(CMAKE_SYSTEM_VERSION 6.1)
	add_compile_definitions(NTDDI_VERSION=NTDDI_WIN7 _WIN32_WINNT=_WIN32_WINNT_WIN7)
endif ()

project("LegacyClonk" LANGUAGES CXX ${LANGUAGE_OBJCXX})

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (APPLE)
	set(CMAKE_OBJCXX_STANDARD ${CMAKE_CXX_STANDARD})
	set(CMAKE_OBJCXX_STANDARD_REQUIRED ${CMAKE_CXX_STANDARD_REQUIRED})
endif ()

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

include(CheckCXXSymbolExists)
include(CheckIncludeFileCXX)
include(CMakeDependentOption)

# Make sure headers of the project are found
include_directories(src)

# Additional directory to search for library files
set(EXTRA_DEPS_DIR "deps" CACHE PATH "Additional directory to search for libraries and headers")
include_directories(${EXTRA_DEPS_DIR}/include)
link_directories("${EXTRA_DEPS_DIR}/lib")
list(APPEND CMAKE_PREFIX_PATH ${EXTRA_DEPS_DIR})

# Add cmake directory to module path
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

# Define options

option(DEBUGREC "Write additional debug control to records" OFF)
option(SOLIDMASK_DEBUG "Solid mask debugging" OFF)
option(USE_CONSOLE "Dedicated server mode (compile as pure console application)" OFF)
option(USE_LTO "Enable Link Time Optimization" ON)
option(USE_PCH "Precompile Headers" ON)
option(USE_STAT "Enable internal performance statistics for developers" OFF)
option(USE_TESTS "Enable testing" OFF)

# ENABLE_SOUND
CMAKE_DEPENDENT_OPTION(ENABLE_SOUND "Compile with sound support" ON
	"NOT USE_CONSOLE" OFF)
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
# USE_WINDOWS_RUNTIME
CMAKE_DEPENDENT_OPTION(USE_WINDOWS_RUNTIME "Use Windows Runtime features" ON "WIN32" OFF)

if (USE_LTO)
	include(CheckIPOSupported)
	check_ipo_supported()

	set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)
endif ()

if (WIN32)
	# Don't define min/max macros in Windows.h so that they don't conflict with std::min/max
	add_compile_definitions(NOMINMAX)

	# Disable deprecation and insecure function warnings
	add_compile_definitions(_CRT_NONSTDC_NO_DEPRECATE _CRT_SECURE_NO_WARNINGS)

	# Don't include winsock.h in Windows.h so that Windows.h doesn't conflict with WinSock2.h
	add_compile_definitions(WIN32_LEAN_AND_MEAN)

	# Use Microsoft CryptoAPI instead of OpenSSL when compiling for Windows
	set(USE_MSCAPI ON)

	# Use LargeAddressAware on x86
	if (CMAKE_SIZEOF_VOID_P EQUAL 4)
		if (MSVC)
			add_link_options(/LARGEADDRESSAWARE)
		else ()
			add_link_options(--large-address-aware)
		endif ()
	endif ()
endif ()

# Disable some Visual C++ warnings
if (MSVC)
	# "multiple copy constructors specified"
	add_compile_options(/wd4521)
endif ()

# Incompatible with CF_ENUM
if (APPLE)
	add_compile_options(-Wno-elaborated-enum-base)
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
include_directories("${CMAKE_CURRENT_BINARY_DIR}")

if (ENABLE_SOUND)
	if (USE_SDL_MIXER)
		list(APPEND CLONK_SOURCES src/C4AudioSystemSdl.cpp)
		list(APPEND CLONK_SOURCES src/C4AudioSystemSdl.h)
	endif ()
endif ()

if (WIN32)
	append_filelist(CLONK_SOURCES EngineWin32.txt)
else ()
	if (APPLE)
		append_filelist(CLONK_SOURCES EngineApple.txt)
	endif ()

	list(APPEND CLONK_SOURCES src/StdAppUnix.cpp)
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

if (USE_WINDOWS_RUNTIME)
	list(APPEND CLONK_SOURCES
		src/C4ToastWinRT.cpp src/C4ToastWinRT.h)
endif ()

if (USE_CONSOLE)
	add_executable(clonk ${CLONK_SOURCES})
	target_compile_definitions(clonk PRIVATE USE_CONSOLE=1)
else ()
	add_executable(clonk WIN32 MACOSX_BUNDLE ${CLONK_SOURCES})
endif ()
target_link_libraries(clonk standard)
target_compile_definitions(clonk PRIVATE C4ENGINE)

if (USE_SDL_MIXER AND WIN32)
	target_compile_definitions(clonk PRIVATE SDL_MAIN_HANDLED)
endif ()

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

if (WIN32)
	list(APPEND LIBSTANDARD_SOURCES src/StdRegistry.cpp src/StdRegistry.h)
endif ()

if (USE_MSCAPI)
	list(APPEND LIBSTANDARD_SOURCES src/StdSha1MSCapi.cpp)
else ()
	list(APPEND LIBSTANDARD_SOURCES src/StdSha1OpenSSL.cpp)
endif ()

add_library(standard STATIC ${LIBSTANDARD_SOURCES})
target_compile_definitions(standard PRIVATE C4ENGINE)

# Set C4_OS macro
if (APPLE)
	set(C4_OS "mac")
elseif (UNIX AND CMAKE_SIZEOF_VOID_P EQUAL 4)
	set(C4_OS "linux")
elseif (UNIX AND CMAKE_SIZEOF_VOID_P EQUAL 8)
	set(C4_OS "linux64")
elseif (WIN32 AND CMAKE_SIZEOF_VOID_P EQUAL 4)
	set(C4_OS "win32")
elseif (WIN32 AND CMAKE_SIZEOF_VOID_P EQUAL 8)
	set(C4_OS "win64")
else ()
	message(FATAL_ERROR "Unknown platform")
endif ()

# Link OpenSSL crypto library
if (NOT USE_MSCAPI)
	find_package(OpenSSL)
	if (NOT TARGET OpenSSL::Crypto)
		message(FATAL_ERROR "Cannot find OpenSSL crypto library")
	endif ()
	target_link_libraries(standard OpenSSL::Crypto)
endif ()

# Link libraries

# Link Catch2
if (USE_TESTS)
	find_package(Catch2 3 REQUIRED)
endif ()

# Link fmt
find_package(fmt REQUIRED)
target_link_libraries(standard fmt::fmt)
list(APPEND PCH_EXTRA <fmt/format.h> <fmt/printf.h>)

# Link Freetype
if (NOT USE_CONSOLE)
	find_package(Freetype REQUIRED)
	target_link_libraries(clonk Freetype::Freetype)
	set(HAVE_FREETYPE 1)
endif ()

# Link GTK2
if (WITH_DEVELOPER_MODE)
	set(WITH_GLIB 1)
	find_package(GTK2 2.4 REQUIRED gtk)
	target_link_libraries(clonk GTK2::gtk)
endif ()

# Link iconv
if (NOT WIN32)
	find_package(Iconv REQUIRED)
	target_link_libraries(clonk Iconv::Iconv)
	set(HAVE_ICONV 1)
endif ()

# Link libjpeg
if (NOT USE_WIC)
	find_package(JPEG REQUIRED)
	target_link_libraries(clonk JPEG::JPEG)
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
	target_link_libraries(clonk SDL2::SDL2)
endif ()

# Link SDL2_mixer
if (USE_SDL_MIXER)
	find_package(SDL2_mixer REQUIRED)
	target_link_libraries(clonk SDL2_mixer::SDL2_mixer)
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
	target_link_libraries(clonk PkgConfig::libnotify)
endif ()

# Link Windows libraries
if (WIN32)
	target_link_libraries(clonk dbghelp dwmapi iphlpapi winmm XInput ws2_32)

	if (USE_SDL_MIXER)
		target_link_libraries(clonk imm32.lib setupapi.lib version.lib)
	endif ()
endif ()

# Link C++/WinRT
if (WIN32)
	find_program(CPPWINRT cppwinrt REQUIRED)
	set(CPPWINRT_ARGS -base -optimize -output "${CMAKE_CURRENT_BINARY_DIR}/generated/cppwinrt")

	if (USE_WINDOWS_RUNTIME)
		list(APPEND CPPWINRT_ARGS -input sdk)
	endif ()

	execute_process(COMMAND "${CPPWINRT}" ${CPPWINRT_ARGS})

	target_include_directories(clonk PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/generated/cppwinrt")

	if (USE_WINDOWS_RUNTIME)
		if ("${CMAKE_SIZEOF_VOID_P}" STREQUAL "8")
			target_link_options(clonk PRIVATE "/alternatename:WINRT_IMPL_GetSystemTimePreciseAsFileTime=Clonk_GetSystemTimePreciseAsFileTime")
		else ()
			target_link_options(clonk PRIVATE "/alternatename:_WINRT_IMPL_GetSystemTimePreciseAsFileTime@4=_Clonk_GetSystemTimePreciseAsFileTime@4")
		endif ()
	endif ()
endif ()

if (APPLE)
	target_link_libraries(clonk "-framework AppKit")
endif ()

# Link X11
if (USE_X11)
	find_package(X11 REQUIRED)
	if (NOT X11_FOUND)
		message(FATAL_ERROR "libX11 not found.")
	endif ()
	if (NOT X11_Xpm_FOUND)
		message(FATAL_ERROR "libXpm not found.")
	endif ()
	if (NOT X11_Xxf86vm_FOUND)
		message(FATAL_ERROR "XF86VidMode not found.")
	endif ()
	target_link_libraries(clonk X11::X11 X11::Xpm X11::Xxf86vm)
endif ()

# Link zlib
find_package(ZLIB REQUIRED)
target_link_libraries(standard ZLIB::ZLIB)

# Setup testing

if (USE_TESTS)
	enable_testing()
	add_subdirectory(tests)
	get_property(MACRO_TARGETS DIRECTORY tests PROPERTY BUILDSYSTEM_TARGETS)
endif ()

list(PREPEND MACRO_TARGETS standard)

# Define macros

foreach (MACRO_TARGET ${MACRO_TARGETS})
	target_compile_definitions(${MACRO_TARGET} PUBLIC
		C4_OS="${C4_OS}"
		ICONV_CONST=
		STD_APPUSERMODELID="LegacyClonkTeam.LegacyClonk"
		# A standard product name for this project which is used in window registration etc.
		STD_PRODUCT="LegacyClonk"
		ZLIB_CONST=1
	)

	# Define macros if their variable is set
	foreach(MACRO_VAR
		DEBUGREC
		ENABLE_SOUND
		HAVE_FREETYPE
		HAVE_ICONV
		SOLIDMASK_DEBUG
		USE_LIBNOTIFY
		USE_SDL_FOR_GAMEPAD
		USE_SDL_MAINLOOP
		USE_SDL_MIXER
		USE_STAT
		USE_WINDOWS_RUNTIME
		USE_X11
		WITH_DEVELOPER_MODE
		WITH_GLIB
	)
		if (${MACRO_VAR})
			target_compile_definitions(${MACRO_TARGET} PUBLIC ${MACRO_VAR}=1)
		endif ()
	endforeach()

	# Define DEBUGREC_* macros if DEBUGREC is set
	if (DEBUGREC)
		foreach(DEBUGREC_MACRO
			MATSCAN=1
			MENU=1
			OBJCOM=1
			OCF=1
			PXS=1
			#RECRUITMENT=1
			SCRIPT=1
			START_FRAME=0
		)
			target_compile_definitions(${MACRO_TARGET} PUBLIC "DEBUGREC_${DEBUGREC_MACRO}")
		endforeach()
	endif ()
endforeach ()

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

# Precompile headers

function(set_up_pch TARGET)
	# Go through each source file of the target
	get_target_property(SOURCES ${TARGET} SOURCES)
	foreach(SOURCE ${SOURCES})
		file(STRINGS "${SOURCE}" SOURCE_LINES ENCODING UTF-8)
		# Include header files that use "#pragma once"
		if (SOURCE MATCHES ".*\.h$")
			foreach(SOURCE_LINE ${SOURCE_LINES})
				if (SOURCE_LINE MATCHES "^[\t ]*#[\t ]*pragma[\t ]*once$")
					list(APPEND PCH_LIST "${SOURCE}")
					break()
				endif ()
			endforeach()
		endif ()
		# Find C++ standard library headers used by the target and add them to the PCH list
		foreach(SOURCE_LINE ${SOURCE_LINES})
			string(REGEX MATCH "^[\t ]*#[\t ]*include[\t ]*<[\t ]*([_a-z]+)[\t ]*>" INCLUDE_MATCH "${SOURCE_LINE}")
			if (NOT INCLUDE_MATCH STREQUAL "")
				list(APPEND PCH_LIST "<${CMAKE_MATCH_1}>")
			endif ()
		endforeach()
	endforeach()
	# Don't precompile certain headers
	file(STRINGS "cmake/filelists/PchExclude.txt" PCH_EXCLUDE)
	list(REMOVE_ITEM PCH_LIST ${PCH_EXCLUDE})
	# Include headers of external libraries
	list(APPEND PCH_LIST ${PCH_EXTRA})
	# Clean up PCH list
	list(REMOVE_DUPLICATES PCH_LIST)
	list(SORT PCH_LIST)
	# Enable PCH
	target_precompile_headers(${TARGET} PRIVATE ${PCH_LIST})
endfunction()

if (USE_PCH)
	foreach(TARGET c4group clonk standard)
		set_up_pch(${TARGET})
	endforeach()
endif ()
