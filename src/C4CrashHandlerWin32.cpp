/*
 * LegacyClonk
 *
 * Copyright (c) 1998-2000, Matthes Bender
 * Copyright (c) 2001-2009, RedWolf Design GmbH, http://www.clonk.de/
 * Copyright (c) 2009-2016, The OpenClonk Team and contributors
 * Copyright (c) 2020, The LegacyClonk Team and contributors
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

 // Crash handler, Win32 version

#include "C4Include.h"

// Dump generation on crash
#include "C4Config.h"
#include "C4Log.h"
#include "C4Version.h"
#include "C4Windows.h"

#include <dbghelp.h>
#include <tlhelp32.h>

#include <cinttypes>

static bool FirstCrash = true;

#define LC_MACHINE_UNKNOWN 0x0
#define LC_MACHINE_X86     0x1
#define LC_MACHINE_X64     0x2
#if defined(_M_X64) || defined(__amd64)
#	define LC_MACHINE LC_MACHINE_X64
#elif defined(_M_IX86) || defined(__i386__)
#	define LC_MACHINE LC_MACHINE_X86
#else
#	define LC_MACHINE LC_MACHINE_UNKNOWN
#endif

static constexpr size_t DumpBufferSize = 2048;
static char DumpBuffer[DumpBufferSize];

LONG WINAPI GenerateDump(EXCEPTION_POINTERS *pExceptionPointers)
{
	enum
	{
		MDST_BuildId = LastReservedStream + 1
	};

	if (!FirstCrash) return EXCEPTION_CONTINUE_SEARCH;
	FirstCrash = false;

	// Open dump file
	// Work on the assumption that the config isn't corrupted

	char filenameBuffer[_MAX_PATH + sizeof("\\\\?\\")] = {'\0'}; // extra chars for GetFinalPathNameByHandleA, null byte space included
	strncpy(filenameBuffer, Config.General.UserPath, strnlen(Config.General.UserPath, sizeof(Config.General.UserPath)));

	auto *filename = reinterpret_cast<LPSTR>(DumpBuffer);

	ExpandEnvironmentStringsA(filenameBuffer, filename, _MAX_PATH);

	if (!DirectoryExists(filename))
	{
		// Config corrupted or broken
		filename[0] = '\0';
	}

	HANDLE file = INVALID_HANDLE_VALUE;
	if (filename[0] != '\0')
	{
		// There is some path where we want to store our data
		const char tmpl[] = C4ENGINENAME "-crash-YYYY-MM-DD-HH-MM-SS.dmp";
		size_t pathLength = strlen(filename);
		if (pathLength + sizeof(tmpl) / sizeof(*tmpl) > DumpBufferSize)
		{
			// Somehow the length of the required path is too long to fit in
			// our buffer. Don't dump anything then.
			filename[0] = '\0';
		}
		else
		{
			// Make sure the path ends in a backslash.
			if (filename[pathLength - 1] != '\\')
			{
				filename[pathLength] = '\\';
				filename[++pathLength] = '\0';
			}
			SYSTEMTIME st;
			GetSystemTime(&st);

			auto *ptr = filename + pathLength;
			sprintf_s(ptr, _MAX_PATH - pathLength, "%s-crash-%04d-%02d-%02d-%02d-%02d-%02d.dmp",
					C4ENGINENAME, st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
		}
	}

	if (filename[0] != '\0')
	{
		file = CreateFileA(filename, GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_DELETE, nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
		// If we can't create a *new* file to dump into, don't dump at all.
		if (file == INVALID_HANDLE_VALUE)
		{
			filename[0] = '\0';
		}
	}

	char buffer[DumpBufferSize] = {'\0'};
	strcat(buffer, "LegacyClonk crashed. Please report this crash ");

	if (GetLogFD() != -1 || filename[0] != '\0')
	{
		strcat(buffer, "together with the following information to the developers:\n");

		if (GetLogFD() != -1)
		{
			strcat(buffer, "\nYou can find detailed information in ");
			GetFinalPathNameByHandleA(reinterpret_cast<HANDLE>(_get_osfhandle(GetLogFD())), filenameBuffer, sizeof(filenameBuffer), 0);
			strcat(strcat(buffer, filenameBuffer), ".");
		}

		if (filename[0] != '\0')
		{
			strcat(strcat(strcat(buffer, "\nA crash dump has been generated at "), filename), ".");
		}
	}
	else
	{
		strcat(buffer, "to the developers.");
	}


	if (file != INVALID_HANDLE_VALUE)
	{
		MINIDUMP_EXCEPTION_INFORMATION ExpParam;
		ExpParam.ThreadId = GetCurrentThreadId();
		ExpParam.ExceptionPointers = pExceptionPointers;
		ExpParam.ClientPointers = true;
		MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(),
						  file, MiniDumpNormal, &ExpParam, nullptr, nullptr);
		CloseHandle(file);
	}

	MessageBoxA(nullptr, buffer, "LegacyClonk crashed", MB_ICONERROR);

	// Call native exception handler
	return EXCEPTION_CONTINUE_SEARCH;
}

static LONG WINAPI HandleHeapCorruption(PEXCEPTION_POINTERS exceptionPointers)
{
	if (exceptionPointers->ExceptionRecord->ExceptionCode == STATUS_HEAP_CORRUPTION)
	{
		return GenerateDump(exceptionPointers);
	}

	return EXCEPTION_CONTINUE_SEARCH;
}

#ifndef NDEBUG
namespace
{
	// Assertion logging hook. This will replace the prologue of the standard assertion
	// handler with a trampoline to assertionHandler(), which logs the assertion, then
	// replaces the trampoline with the original prologue, and calls the handler.
	// If the standard handler returns control to assertionHandler(), it will then
	// restore the hook.
	typedef void(__cdecl *ASSERT_FUNC)(const wchar_t *, const wchar_t *, unsigned);
	const ASSERT_FUNC assertFunc{&_wassert};

	unsigned char trampoline[] = {
#if LC_MACHINE == LC_MACHINE_X64
		// MOV rax, 0xCCCCCCCCCCCCCCCC
		0x48 /* REX.W */, 0xB8, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC,
		// JMP rax
		0xFF, 0xE0
