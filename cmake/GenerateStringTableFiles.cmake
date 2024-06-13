# LegacyClonk
#
# Copyright (c) 2024, The LegacyClonk Team and contributors
#
# Distributed under the terms of the ISC license; see accompanying file
# "COPYING" for details.
#
# "Clonk" is a registered trademark of Matthes Bender, used with permission.
# See accompanying file "TRADEMARK" for details.
#
# To redistribute this file separately, substitute the full license texts
# for the above references.

cmake_policy(SET CMP0007 NEW)

file(WRITE ${OUTPUT_FILE_H} "#pragma once\n\n#include <cstdint>\n\nenum class C4ResStrTableKey : std::uint16_t\n{\n")

file(STRINGS ${INPUT_FILE} INPUT)
list(FILTER INPUT EXCLUDE REGEX "^#")
list(FILTER INPUT INCLUDE REGEX ".")

list(TRANSFORM INPUT PREPEND "\t" OUTPUT_VARIABLE INPUT_ENUM)
list(TRANSFORM INPUT_ENUM APPEND ",")
list(JOIN INPUT_ENUM "\n" INPUT_ENUM)
file(APPEND ${OUTPUT_FILE_H} ${INPUT_ENUM})

file(APPEND ${OUTPUT_FILE_H} "\n\tNumberOfEntries\n};\n")

file(WRITE ${OUTPUT_FILE_CPP} "#include \"C4ResStrTable.h\"\n#include <string_view>\n#include<unordered_map>\n\n")
file(APPEND ${OUTPUT_FILE_CPP} "std::unordered_map<std::string_view, C4ResStrTableKey> C4ResStrTable::GetKeyStringMap()\n{\n")
file(APPEND ${OUTPUT_FILE_CPP} "\treturn {{\n")

set(INPUT_MAP "${INPUT}")
list(LENGTH INPUT_MAP INPUT_MAP_LENGTH)
math(EXPR INPUT_MAP_LENGTH "${INPUT_MAP_LENGTH} - 1")

foreach (LINE_INDEX RANGE ${INPUT_MAP_LENGTH})
	list(GET INPUT_MAP ${LINE_INDEX} LINE)
	set(LINE "\t\t{\"${LINE}\", C4ResStrTableKey::${LINE}}")
	list(INSERT INPUT_MAP ${LINE_INDEX} ${LINE})
	math(EXPR LINE_INDEX_PLUS_ONE "${LINE_INDEX} + 1")
	list(REMOVE_AT INPUT_MAP ${LINE_INDEX_PLUS_ONE})
endforeach ()

math(EXPR INPUT_MAP_LENGTH "${INPUT_MAP_LENGTH} - 1")
list(TRANSFORM INPUT_MAP APPEND "," FOR 0 ${INPUT_MAP_LENGTH})

list(JOIN INPUT_MAP "\n" INPUT_MAP)

file(APPEND ${OUTPUT_FILE_CPP} "${INPUT_MAP}")
file(APPEND ${OUTPUT_FILE_CPP} "\n\t}};\n}\n")
