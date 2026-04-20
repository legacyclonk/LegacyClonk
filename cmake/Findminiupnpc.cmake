# Findminiupnpc.cmake
# Finds the miniupnpc library using pkg-config as fallback
# when no CMake config file is available (e.g. Homebrew on macOS).
#
# Creates the miniupnpc::miniupnpc imported target to match
# the CONFIG mode target name used in CMakeLists.txt.

# Try pkg-config first
find_package(PkgConfig QUIET)
if (PkgConfig_FOUND)
	pkg_check_modules(_MINIUPNPC QUIET miniupnpc)
endif ()

find_path(miniupnpc_INCLUDE_DIR
	NAMES miniupnpc/miniupnpc.h
	HINTS ${_MINIUPNPC_INCLUDE_DIRS}
)

find_library(miniupnpc_LIBRARY
	NAMES miniupnpc
	HINTS ${_MINIUPNPC_LIBRARY_DIRS}
)

if (_MINIUPNPC_VERSION)
	set(miniupnpc_VERSION "${_MINIUPNPC_VERSION}")
endif ()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(miniupnpc
	REQUIRED_VARS miniupnpc_LIBRARY miniupnpc_INCLUDE_DIR
	VERSION_VAR miniupnpc_VERSION
)

if (miniupnpc_FOUND AND NOT TARGET miniupnpc::miniupnpc)
	add_library(miniupnpc::miniupnpc UNKNOWN IMPORTED)
	set_target_properties(miniupnpc::miniupnpc PROPERTIES
		IMPORTED_LOCATION "${miniupnpc_LIBRARY}"
		INTERFACE_INCLUDE_DIRECTORIES "${miniupnpc_INCLUDE_DIR}"
	)
endif ()

mark_as_advanced(miniupnpc_INCLUDE_DIR miniupnpc_LIBRARY)