#elif LC_MACHINE == LC_MACHINE_X86
		// NOP ; to align jump target
		0x90,
		// MOV eax, 0xCCCCCCCC
		0xB8, 0xCC, 0xCC, 0xCC, 0xCC,
		// JMP eax
		0xFF, 0xE0
#endif
	};
	unsigned char trampolineBackup[sizeof(trampoline)];
	void HookAssert(ASSERT_FUNC hook)
	{
		// Write hook function address to trampoline
		memcpy(trampoline + 2, reinterpret_cast<void *>(&hook), sizeof(void *));
		// Make target location writable
		DWORD oldProtect = 0;
		if (!VirtualProtect(reinterpret_cast<LPVOID>(assertFunc), sizeof(trampoline), PAGE_EXECUTE_READWRITE, &oldProtect))
			return;
		// Take backup of old target function and replace it with trampoline
		memcpy(trampolineBackup, reinterpret_cast<void *>(assertFunc), sizeof(trampolineBackup));
		memcpy(reinterpret_cast<void *>(assertFunc), trampoline, sizeof(trampoline));
		// Restore memory protection
		VirtualProtect(reinterpret_cast<LPVOID>(assertFunc), sizeof(trampoline), oldProtect, &oldProtect);
		// Flush processor caches. Not strictly necessary on x86 and x64.
		FlushInstructionCache(GetCurrentProcess(), reinterpret_cast<LPCVOID>(assertFunc), sizeof(trampoline));
	}
	void UnhookAssert()
	{
		DWORD oldProtect = 0;
		if (!VirtualProtect(reinterpret_cast<LPVOID>(assertFunc), sizeof(trampolineBackup), PAGE_EXECUTE_READWRITE, &oldProtect))
			// Couldn't make assert function writable. Abort program (it's what assert() is supposed to do anyway).
			abort();
		// Replace function with backup
		memcpy(reinterpret_cast<void *>(assertFunc), trampolineBackup, sizeof(trampolineBackup));
		VirtualProtect(reinterpret_cast<LPVOID>(assertFunc), sizeof(trampolineBackup), oldProtect, &oldProtect);
		FlushInstructionCache(GetCurrentProcess(), reinterpret_cast<LPCVOID>(assertFunc), sizeof(trampolineBackup));
	}

	// Replacement assertion handler
	void __cdecl assertionHandler(const wchar_t *expression, const wchar_t *file, unsigned line)
	{
		ULONG_PTR arguments[]{
			reinterpret_cast<ULONG_PTR>(expression),
			reinterpret_cast<ULONG_PTR>(file),
			static_cast<ULONG_PTR>(line)
		};

		RaiseException(STATUS_ASSERTION_FAILURE, 0, std::size(arguments), arguments);
	}
}
#endif

void InstallCrashHandler()
{
	// Disable process-wide callback filter for exceptions on Windows Vista.
	// Newer versions of Windows already get this disabled by the application
	// manifest. Without turning this off, we won't be able to handle crashes
	// inside window procedures on 64-bit Windows, regardless of whether we
	// are 32 or 64 bit ourselves.
	typedef BOOL(WINAPI *SetProcessUserModeExceptionPolicyProc)(DWORD);
	typedef BOOL(WINAPI *GetProcessUserModeExceptionPolicyProc)(LPDWORD);
	HMODULE kernel32 = LoadLibrary(TEXT("kernel32"));
	const SetProcessUserModeExceptionPolicyProc SetProcessUserModeExceptionPolicy =
		(SetProcessUserModeExceptionPolicyProc) GetProcAddress(kernel32, "SetProcessUserModeExceptionPolicy");
	const GetProcessUserModeExceptionPolicyProc GetProcessUserModeExceptionPolicy =
		(GetProcessUserModeExceptionPolicyProc) GetProcAddress(kernel32, "GetProcessUserModeExceptionPolicy");
#ifndef PROCESS_CALLBACK_FILTER_ENABLED
#	define PROCESS_CALLBACK_FILTER_ENABLED 0x1
#endif
	if (SetProcessUserModeExceptionPolicy && GetProcessUserModeExceptionPolicy)
	{
		DWORD flags;
		if (GetProcessUserModeExceptionPolicy(&flags))
		{
			SetProcessUserModeExceptionPolicy(flags & ~PROCESS_CALLBACK_FILTER_ENABLED);
		}
	}
	FreeLibrary(kernel32);

	SetUnhandledExceptionFilter(GenerateDump);
	AddVectoredExceptionHandler(0, HandleHeapCorruption);

#ifndef NDEBUG
	// Hook _wassert/_assert, unless we're running under a debugger
	if (!IsDebuggerPresent())
		HookAssert(&assertionHandler);
#endif
}
