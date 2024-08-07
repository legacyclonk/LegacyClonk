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
#include "C4Log.h"
#include "C4Version.h"
#include "C4Windows.h"
#include "StdStringEncodingConverter.h"

#include <dbghelp.h>
#include <tlhelp32.h>

#include <cinttypes>

static bool FirstCrash = true;

namespace
{
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

	constexpr size_t DumpBufferSize = 2048;
	constexpr size_t DumpBufferSizeInBytes = DumpBufferSize * sizeof(wchar_t);
	wchar_t DumpBuffer[DumpBufferSize];
	char SymbolBuffer[DumpBufferSize];
	wchar_t UserPathWide[_MAX_PATH] = {L'0'};

	// Dump crash info in a human readable format. Uses a static buffer to avoid heap allocations
	// from an exception handler. For the same reason, this also doesn't use Log/LogF etc.
	void SafeTextDump(LPEXCEPTION_POINTERS exc, int fd, const wchar_t *dumpFilename)
	{
#if defined(_MSC_VER)
#	define LOG_SNPRINTF _snwprintf
#else
#	define LOG_SNPRINTF snwprintf
#endif
#define LOG_STATIC_TEXT(text) write(fd, text, sizeof(text) - sizeof(wchar_t))
#define LOG_DYNAMIC_TEXT(...) write(fd, DumpBuffer, LOG_SNPRINTF(DumpBuffer, DumpBufferSize-sizeof(wchar_t), __VA_ARGS__))

		// Figure out which kind of format string will output a pointer in hex
#if defined(PRIdPTR)
#	define POINTER_FORMAT_SUFFIX _CRT_WIDE(PRIdPTR)
#elif defined(_MSC_VER)
#	define POINTER_FORMAT_SUFFIX L"Ix"
#elif defined(__GNUC__)
#	define POINTER_FORMAT_SUFFIX L"zx"
#else
#	define POINTER_FORMAT_SUFFIX L"p"
#endif
#if LC_MACHINE == LC_MACHINE_X64
#	define POINTER_FORMAT L"0x%016" POINTER_FORMAT_SUFFIX
#elif LC_MACHINE == LC_MACHINE_X86
#	define POINTER_FORMAT L"0x%08" POINTER_FORMAT_SUFFIX
#else
#	define POINTER_FORMAT L"0x%" POINTER_FORMAT_SUFFIX
#endif

#ifndef STATUS_ASSERTION_FAILURE
#	define STATUS_ASSERTION_FAILURE ((DWORD)0xC0000420L)
#endif

		LOG_STATIC_TEXT(L"**********************************************************************\n");
		LOG_STATIC_TEXT(L"* UNHANDLED EXCEPTION\n");

		if (exc->ExceptionRecord->ExceptionCode != STATUS_ASSERTION_FAILURE && dumpFilename && dumpFilename[0] != L'\0')
		{
			LOG_STATIC_TEXT(L"* A crash dump may have been written to ");
			write(fd, dumpFilename, std::wcslen(dumpFilename));
			LOG_STATIC_TEXT(L"\n");
			LOG_STATIC_TEXT(L"* If this file exists, please send it to a developer for investigation.\n");
		}
		LOG_STATIC_TEXT("**********************************************************************\n");
		// Log exception type
		switch (exc->ExceptionRecord->ExceptionCode)
		{
#define LOG_EXCEPTION(code, text) case code: LOG_STATIC_TEXT(_CRT_WIDE(#code) L": " text L"\n"); break
			LOG_EXCEPTION(EXCEPTION_ACCESS_VIOLATION, L"The thread tried to read from or write to a virtual address for which it does not have the appropriate access.");
			LOG_EXCEPTION(EXCEPTION_ILLEGAL_INSTRUCTION, L"The thread tried to execute an invalid instruction.");
			LOG_EXCEPTION(EXCEPTION_IN_PAGE_ERROR, L"The thread tried to access a page that was not present, and the system was unable to load the page.");
			LOG_EXCEPTION(EXCEPTION_NONCONTINUABLE_EXCEPTION, L"The thread tried to continue execution after a noncontinuable exception occurred.");
			LOG_EXCEPTION(EXCEPTION_PRIV_INSTRUCTION, L"The thread tried to execute an instruction whose operation is not allowed in the current machine mode.");
			LOG_EXCEPTION(EXCEPTION_STACK_OVERFLOW, L"The thread used up its stack.");
			LOG_EXCEPTION(EXCEPTION_GUARD_PAGE, L"The thread accessed memory allocated with the PAGE_GUARD modifier.");
			LOG_EXCEPTION(STATUS_ASSERTION_FAILURE, L"The thread specified a pre- or postcondition that did not hold.");
#undef LOG_EXCEPTION
		default:
			LOG_DYNAMIC_TEXT(L"%#08x: The thread raised an unknown exception.\n", static_cast<unsigned int>(exc->ExceptionRecord->ExceptionCode));
			break;
		}
		if (exc->ExceptionRecord->ExceptionFlags == EXCEPTION_NONCONTINUABLE)
			LOG_STATIC_TEXT(L"This is a non-continuable exception.\n");
		else
			LOG_STATIC_TEXT(L"This is a continuable exception.\n");

		// For some exceptions, there is a defined meaning to the ExceptionInformation field
		switch (exc->ExceptionRecord->ExceptionCode)
		{
		case EXCEPTION_ACCESS_VIOLATION:
		case EXCEPTION_IN_PAGE_ERROR:
			if (exc->ExceptionRecord->NumberParameters < 2)
			{
				LOG_STATIC_TEXT(L"Additional information for the exception was not provided.\n");
				break;
			}
			LOG_STATIC_TEXT(L"Additional information for the exception: The thread ");
			switch (exc->ExceptionRecord->ExceptionInformation[0])
			{
#ifndef EXCEPTION_READ_FAULT
#	define EXCEPTION_READ_FAULT 0
#	define EXCEPTION_WRITE_FAULT 1
#	define EXCEPTION_EXECUTE_FAULT 8
#endif
			case EXCEPTION_READ_FAULT: LOG_STATIC_TEXT(L"tried to read from memory"); break;
			case EXCEPTION_WRITE_FAULT: LOG_STATIC_TEXT(L"tried to write to memory"); break;
			case EXCEPTION_EXECUTE_FAULT: LOG_STATIC_TEXT(L"caused an user-mode DEP violation"); break;
			default: LOG_DYNAMIC_TEXT(L"tried to access (%#x) memory", static_cast<unsigned int>(exc->ExceptionRecord->ExceptionInformation[0])); break;
			}
			LOG_DYNAMIC_TEXT(L" at address " POINTER_FORMAT ".\n", static_cast<size_t>(exc->ExceptionRecord->ExceptionInformation[1]));
			if (exc->ExceptionRecord->ExceptionCode == EXCEPTION_IN_PAGE_ERROR)
			{
				if (exc->ExceptionRecord->NumberParameters >= 3)
					LOG_DYNAMIC_TEXT(L"The NTSTATUS code that resulted in this exception was " POINTER_FORMAT ".\n", static_cast<size_t>(exc->ExceptionRecord->ExceptionInformation[2]));
				else
					LOG_STATIC_TEXT(L"The NTSTATUS code that resulted in this exception was not provided.\n");
			}
			break;

		case STATUS_ASSERTION_FAILURE:
			if (exc->ExceptionRecord->NumberParameters < 3)
			{
				LOG_STATIC_TEXT(L"Additional information for the exception was not provided.\n");
				break;
			}

			LOG_DYNAMIC_TEXT(L"Additional information for the exception:\n    Assertion that failed: %s\n    File: %s\n    Line: %d\n",
							 reinterpret_cast<wchar_t *>(exc->ExceptionRecord->ExceptionInformation[0]),
							 reinterpret_cast<wchar_t *>(exc->ExceptionRecord->ExceptionInformation[1]),
							 (int) exc->ExceptionRecord->ExceptionInformation[2]);
			break;
		}

		// Dump registers
#if LC_MACHINE == LC_MACHINE_X64
		LOG_STATIC_TEXT(L"\nProcessor registers (x86_64):\n");
		LOG_DYNAMIC_TEXT(L"RAX: " POINTER_FORMAT L", RBX: " POINTER_FORMAT L", RCX: " POINTER_FORMAT L", RDX: " POINTER_FORMAT L"\n",
						 static_cast<size_t>(exc->ContextRecord->Rax), static_cast<size_t>(exc->ContextRecord->Rbx),
						 static_cast<size_t>(exc->ContextRecord->Rcx), static_cast<size_t>(exc->ContextRecord->Rdx));
		LOG_DYNAMIC_TEXT(L"RBP: " POINTER_FORMAT L", RSI: " POINTER_FORMAT L", RDI: " POINTER_FORMAT L",  R8: " POINTER_FORMAT L"\n",
						 static_cast<size_t>(exc->ContextRecord->Rbp), static_cast<size_t>(exc->ContextRecord->Rsi),
						 static_cast<size_t>(exc->ContextRecord->Rdi), static_cast<size_t>(exc->ContextRecord->R8));
		LOG_DYNAMIC_TEXT(L" R9: " POINTER_FORMAT L", R10: " POINTER_FORMAT L", R11: " POINTER_FORMAT L", R12: " POINTER_FORMAT L"\n",
						 static_cast<size_t>(exc->ContextRecord->R9), static_cast<size_t>(exc->ContextRecord->R10),
						 static_cast<size_t>(exc->ContextRecord->R11), static_cast<size_t>(exc->ContextRecord->R12));
		LOG_DYNAMIC_TEXT(L"R13: " POINTER_FORMAT L", R14: " POINTER_FORMAT L", R15: " POINTER_FORMAT L"\n",
						 static_cast<size_t>(exc->ContextRecord->R13), static_cast<size_t>(exc->ContextRecord->R14),
						 static_cast<size_t>(exc->ContextRecord->R15));
		LOG_DYNAMIC_TEXT(L"RSP: " POINTER_FORMAT L", RIP: " POINTER_FORMAT L"\n",
						 static_cast<size_t>(exc->ContextRecord->Rsp), static_cast<size_t>(exc->ContextRecord->Rip));
#elif LC_MACHINE == LC_MACHINE_X86
		LOG_STATIC_TEXT(L"\nProcessor registers (x86):\n");
		LOG_DYNAMIC_TEXT(L"EAX: " POINTER_FORMAT L", EBX: " POINTER_FORMAT L", ECX: " POINTER_FORMAT L", EDX: " POINTER_FORMAT L"\n",
						 static_cast<size_t>(exc->ContextRecord->Eax), static_cast<size_t>(exc->ContextRecord->Ebx),
						 static_cast<size_t>(exc->ContextRecord->Ecx), static_cast<size_t>(exc->ContextRecord->Edx));
		LOG_DYNAMIC_TEXT(L"ESI: " POINTER_FORMAT L", EDI: " POINTER_FORMAT L"\n",
						 static_cast<size_t>(exc->ContextRecord->Esi), static_cast<size_t>(exc->ContextRecord->Edi));
		LOG_DYNAMIC_TEXT(L"EBP: " POINTER_FORMAT L", ESP: " POINTER_FORMAT L", EIP: " POINTER_FORMAT L"\n",
						 static_cast<size_t>(exc->ContextRecord->Ebp), static_cast<size_t>(exc->ContextRecord->Esp),
						 static_cast<size_t>(exc->ContextRecord->Eip));
#endif
#if LC_MACHINE == LC_MACHINE_X64 || LC_MACHINE == LC_MACHINE_X86
		LOG_DYNAMIC_TEXT(L"EFLAGS: 0x%08x (%c%c%c%c%c%c%c)\n", static_cast<unsigned int>(exc->ContextRecord->EFlags),
						 exc->ContextRecord->EFlags & 0x800 ? L'O' : L'.',	// overflow
						 exc->ContextRecord->EFlags & 0x400 ? L'D' : L'.',	// direction
						 exc->ContextRecord->EFlags & 0x80 ? L'S' : L'.',	// sign
						 exc->ContextRecord->EFlags & 0x40 ? L'Z' : L'.',	// zero
						 exc->ContextRecord->EFlags & 0x10 ? L'A' : L'.',	// auxiliary carry
						 exc->ContextRecord->EFlags & 0x4 ? L'P' : L'.',	// parity
						 exc->ContextRecord->EFlags & 0x1 ? L'C' : L'.');	// carry
#endif

		// Dump stack
		LOG_STATIC_TEXT(L"\nStack contents:\n");
		MEMORY_BASIC_INFORMATION stackInfo;
		intptr_t stackPointer =
#if LC_MACHINE == LC_MACHINE_X64
			exc->ContextRecord->Rsp
#elif LC_MACHINE == LC_MACHINE_X86
			exc->ContextRecord->Esp
#endif
			;
		if (VirtualQuery(reinterpret_cast<LPCVOID>(stackPointer), &stackInfo, sizeof(stackInfo)))
		{
			intptr_t stackBase = reinterpret_cast<intptr_t>(stackInfo.BaseAddress);
			intptr_t dumpMin = std::max<intptr_t>(stackBase, (stackPointer - 256) & ~0xF);
			intptr_t dumpMax = std::min<intptr_t>(stackBase + stackInfo.RegionSize, (stackPointer + 256) | 0xF);

			for (intptr_t dumpRowBase = dumpMin & ~0xF; dumpRowBase < dumpMax; dumpRowBase += 0x10)
			{
				LOG_DYNAMIC_TEXT(POINTER_FORMAT L": ", dumpRowBase);
				// Hex dump
				for (intptr_t dumpRowCursor = dumpRowBase; dumpRowCursor < dumpRowBase + 16; ++dumpRowCursor)
				{
					if (dumpRowCursor < dumpMin || dumpRowCursor > dumpMax)
						LOG_STATIC_TEXT(L"   ");
					else
						LOG_DYNAMIC_TEXT(L"%02x ", (unsigned int) *reinterpret_cast<unsigned char *>(dumpRowCursor)); // Safe, since it's inside the VM of our process
				}
				LOG_STATIC_TEXT(L"   ");
				// Text dump
				for (intptr_t dumpRowCursor = dumpRowBase; dumpRowCursor < dumpRowBase + 16; ++dumpRowCursor)
				{
					if (dumpRowCursor < dumpMin || dumpRowCursor > dumpMax)
						LOG_STATIC_TEXT(L" ");
					else
					{
						unsigned char c = *reinterpret_cast<unsigned char *>(dumpRowCursor); // Safe, since it's inside the VM of our process
						if (c < 0x20 || (c > 0x7e && c < 0xa1))
							LOG_STATIC_TEXT(L".");
						else
							LOG_DYNAMIC_TEXT(L"%c", static_cast<char>(c));
					}
				}
				LOG_STATIC_TEXT(L"\n");
			}
		}
		else
		{
			LOG_STATIC_TEXT(L"[Failed to access stack memory]\n");
		}

		// Initialize DbgHelp.dll symbol functions
		SymSetOptions(SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS | SYMOPT_LOAD_LINES);
		HANDLE process = GetCurrentProcess();
		if (SymInitializeW(process, nullptr, true))
		{
			LOG_STATIC_TEXT(L"\nStack trace:\n");
			auto frame = STACKFRAME64();
			DWORD imageType;
			CONTEXT context = *exc->ContextRecord;
			// Setup frame info
			frame.AddrPC.Mode = AddrModeFlat;
			frame.AddrStack.Mode = AddrModeFlat;
			frame.AddrFrame.Mode = AddrModeFlat;
#if LC_MACHINE == LC_MACHINE_X64
			imageType = IMAGE_FILE_MACHINE_AMD64;
			frame.AddrPC.Offset = context.Rip;
			frame.AddrStack.Offset = context.Rsp;
			// Some compilers use rdi for their frame pointer instead. Let's hope they're in the minority.
			frame.AddrFrame.Offset = context.Rbp;
#elif LC_MACHINE == LC_MACHINE_X86
			imageType = IMAGE_FILE_MACHINE_I386;
			frame.AddrPC.Offset = context.Eip;
			frame.AddrStack.Offset = context.Esp;
			frame.AddrFrame.Offset = context.Ebp;
#endif
			// Dump stack trace
			SYMBOL_INFOW *symbol = reinterpret_cast<SYMBOL_INFOW *>(SymbolBuffer);
			static_assert(DumpBufferSizeInBytes >= sizeof(*symbol), "SYMBOL_INFO too large to fit into buffer");
			IMAGEHLP_MODULEW64 *module = reinterpret_cast<IMAGEHLP_MODULEW64 *>(SymbolBuffer);
			static_assert(DumpBufferSizeInBytes >= sizeof(*module), "IMAGEHLP_MODULE64 too large to fit into buffer");
			IMAGEHLP_LINEW64 *line = reinterpret_cast<IMAGEHLP_LINEW64 *>(SymbolBuffer);
			static_assert(DumpBufferSizeInBytes >= sizeof(*line), "IMAGEHLP_LINE64 too large to fit into buffer");
			int frameNumber = 0;
			while (StackWalk64(imageType, process, GetCurrentThread(), &frame, &context, nullptr, SymFunctionTableAccess64, SymGetModuleBase64, nullptr))
			{
				LOG_DYNAMIC_TEXT(L"#%3d ", frameNumber);
				module->SizeOfStruct = sizeof(*module);
				DWORD64 imageBase = 0;
				if (SymGetModuleInfoW64(process, frame.AddrPC.Offset, module))
				{
					LOG_DYNAMIC_TEXT(L"%s", module->ModuleName);
					imageBase = module->BaseOfImage;
				}
				DWORD64 disp64;
				symbol->MaxNameLen = DumpBufferSize - sizeof(*symbol);
				symbol->SizeOfStruct = sizeof(*symbol);
				if (SymFromAddrW(process, frame.AddrPC.Offset, &disp64, symbol))
				{
					LOG_DYNAMIC_TEXT(L"!%s+%#lx", symbol->Name, static_cast<long>(disp64));
				}
				else if (imageBase > 0)
				{
					LOG_DYNAMIC_TEXT(L"+%#lx", static_cast<long>(frame.AddrPC.Offset - imageBase));
				}
				else
				{
					LOG_DYNAMIC_TEXT(L"%#lx", static_cast<long>(frame.AddrPC.Offset));
				}
				DWORD disp;
				line->SizeOfStruct = sizeof(*line);
				if (SymGetLineFromAddrW64(process, frame.AddrPC.Offset, &disp, line))
				{
					LOG_DYNAMIC_TEXT(L" [%s @ %u]", line->FileName, static_cast<unsigned int>(line->LineNumber));
				}
				LOG_STATIC_TEXT(L"\n");
				++frameNumber;
			}
			SymCleanup(process);
		}
		else
		{
			LOG_STATIC_TEXT(L"[Stack trace not available: failed to initialize Debugging Help Library]\n");
		}

		// Dump loaded modules
		HANDLE snapshot;
		while ((snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, 0)) == INVALID_HANDLE_VALUE)
		{
			if (GetLastError() != ERROR_BAD_LENGTH)
			{
				break;
			}
		}

