/*
 * LegacyClonk
 *
 * Copyright (c) 2025, The LegacyClonk Team and contributors
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

#include "StdAdaptors.h"
#include "StdOSVersion.h"

#include <cinttypes>

void CStdOSVersion::CompileFunc(StdCompiler *const comp)
{
	if (comp->isCompiler())
	{
		major = 0;
		minor = 0;
		build = 0;
	}

	comp->Value(mkDefaultAdapt(major, 0));
	if (!comp->Separator(StdCompiler::SEP_SEP)) return;
	comp->Value(mkDefaultAdapt(minor, 0));
	if (!comp->Separator(StdCompiler::SEP_SEP)) return;
	comp->Value(mkDefaultAdapt(build, 0));
}

#ifdef _WIN32

#include "C4Windows.h"

#include <ntstatus.h>
#include <winnt.h>
#include <winternl.h>

using RtlGetVersionFuncPtr = NTSTATUS(NTAPI *)(PRTL_OSVERSIONINFOW);

namespace
{

struct RegKeyDeleter
{
	using pointer = HKEY;

	void operator()(const pointer key)
	{
		RegCloseKey(key);
	}
};

}

CStdOSVersion CStdOSVersion::GetLocal()
{
	static auto *const RtlGetVersion{reinterpret_cast<RtlGetVersionFuncPtr>(GetProcAddress(GetModuleHandle(L"ntdll.dll"), "RtlGetVersion"))};

	if (!RtlGetVersion)
	{
		throw std::runtime_error{std::format("Could not get RtlGetVersion: {}", GetLastError())};
	}

	RTL_OSVERSIONINFOW versionInfo{.dwOSVersionInfoSize = sizeof(versionInfo)};

	if (const NTSTATUS result{RtlGetVersion(&versionInfo)}; result != STATUS_SUCCESS)
	{
		throw std::runtime_error{std::format("RtlGetVersion failed: 0x{:8X}", result)};
	}

	return {
		static_cast<std::uint16_t>(versionInfo.dwMajorVersion),
		static_cast<std::uint16_t>(versionInfo.dwMinorVersion),
		static_cast<std::uint16_t>(versionInfo.dwBuildNumber)
	};
}

std::string CStdOSVersion::GetFriendlyOSName()
{
	using KeyHandle = std::unique_ptr<HKEY, RegKeyDeleter>;

	auto log = [](const LRESULT code) { OutputDebugString(std::format(L"{}\n", code).c_str()); return code; };

	KeyHandle currentVersion;
	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", REG_OPTION_NON_VOLATILE, KEY_READ, std::out_ptr(currentVersion)) == ERROR_SUCCESS)
	{
		std::string result;
		DWORD type;
		DWORD size{0};

		for (;;)
		{
			switch (RegQueryValueExA(currentVersion.get(), "ProductName", nullptr, &type, reinterpret_cast<LPBYTE>(result.data()), &size))
			{
			case ERROR_SUCCESS:
				if (type == REG_SZ)
				{
					result.resize(strnlen(result.c_str(), size));
					return result;
				}
				else
				{
					return "Windows";
				}

			case ERROR_MORE_DATA:
				if (type == REG_SZ)
				{
					result.resize(size);
					continue;
				}
				[[fallthrough]];

			default:
				return "Windows";
			}
		}
	}

	return "Windows";
}

#elif defined(__linux__)

#include <sys/utsname.h>

CStdOSVersion CStdOSVersion::GetLocal()
{
	utsname name;
	uname(&name);

	std::uint16_t major;
	std::uint16_t minor;
	std::uint16_t build;
	std::sscanf(name.release, "%" SCNu16 ".%" SCNu16 ".%" SCNu16, &major, &minor, &build);

	return {major, minor, build};
}

std::string CStdOSVersion::GetFriendlyOSName()
{
	utsname name;
	uname(&name);

	return name.release;
}

#endif
