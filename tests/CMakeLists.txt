# LegacyClonk
#
# Copyright (c) 2023, The LegacyClonk Team and contributors
#
# Distributed under the terms of the ISC license; see accompanying file
# "COPYING" for details.
#
# "Clonk" is a registered trademark of Matthes Bender, used with permission.
# See accompanying file "TRADEMARK" for details.
#
# To redistribute this file separately, substitute the full license texts
# for the above references.

function (add_test_target TEST_NAME)
	set(TARGET "test_${TEST_NAME}")
	cmake_parse_arguments(PARSE_ARGV 1 "ADD_TEST" "" "" "SOURCES;LIBRARIES")

	list(PREPEND ADD_TEST_SOURCES "tests/${TARGET}.cpp")
	list(TRANSFORM ADD_TEST_SOURCES PREPEND "${CMAKE_SOURCE_DIR}/")

	add_executable("${TARGET}" "${ADD_TEST_SOURCES}")
	target_include_directories("${TARGET}" PRIVATE "${CMAKE_SOURCE_DIR}/src")
	target_link_libraries("${TARGET}" PRIVATE Catch2::Catch2WithMain "${ADD_TEST_LIBRARIES}")

	add_test(NAME "${TEST_NAME}" COMMAND "${TARGET}" WORKING_DIRECTORY "${CMAKE_BINARY_DIR}")
endfunction ()
