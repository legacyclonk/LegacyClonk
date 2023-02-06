/*
 * LegacyClonk
 *
 * Copyright (c) 2023, The LegacyClonk Team and contributors
 *
 * Distributed under the terms of the ISC license; see accompanying file
 * "COPYING" for details.
 *
 * "Clonk" is a registered trademark of Matthes Bender, used with permission.
 * See accompanying file "TRADEMARK" for details.
 *
 * To redistribute this file separately, substitute the full license texts
 * for the above references.
 */

#pragma once

enum class CrashReporterErrorCode : std::int32_t
{
	Success = 0,
	ParseCommandLine,
	ValidateHandle,
	LoadConfig,
	EnvironmentVariables,
	UserPathMissing,
	FormatSystemTime,
	CreateDumpFile,
	MiniDumpWriteDump,
	Timeout
};

void InstallCrashHandler();
CrashReporterErrorCode GenerateParentProcessDump(std::wstring_view commandLine, const std::string &config);
