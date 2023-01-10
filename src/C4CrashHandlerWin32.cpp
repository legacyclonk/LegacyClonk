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
#include "C4Version.h"
#include "C4Windows.h"

#include <dbghelp.h>
#include <strsafe.h>
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

static constexpr size_t DumpBufferSize = 1024;
static wchar_t DumpBuffer[DumpBufferSize];

#define FORMAT_STRING L"%" _CRT_WIDE(SCNuPTR) L"|%" _CRT_WIDE(SCNuPTR) L"|%" _CRT_WIDE(SCNuPTR) L"|%" _CRT_WIDE(SCNuPTR)

template<std::intptr_t InvalidValue>
using Handle = std::unique_ptr<std::remove_pointer_t<HANDLE>, decltype([](const HANDLE handle) { if (handle != std::bit_cast<HANDLE>(InvalidValue)) { CloseHandle(handle); } })>;
using HandleNull = Handle<0>;

static bool PathValid(const char *const path)
{
	__try
	{
		return FileExists(path);
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		return false;
	}
}

LONG WINAPI GenerateDump(LPEXCEPTION_POINTERS exceptionPointers)
{
	if (!FirstCrash) return EXCEPTION_CONTINUE_SEARCH;
	FirstCrash = false;

	{
		wchar_t *buffer{DumpBuffer};
		if (_get_wpgmptr(&buffer))
		{
			return EXCEPTION_CONTINUE_SEARCH;
		}
	}

	const auto duplicateHandle = [](const HANDLE handle, const DWORD access) -> HandleNull
	{
		HANDLE target;
		if (!DuplicateHandle(GetCurrentProcess(), handle, GetCurrentProcess(), &target, access, true, 0))
		{
			return {};
		}

		if (!SetHandleInformation(target, HANDLE_FLAG_INHERIT, HANDLE_FLAG_INHERIT))
		{
			return {};
		}

		return HandleNull{target};
	};

	const auto process = duplicateHandle(GetCurrentProcess(), PROCESS_QUERY_INFORMATION | PROCESS_VM_READ | PROCESS_DUP_HANDLE);
	if (!process)
	{
		return EXCEPTION_CONTINUE_SEARCH;
	}

	const auto thread = duplicateHandle(GetCurrentThread(), THREAD_ALL_ACCESS);
	if (!thread)
	{
		return EXCEPTION_CONTINUE_SEARCH;
	}

	const HandleNull event{CreateEvent(nullptr, false, false, nullptr)};
	if (!event)
	{
		return EXCEPTION_CONTINUE_SEARCH;
	}

	if (!SetHandleInformation(event.get(), HANDLE_FLAG_INHERIT, HANDLE_FLAG_INHERIT))
	{
		return EXCEPTION_CONTINUE_SEARCH;
	}

	const std::size_t pathSize{std::wcslen(DumpBuffer)};
	wchar_t *const commandLineBuffer{DumpBuffer + pathSize + 1};

	wchar_t *end;
	std::size_t remaining;
	if (FAILED(StringCchPrintfExW(
				   commandLineBuffer,
				   DumpBufferSize - pathSize - 1,
				   &end,
				   &remaining,
				   0,
				   L"/crashhandler:" FORMAT_STRING,
				   std::bit_cast<std::uintptr_t>(process.get()),
				   std::bit_cast<std::uintptr_t>(thread.get()),
				   std::bit_cast<std::uintptr_t>(event.get()),
				   std::bit_cast<std::uintptr_t>(exceptionPointers)
				)))
	{
		return EXCEPTION_CONTINUE_SEARCH;
	}

	const char *const configFilename{Config.ConfigFilename.getData()};
	const wchar_t *configFilenameWide{nullptr};

	// The path might be corrupted - check whether it exists and catch any exceptions that might happen
	if (configFilename && PathValid(configFilename))
	{
		wchar_t *const originalEnd{end};

		const std::size_t copied{std::wstring_view{L"/config:"}.copy(end, remaining)};
		end += copied;
		remaining -= copied;

		if (!MultiByteToWideChar(CP_ACP, 0, configFilename, -1, end, remaining))
		{
			end = originalEnd;
			*end = L'\0';
		}
		else
		{
			configFilenameWide = end;
		}
	}

	STARTUPINFOW startupInfo{.cb = sizeof(STARTUPINFO)};
	PROCESS_INFORMATION processInformation{};

	if (!CreateProcessW(
					DumpBuffer,
					commandLineBuffer,
					nullptr,
					nullptr,
					true,
					NORMAL_PRIORITY_CLASS | DETACHED_PROCESS,
					nullptr,
					nullptr,
					&startupInfo,
					&processInformation))
	{
		return EXCEPTION_CONTINUE_SEARCH;
	}

	CloseHandle(processInformation.hThread);

	bool success{false};
	const HANDLE handles[]{event.get(), processInformation.hProcess};

	switch (WaitForMultipleObjects(std::size(handles), handles, false, 5000))
	{
	case WAIT_TIMEOUT: // Process hangs, terminate it
		TerminateProcess(handles[1], STATUS_TIMEOUT);
		break;

	case WAIT_OBJECT_0: // Process succeeded
		success = true;
		break;

	case WAIT_OBJECT_0 + 1: // Process exited
	{
		DWORD exitCode;
		GetExitCodeProcess(handles[1], &exitCode);
		success = exitCode == C4XRV_Completed;
		break;
	}

	default:
		break;
	}

	if (!success)
	{
		MessageBoxW(nullptr, L"LegacyClonk crashed. Crash reporter non-functional.", _CRT_WIDE(STD_PRODUCT), MB_ICONERROR);
	}

	TerminateProcess(GetCurrentProcess(), exceptionPointers->ExceptionRecord->ExceptionCode);
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

static bool ValidateHandle(HandleNull &handle)
{
	HANDLE handleDuplicate;
	if (!DuplicateHandle(GetCurrentProcess(), handle.get(), GetCurrentProcess(), &handleDuplicate, 0, true, DUPLICATE_SAME_ACCESS))
	{
		return false;
	}

	return true;
}

static bool ParseCommandLine(const std::wstring_view commandLine, HandleNull &process, HandleNull &thread, HandleNull &event, std::uintptr_t &exceptionPointerAddress)
{
	std::uintptr_t processRaw;
	std::uintptr_t threadRaw;
	std::uintptr_t eventRaw;
	if (std::swscanf(commandLine.data(), FORMAT_STRING, &processRaw, &threadRaw, &eventRaw, &exceptionPointerAddress) != 4)
	{
		return false;
	}

	process.reset(std::bit_cast<HANDLE>(processRaw));
	thread.reset(std::bit_cast<HANDLE>(threadRaw));
	event.reset(std::bit_cast<HANDLE>(eventRaw));
	return true;
}

int GenerateParentProcessDump(const std::wstring_view commandLine, const std::string &config)
{
	HandleNull process;
	HandleNull thread;
	HandleNull event;
	std::uintptr_t exceptionPointerAddress;

	if (!ParseCommandLine(commandLine, process, thread, event, exceptionPointerAddress))
	{
		return C4XRV_Failure;
	}

	if (!ValidateHandle(process) || !ValidateHandle(thread) || !ValidateHandle(event))
	{
		return C4XRV_Failure;
	}

	Config.Init();
	if (!Config.Load(true, config.empty() ? nullptr : config.c_str()))
	{
		return C4XRV_Failure;
	}

	DWORD size{ExpandEnvironmentStrings(Config.General.UserPath, nullptr, 0)};
	if (!size)
	{
		return C4XRV_Failure;
	}

	const std::size_t sizeWithFilename{size + 1 + std::char_traits<char>::length(C4ENGINENAME "-crash-YYYY-MM-DD-HH-MM-SS.dmp")};

	const auto buffer = std::make_unique_for_overwrite<char[]>(sizeWithFilename);
	size = ExpandEnvironmentStrings(Config.General.UserPath, buffer.get(), size);
	if (!size)
	{
		return C4XRV_Failure;
	}

	// C4Config::Load will attempt to create the user path; abort if it failed
	if (!DirectoryExists(buffer.get()))
	{
		return C4XRV_Failure;
	}

	buffer.get()[size - 1] = '\\';

	SYSTEMTIME st;
	GetSystemTime(&st);

	if (sprintf_s(
				buffer.get() + size, sizeWithFilename - size,
				"%s-crash-%04d-%02d-%02d-%02d-%02d-%02d.dmp",
				C4ENGINENAME,
				st.wYear,
				st.wMonth,
				st.wDay,
				st.wHour,
				st.wMinute,
				st.wSecond
				) == -1)
	{
		return C4XRV_Failure;
	}

	const Handle<-1> file{CreateFile(
					buffer.get(),
					GENERIC_WRITE,
					FILE_SHARE_READ | FILE_SHARE_DELETE,
					nullptr,
					CREATE_NEW,
					FILE_ATTRIBUTE_NORMAL,
					nullptr)};

	if (file.get() == INVALID_HANDLE_VALUE)
	{
		return C4XRV_Failure;
	}

	MINIDUMP_EXCEPTION_INFORMATION information{
		.ThreadId = GetThreadId(thread.get()),
		.ExceptionPointers = std::bit_cast<PEXCEPTION_POINTERS>(exceptionPointerAddress),
		.ClientPointers = true
	};

	if (!MiniDumpWriteDump(
				process.get(),
				GetProcessId(process.get()),
				file.get(),
				MiniDumpNormal,
				&information,
				nullptr,
				nullptr
				))
	{
		return C4XRV_Failure;
	}

	SetEvent(event.get());

	MessageBox(nullptr, "LegacyClonk has crashed", STD_PRODUCT, MB_ICONERROR);
	return C4XRV_Completed;
}