		if (snapshot != INVALID_HANDLE_VALUE)
		{
			LOG_STATIC_TEXT(L"\nLoaded modules:\n");
			MODULEENTRY32 *module = reinterpret_cast<MODULEENTRY32 *>(SymbolBuffer);
			static_assert(DumpBufferSizeInBytes >= sizeof(*module), "MODULEENTRY32 too large to fit into buffer");
			module->dwSize = sizeof(*module);
			for (BOOL success = Module32First(snapshot, module); success; success = Module32Next(snapshot, module))
			{
				LOG_DYNAMIC_TEXT(L"%32s loaded at " POINTER_FORMAT L" - " POINTER_FORMAT L" (%s)\n", module->szModule,
								 reinterpret_cast<size_t>(module->modBaseAddr), reinterpret_cast<size_t>(module->modBaseAddr + module->modBaseSize),
								 module->szExePath);
			}
			CloseHandle(snapshot);
		}
#undef POINTER_FORMAT_SUFFIX
#undef POINTER_FORMAT
#undef LOG_SNPRINTF
#undef LOG_DYNAMIC_TEXT
#undef LOG_STATIC_TEXT
	}
}
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

	wchar_t filenameBuffer[_MAX_PATH + sizeof(L"\\\\?\\")] = {L'\0'}; // extra chars for GetFinalPathNameByHandleA, null byte space included
	std::wmemcpy(filenameBuffer, UserPathWide, std::wcslen(UserPathWide));

	auto *filename = reinterpret_cast<LPWSTR>(DumpBuffer);

	ExpandEnvironmentStrings(filenameBuffer, filename, _MAX_PATH);

	static constexpr auto directoryExists = [](std::wstring_view filename) -> bool
	{
		// Ignore trailing slash or backslash
		wchar_t bufFilename[_MAX_PATH + 1];
		if (!filename.empty())
		{
			if (filename.ends_with(L'\\') || filename.ends_with(L'/'))
			{
				bufFilename[filename.copy(bufFilename, filename.size() - 1)] = L'\0';
				filename = {bufFilename, filename.size() - 1};
			}
		}

		WIN32_FIND_DATA fdt;
		HANDLE handle;
		if ((handle = FindFirstFile(filename.data(), &fdt)) == INVALID_HANDLE_VALUE) return false;
		FindClose(handle);

		return fdt.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY;
	};

	if (!directoryExists(filename))
	{
		// Config corrupted or broken
		filename[0] = L'\0';
	}

	HANDLE file = INVALID_HANDLE_VALUE;
	if (filename[0] != L'\0')
	{
		// There is some path where we want to store our data
		const wchar_t tmpl[] = _CRT_WIDE(C4ENGINENAME) L"-crash-YYYY-MM-DD-HH-MM-SS.dmp";
		size_t pathLength = std::wcslen(filename);
		if (pathLength + std::size(tmpl) > DumpBufferSize)
		{
			// Somehow the length of the required path is too long to fit in
			// our buffer. Don't dump anything then.
			filename[0] = '\0';
		}
		else
		{
			// Make sure the path ends in a backslash.
			if (filename[pathLength - 1] != L'\\')
			{
				filename[pathLength] = L'\\';
				filename[++pathLength] = L'\0';
			}
			SYSTEMTIME st;
			GetSystemTime(&st);

			auto *ptr = filename + pathLength;
			// For some reason, using just %s ends up reading the argument as an ANSI string,
			//even though swprintf should read it as a wide string.
			swprintf_s(ptr, _MAX_PATH - pathLength, L"%ls-crash-%04d-%02d-%02d-%02d-%02d-%02d.dmp",
					_CRT_WIDE(C4ENGINENAME), st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
		}
	}

	if (filename[0] != L'\0')
	{
		file = CreateFile(filename, GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_DELETE, nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
		// If we can't create a *new* file to dump into, don't dump at all.
		if (file == INVALID_HANDLE_VALUE)
		{
			filename[0] = L'\0';
		}
	}

	wchar_t buffer[DumpBufferSize] = {L'\0'};
	std::wcscat(buffer, L"LegacyClonk crashed. Please report this crash ");

	if (GetLogFD() != -1 || filename[0] != L'\0')
	{
		std::wcscat(buffer, L"together with the following information to the developers:\n");

		if (GetLogFD() != -1)
		{
			std::wcscat(buffer, L"\nYou can find detailed information in ");
			GetFinalPathNameByHandle(reinterpret_cast<HANDLE>(_get_osfhandle(GetLogFD())), filenameBuffer, std::size(filenameBuffer), 0);
			std::wcscat(std::wcscat(buffer, filenameBuffer), L".");
		}

		if (filename[0] != L'\0')
		{
			std::wcscat(std::wcscat(std::wcscat(buffer, L"\nA crash dump has been generated at "), filename), L".");
		}
	}
	else
	{
		std::wcscat(buffer, L"to the developers.");
	}

	// Write dump (human readable format)
	if (GetLogFD() != -1)
	{
		SafeTextDump(pExceptionPointers, GetLogFD(), filename);
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

	MessageBox(nullptr, buffer, L"LegacyClonk crashed", MB_ICONERROR);

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

	struct dump_thread_t
	{
		HANDLE thread;
		const wchar_t *expression, *file;
		size_t line;
	};
	// Helper function to get a valid thread context for the main thread
	static DWORD WINAPI dumpThread(LPVOID t)
	{
		dump_thread_t *data = static_cast<dump_thread_t *>(t);

		// Stop calling thread so we can take a snapshot
		if (SuspendThread(data->thread) == -1)
			return FALSE;

		// Get thread info
		auto ctx = CONTEXT();
#ifndef CONTEXT_ALL
#define CONTEXT_ALL (CONTEXT_CONTROL | CONTEXT_INTEGER | CONTEXT_SEGMENTS | \
	CONTEXT_FLOATING_POINT | CONTEXT_DEBUG_REGISTERS | CONTEXT_EXTENDED_REGISTERS)
#endif
		ctx.ContextFlags = CONTEXT_ALL;
		BOOL result = GetThreadContext(data->thread, &ctx);

		// Setup a fake exception to log
		auto erec = EXCEPTION_RECORD();
		erec.ExceptionCode = STATUS_ASSERTION_FAILURE;
		erec.ExceptionFlags = 0L;
		erec.ExceptionInformation[0] = (ULONG_PTR) data->expression;
		erec.ExceptionInformation[1] = (ULONG_PTR) data->file;
		erec.ExceptionInformation[2] = (ULONG_PTR) data->line;
		erec.NumberParameters = 3;

		erec.ExceptionAddress = (LPVOID)
#if LC_MACHINE == LC_MACHINE_X64
			ctx.Rip
#elif LC_MACHINE == LC_MACHINE_X86
			ctx.Eip
#else
			0
#endif
			;
		EXCEPTION_POINTERS eptr;
		eptr.ContextRecord = &ctx;
		eptr.ExceptionRecord = &erec;

		// Log
		if (GetLogFD() != -1)
			SafeTextDump(&eptr, GetLogFD(), nullptr);

		// Continue caller
		if (ResumeThread(data->thread) == -1)
			abort();
		return result;
	}

	// Replacement assertion handler
	void __cdecl assertionHandler(const wchar_t *expression, const wchar_t *file, unsigned line)
	{
		// Dump thread status on a different thread because we can't get a valid thread context otherwise
		HANDLE thisThread;
		DuplicateHandle(GetCurrentProcess(), GetCurrentThread(), GetCurrentProcess(), &thisThread, 0, FALSE, DUPLICATE_SAME_ACCESS);
		dump_thread_t dumpThreadData = {
			thisThread,
			expression, file, line
		};
		HANDLE ctxThread = CreateThread(nullptr, 0L, &dumpThread, &dumpThreadData, 0L, nullptr);
		WaitForSingleObject(ctxThread, INFINITE);
		CloseHandle(thisThread);
		CloseHandle(ctxThread);
		// Unhook _wassert/_assert
		UnhookAssert();
		// Call old _wassert/_assert
		assertFunc(expression, file, line);
		// If we get here: rehook
		HookAssert(&assertionHandler);
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

namespace C4CrashHandlerWin32
{
	void SetUserPath(const std::string_view userPath)
	{
		const std::wstring wide{StdStringEncodingConverter::WinAcpToUtf16(userPath)};
		UserPathWide[wide.copy(UserPathWide, sizeof(UserPathWide) - 1)] = L'\0';
	}
}
