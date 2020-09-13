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

#if defined(__CRT_WIDE) || (defined(_MSC_VER) && _MSC_VER >= 1900)
#define USE_WIDE_ASSERT
#endif

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
	char DumpBuffer[DumpBufferSize];
	char SymbolBuffer[DumpBufferSize];
	// Dump crash info in a human readable format. Uses a static buffer to avoid heap allocations
	// from an exception handler. For the same reason, this also doesn't use Log/LogF etc.
	void SafeTextDump(LPEXCEPTION_POINTERS exc, int fd, const char *dumpFilename)
	{
#if defined(_MSC_VER)
#	define LOG_SNPRINTF _snprintf
#else
#	define LOG_SNPRINTF snprintf
#endif
#define LOG_STATIC_TEXT(text) write(fd, text, sizeof(text) - 1)
#define LOG_DYNAMIC_TEXT(...) write(fd, DumpBuffer, LOG_SNPRINTF(DumpBuffer, DumpBufferSize-1, __VA_ARGS__))

		// Figure out which kind of format string will output a pointer in hex
#if defined(PRIdPTR)
#	define POINTER_FORMAT_SUFFIX PRIdPTR
#elif defined(_MSC_VER)
#	define POINTER_FORMAT_SUFFIX "Ix"
#elif defined(__GNUC__)
#	define POINTER_FORMAT_SUFFIX "zx"
#else
#	define POINTER_FORMAT_SUFFIX "p"
#endif
#if LC_MACHINE == LC_MACHINE_X64
#	define POINTER_FORMAT "0x%016" POINTER_FORMAT_SUFFIX
#elif LC_MACHINE == LC_MACHINE_X86
#	define POINTER_FORMAT "0x%08" POINTER_FORMAT_SUFFIX
#else
#	define POINTER_FORMAT "0x%" POINTER_FORMAT_SUFFIX
#endif

#ifndef STATUS_ASSERTION_FAILURE
#	define STATUS_ASSERTION_FAILURE ((DWORD)0xC0000420L)
#endif

		LOG_STATIC_TEXT("**********************************************************************\n");
		LOG_STATIC_TEXT("* UNHANDLED EXCEPTION\n");

		if (exc->ExceptionRecord->ExceptionCode != STATUS_ASSERTION_FAILURE && dumpFilename && dumpFilename[0] != '\0')
		{
			LOG_STATIC_TEXT("* A crash dump may have been written to ");
			write(fd, dumpFilename, strlen(dumpFilename));
			LOG_STATIC_TEXT("\n");
			LOG_STATIC_TEXT("* If this file exists, please send it to a developer for investigation.\n");
		}
		LOG_STATIC_TEXT("**********************************************************************\n");
		// Log exception type
		switch (exc->ExceptionRecord->ExceptionCode)
		{
#define LOG_EXCEPTION(code, text) case code: LOG_STATIC_TEXT(#code ": " text "\n"); break
			LOG_EXCEPTION(EXCEPTION_ACCESS_VIOLATION, "The thread tried to read from or write to a virtual address for which it does not have the appropriate access.");
			LOG_EXCEPTION(EXCEPTION_ILLEGAL_INSTRUCTION, "The thread tried to execute an invalid instruction.");
			LOG_EXCEPTION(EXCEPTION_IN_PAGE_ERROR, "The thread tried to access a page that was not present, and the system was unable to load the page.");
			LOG_EXCEPTION(EXCEPTION_NONCONTINUABLE_EXCEPTION, "The thread tried to continue execution after a noncontinuable exception occurred.");
			LOG_EXCEPTION(EXCEPTION_PRIV_INSTRUCTION, "The thread tried to execute an instruction whose operation is not allowed in the current machine mode.");
			LOG_EXCEPTION(EXCEPTION_STACK_OVERFLOW, "The thread used up its stack.");
			LOG_EXCEPTION(EXCEPTION_GUARD_PAGE, "The thread accessed memory allocated with the PAGE_GUARD modifier.");
			LOG_EXCEPTION(STATUS_ASSERTION_FAILURE, "The thread specified a pre- or postcondition that did not hold.");
#undef LOG_EXCEPTION
		default:
			LOG_DYNAMIC_TEXT("%#08x: The thread raised an unknown exception.\n", static_cast<unsigned int>(exc->ExceptionRecord->ExceptionCode));
			break;
		}
		if (exc->ExceptionRecord->ExceptionFlags == EXCEPTION_NONCONTINUABLE)
			LOG_STATIC_TEXT("This is a non-continuable exception.\n");
		else
			LOG_STATIC_TEXT("This is a continuable exception.\n");

		// For some exceptions, there is a defined meaning to the ExceptionInformation field
		switch (exc->ExceptionRecord->ExceptionCode)
		{
		case EXCEPTION_ACCESS_VIOLATION:
		case EXCEPTION_IN_PAGE_ERROR:
			if (exc->ExceptionRecord->NumberParameters < 2)
			{
				LOG_STATIC_TEXT("Additional information for the exception was not provided.\n");
				break;
			}
			LOG_STATIC_TEXT("Additional information for the exception: The thread ");
			switch (exc->ExceptionRecord->ExceptionInformation[0])
			{
#ifndef EXCEPTION_READ_FAULT
#	define EXCEPTION_READ_FAULT 0
#	define EXCEPTION_WRITE_FAULT 1
#	define EXCEPTION_EXECUTE_FAULT 8
#endif
			case EXCEPTION_READ_FAULT: LOG_STATIC_TEXT("tried to read from memory"); break;
			case EXCEPTION_WRITE_FAULT: LOG_STATIC_TEXT("tried to write to memory"); break;
			case EXCEPTION_EXECUTE_FAULT: LOG_STATIC_TEXT("caused an user-mode DEP violation"); break;
			default: LOG_DYNAMIC_TEXT("tried to access (%#x) memory", static_cast<unsigned int>(exc->ExceptionRecord->ExceptionInformation[0])); break;
			}
			LOG_DYNAMIC_TEXT(" at address " POINTER_FORMAT ".\n", static_cast<size_t>(exc->ExceptionRecord->ExceptionInformation[1]));
			if (exc->ExceptionRecord->ExceptionCode == EXCEPTION_IN_PAGE_ERROR)
			{
				if (exc->ExceptionRecord->NumberParameters >= 3)
					LOG_DYNAMIC_TEXT("The NTSTATUS code that resulted in this exception was " POINTER_FORMAT ".\n", static_cast<size_t>(exc->ExceptionRecord->ExceptionInformation[2]));
				else
					LOG_STATIC_TEXT("The NTSTATUS code that resulted in this exception was not provided.\n");
			}
			break;

		case STATUS_ASSERTION_FAILURE:
			if (exc->ExceptionRecord->NumberParameters < 3)
			{
				LOG_STATIC_TEXT("Additional information for the exception was not provided.\n");
				break;
			}
#ifdef USE_WIDE_ASSERT
#	define ASSERTION_INFO_FORMAT "%ls"
#	define ASSERTION_INFO_TYPE wchar_t *
#else
#	define ASSERTION_INFO_FORMAT "%s"
#	define ASSERTION_INFO_TYPE char *
#endif
			LOG_DYNAMIC_TEXT("Additional information for the exception:\n    Assertion that failed: " ASSERTION_INFO_FORMAT "\n    File: " ASSERTION_INFO_FORMAT "\n    Line: %d\n",
							 reinterpret_cast<ASSERTION_INFO_TYPE>(exc->ExceptionRecord->ExceptionInformation[0]),
							 reinterpret_cast<ASSERTION_INFO_TYPE>(exc->ExceptionRecord->ExceptionInformation[1]),
							 (int) exc->ExceptionRecord->ExceptionInformation[2]);
			break;
		}

		// Dump registers
#if LC_MACHINE == LC_MACHINE_X64
		LOG_STATIC_TEXT("\nProcessor registers (x86_64):\n");
		LOG_DYNAMIC_TEXT("RAX: " POINTER_FORMAT ", RBX: " POINTER_FORMAT ", RCX: " POINTER_FORMAT ", RDX: " POINTER_FORMAT "\n",
						 static_cast<size_t>(exc->ContextRecord->Rax), static_cast<size_t>(exc->ContextRecord->Rbx),
						 static_cast<size_t>(exc->ContextRecord->Rcx), static_cast<size_t>(exc->ContextRecord->Rdx));
		LOG_DYNAMIC_TEXT("RBP: " POINTER_FORMAT ", RSI: " POINTER_FORMAT ", RDI: " POINTER_FORMAT ",  R8: " POINTER_FORMAT "\n",
						 static_cast<size_t>(exc->ContextRecord->Rbp), static_cast<size_t>(exc->ContextRecord->Rsi),
						 static_cast<size_t>(exc->ContextRecord->Rdi), static_cast<size_t>(exc->ContextRecord->R8));
		LOG_DYNAMIC_TEXT(" R9: " POINTER_FORMAT ", R10: " POINTER_FORMAT ", R11: " POINTER_FORMAT ", R12: " POINTER_FORMAT "\n",
						 static_cast<size_t>(exc->ContextRecord->R9), static_cast<size_t>(exc->ContextRecord->R10),
						 static_cast<size_t>(exc->ContextRecord->R11), static_cast<size_t>(exc->ContextRecord->R12));
		LOG_DYNAMIC_TEXT("R13: " POINTER_FORMAT ", R14: " POINTER_FORMAT ", R15: " POINTER_FORMAT "\n",
						 static_cast<size_t>(exc->ContextRecord->R13), static_cast<size_t>(exc->ContextRecord->R14),
						 static_cast<size_t>(exc->ContextRecord->R15));
		LOG_DYNAMIC_TEXT("RSP: " POINTER_FORMAT ", RIP: " POINTER_FORMAT "\n",
						 static_cast<size_t>(exc->ContextRecord->Rsp), static_cast<size_t>(exc->ContextRecord->Rip));
#elif LC_MACHINE == LC_MACHINE_X86
		LOG_STATIC_TEXT("\nProcessor registers (x86):\n");
		LOG_DYNAMIC_TEXT("EAX: " POINTER_FORMAT ", EBX: " POINTER_FORMAT ", ECX: " POINTER_FORMAT ", EDX: " POINTER_FORMAT "\n",
						 static_cast<size_t>(exc->ContextRecord->Eax), static_cast<size_t>(exc->ContextRecord->Ebx),
						 static_cast<size_t>(exc->ContextRecord->Ecx), static_cast<size_t>(exc->ContextRecord->Edx));
		LOG_DYNAMIC_TEXT("ESI: " POINTER_FORMAT ", EDI: " POINTER_FORMAT "\n",
						 static_cast<size_t>(exc->ContextRecord->Esi), static_cast<size_t>(exc->ContextRecord->Edi));
		LOG_DYNAMIC_TEXT("EBP: " POINTER_FORMAT ", ESP: " POINTER_FORMAT ", EIP: " POINTER_FORMAT "\n",
						 static_cast<size_t>(exc->ContextRecord->Ebp), static_cast<size_t>(exc->ContextRecord->Esp),
						 static_cast<size_t>(exc->ContextRecord->Eip));
#endif
#if LC_MACHINE == LC_MACHINE_X64 || LC_MACHINE == LC_MACHINE_X86
		LOG_DYNAMIC_TEXT("EFLAGS: 0x%08x (%c%c%c%c%c%c%c)\n", static_cast<unsigned int>(exc->ContextRecord->EFlags),
						 exc->ContextRecord->EFlags & 0x800 ? 'O' : '.',	// overflow
						 exc->ContextRecord->EFlags & 0x400 ? 'D' : '.',	// direction
						 exc->ContextRecord->EFlags & 0x80 ? 'S' : '.',	// sign
						 exc->ContextRecord->EFlags & 0x40 ? 'Z' : '.',	// zero
						 exc->ContextRecord->EFlags & 0x10 ? 'A' : '.',	// auxiliary carry
						 exc->ContextRecord->EFlags & 0x4 ? 'P' : '.',	// parity
						 exc->ContextRecord->EFlags & 0x1 ? 'C' : '.');	// carry
#endif

		// Dump stack
		LOG_STATIC_TEXT("\nStack contents:\n");
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
				LOG_DYNAMIC_TEXT(POINTER_FORMAT ": ", dumpRowBase);
				// Hex dump
				for (intptr_t dumpRowCursor = dumpRowBase; dumpRowCursor < dumpRowBase + 16; ++dumpRowCursor)
				{
					if (dumpRowCursor < dumpMin || dumpRowCursor > dumpMax)
						LOG_STATIC_TEXT("   ");
					else
						LOG_DYNAMIC_TEXT("%02x ", (unsigned int) *reinterpret_cast<unsigned char *>(dumpRowCursor)); // Safe, since it's inside the VM of our process
				}
				LOG_STATIC_TEXT("   ");
				// Text dump
				for (intptr_t dumpRowCursor = dumpRowBase; dumpRowCursor < dumpRowBase + 16; ++dumpRowCursor)
				{
					if (dumpRowCursor < dumpMin || dumpRowCursor > dumpMax)
						LOG_STATIC_TEXT(" ");
					else
					{
						unsigned char c = *reinterpret_cast<unsigned char *>(dumpRowCursor); // Safe, since it's inside the VM of our process
						if (c < 0x20 || (c > 0x7e && c < 0xa1))
							LOG_STATIC_TEXT(".");
						else
							LOG_DYNAMIC_TEXT("%c", static_cast<char>(c));
					}
				}
				LOG_STATIC_TEXT("\n");
			}
		}
		else
		{
			LOG_STATIC_TEXT("[Failed to access stack memory]\n");
		}

		// Initialize DbgHelp.dll symbol functions
		SymSetOptions(SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS | SYMOPT_LOAD_LINES);
		HANDLE process = GetCurrentProcess();
		if (SymInitialize(process, nullptr, true))
		{
			LOG_STATIC_TEXT("\nStack trace:\n");
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
			SYMBOL_INFO *symbol = reinterpret_cast<SYMBOL_INFO *>(SymbolBuffer);
			static_assert(DumpBufferSize >= sizeof(*symbol), "SYMBOL_INFO too large to fit into buffer");
			IMAGEHLP_MODULE64 *module = reinterpret_cast<IMAGEHLP_MODULE64 *>(SymbolBuffer);
			static_assert(DumpBufferSize >= sizeof(*module), "IMAGEHLP_MODULE64 too large to fit into buffer");
			IMAGEHLP_LINE64 *line = reinterpret_cast<IMAGEHLP_LINE64 *>(SymbolBuffer);
			static_assert(DumpBufferSize >= sizeof(*line), "IMAGEHLP_LINE64 too large to fit into buffer");
			int frameNumber = 0;
			while (StackWalk64(imageType, process, GetCurrentThread(), &frame, &context, nullptr, SymFunctionTableAccess64, SymGetModuleBase64, nullptr))
			{
				LOG_DYNAMIC_TEXT("#%3d ", frameNumber);
				module->SizeOfStruct = sizeof(*module);
				DWORD64 imageBase = 0;
				if (SymGetModuleInfo64(process, frame.AddrPC.Offset, module))
				{
					LOG_DYNAMIC_TEXT("%s", module->ModuleName);
					imageBase = module->BaseOfImage;
				}
				DWORD64 disp64;
				symbol->MaxNameLen = DumpBufferSize - sizeof(*symbol);
				symbol->SizeOfStruct = sizeof(*symbol);
				if (SymFromAddr(process, frame.AddrPC.Offset, &disp64, symbol))
				{
					LOG_DYNAMIC_TEXT("!%s+%#lx", symbol->Name, static_cast<long>(disp64));
				}
				else if (imageBase > 0)
				{
					LOG_DYNAMIC_TEXT("+%#lx", static_cast<long>(frame.AddrPC.Offset - imageBase));
				}
				else
				{
					LOG_DYNAMIC_TEXT("%#lx", static_cast<long>(frame.AddrPC.Offset));
				}
				DWORD disp;
				line->SizeOfStruct = sizeof(*line);
				if (SymGetLineFromAddr64(process, frame.AddrPC.Offset, &disp, line))
				{
					LOG_DYNAMIC_TEXT(" [%s @ %u]", line->FileName, static_cast<unsigned int>(line->LineNumber));
				}
				LOG_STATIC_TEXT("\n");
				++frameNumber;
			}
			SymCleanup(process);
		}
		else
		{
			LOG_STATIC_TEXT("[Stack trace not available: failed to initialize Debugging Help Library]\n");
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
			LOG_STATIC_TEXT("\nLoaded modules:\n");
			MODULEENTRY32 *module = reinterpret_cast<MODULEENTRY32 *>(SymbolBuffer);
			static_assert(DumpBufferSize >= sizeof(*module), "MODULEENTRY32 too large to fit into buffer");
			module->dwSize = sizeof(*module);
			for (BOOL success = Module32First(snapshot, module); success; success = Module32Next(snapshot, module))
			{
				LOG_DYNAMIC_TEXT("%32hs loaded at " POINTER_FORMAT " - " POINTER_FORMAT " (%hs)\n", module->szModule,
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

	MessageBoxA(nullptr, buffer, "LegacyClonk crashed", MB_ICONERROR);

	// Call native exception handler
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
#ifdef USE_WIDE_ASSERT
	typedef void(__cdecl *ASSERT_FUNC)(const wchar_t *, const wchar_t *, unsigned);
	const ASSERT_FUNC assertFunc =
		&_wassert;
#else
	typedef void(__cdecl *ASSERT_FUNC)(const char *, const char *, int);
	const ASSERT_FUNC assertFunc =
		&_assert;
#endif
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
#ifdef USE_WIDE_ASSERT
		const wchar_t
#else
		const char
#endif
			*expression, *file;
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
#ifdef USE_WIDE_ASSERT
	void __cdecl assertionHandler(const wchar_t *expression, const wchar_t *file, unsigned line)
#else
	void __cdecl assertionHandler(const char *expression, const char *file, int line)
#endif
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

#ifndef NDEBUG
	// Hook _wassert/_assert, unless we're running under a debugger
	if (!IsDebuggerPresent())
		HookAssert(&assertionHandler);
#endif
}
