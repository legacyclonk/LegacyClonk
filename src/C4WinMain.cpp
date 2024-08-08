/*
 * LegacyClonk
 *
 * Copyright (c) 1998-2000, Matthes Bender (RedWolf Design)
 * Copyright (c) 2017-2021, The LegacyClonk Team and contributors
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

/* Main program entry point */

#include <C4Include.h>
#include <C4Application.h>

#include <C4Console.h>
#include <C4FullScreen.h>
#include <C4Log.h>

#ifdef WITH_DEVELOPER_MODE
#include <gtk/gtk.h>
#endif

// debug memory management
#if !defined(NODEBUGMEM) && defined(_MSC_VER)
#include <crtdbg.h>
#endif

#ifdef _WIN32
#include "C4Com.h"
#include "C4WinRT.h"

#include <span>
#include <string_view>

#include <objbase.h>
#include <tchar.h>
#endif

#ifdef __APPLE__
#include "MacAppTranslocation.h"
#include <libgen.h>
#endif

C4Application Application;
C4Console Console;
C4FullScreen FullScreen;
C4Game Game;
C4Config Config;

#ifdef _WIN32

void InstallCrashHandler();

int ClonkMain(const HINSTANCE instance, const int cmdShow, const int argc, char **const argv, const LPSTR commandLine)
{
#if defined(_MSC_VER)
	// enable debugheap!
	_CrtSetDbgFlag(_CrtSetDbgFlag(_CRTDBG_REPORT_FLAG) | _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

	SetCurrentProcessExplicitAppUserModelID(_CRT_WIDE(STD_APPUSERMODELID));

	InstallCrashHandler();

#ifndef USE_CONSOLE
#ifdef NDEBUG
	const std::span args{argv, static_cast<std::size_t>(argc)};

	const auto hasArgument = [&args](const std::string_view argument)
	{
		return std::ranges::find(args, argument) != std::ranges::end(args);
	};

	if (hasArgument("/allocconsole"))
#endif
	{
		if (!AllocConsole())
		{
			return C4XRV_Failure;
		}

		freopen("CONIN$", "r", stdin);
		freopen("CONOUT$", "w", stdout);
		freopen("CONOUT$", "w", stderr);
	}
#endif

	C4Com com;

	try
	{
		com = C4Com{winrt::apartment_type::multi_threaded};
	}
	catch (const winrt::hresult_error &e)
	{
		MessageBoxW(nullptr, (std::wstring{L"Failed to initialize COM: "} + e.message()).c_str(), _CRT_WIDE(STD_PRODUCT), MB_ICONERROR);
		return C4XRV_Failure;
	}

	// Init application
	try
	{
		Application.Init(instance, cmdShow, commandLine);
	}
	catch (const CStdApp::StartupException &e)
	{
		Application.Clear();
		MessageBoxA(nullptr, e.what(), STD_PRODUCT, MB_ICONERROR);
		return C4XRV_Failure;
	}

	// Run it
	Application.Run();
	Application.Clear();

	// Return exit code
	return C4XRV_Completed;
}

int WINAPI WinMain(HINSTANCE hInst,
	HINSTANCE hPrevInstance,
	LPSTR lpszCmdParam,
	int nCmdShow)
{
	return ClonkMain(hInst, nCmdShow, __argc, __argv, lpszCmdParam);
}

int main(const int argc, char **const argv)
{
	// Get command line, go over program name
	char *commandLine{GetCommandLineA()};
	if (*commandLine == L'"')
	{
		++commandLine;
		while (*commandLine && *commandLine != '"')
		{
			++commandLine;
		}

		if (*commandLine == '"')
		{
			++commandLine;
		}
	}
	else
	{
		while (*commandLine && *commandLine != ' ')
		{
			++commandLine;
		}
	}
	while (*commandLine == ' ')
	{
		++commandLine;
	}

	return ClonkMain(GetModuleHandle(nullptr), 0, argc, argv, commandLine);
}

#else

#include <unistd.h>
#include <fcntl.h>
#include <signal.h>

#ifndef _WIN32

#include <execinfo.h>

#define HAVE_EXECINFO_H

static void crash_handler(int signo)
{
	int logfd = STDERR_FILENO;
	for (;;)
	{
		// Print out the signal
		write(logfd, C4VERSION ": Caught signal ", sizeof(C4VERSION ": Caught signal ") - 1);
		switch (signo)
		{
		case SIGBUS:  write(logfd, "SIGBUS",  sizeof("SIGBUS")  - 1); break;
		case SIGILL:  write(logfd, "SIGILL",  sizeof("SIGILL")  - 1); break;
		case SIGSEGV: write(logfd, "SIGSEGV", sizeof("SIGSEGV") - 1); break;
		case SIGABRT: write(logfd, "SIGABRT", sizeof("SIGABRT") - 1); break;
		case SIGINT:  write(logfd, "SIGINT",  sizeof("SIGINT")  - 1); break;
		case SIGQUIT: write(logfd, "SIGQUIT", sizeof("SIGQUIT") - 1); break;
		case SIGFPE:  write(logfd, "SIGFPE",  sizeof("SIGFPE")  - 1); break;
		case SIGTERM: write(logfd, "SIGTERM", sizeof("SIGTERM") - 1); break;
		}
		write(logfd, "\n", sizeof("\n") - 1);
		if (logfd == STDERR_FILENO) logfd = GetLogFD();
		else break;
		if (logfd < 0) break;
	}
	// Get the backtrace
	void *stack[100];
	int count = backtrace(stack, 100);
	// Print it out
	backtrace_symbols_fd(stack, count, STDERR_FILENO);
	// Also to the log file
	if (logfd >= 0)
		backtrace_symbols_fd(stack, count, logfd);

	signal(signo, SIG_DFL);
	raise(signo);
}

#endif

#ifdef __APPLE__
void restart(char *[]); // MacUtility.mm
#else
static void restart(char *argv[])
{
	// Close all file descriptors except stdin, stdout, stderr
	int open_max = sysconf(_SC_OPEN_MAX);
	for (int fd = 4; fd < open_max; fd++)
		fcntl(fd, F_SETFD, FD_CLOEXEC);
	// Execute the new engine
	execlp(argv[0], argv[0], static_cast<char *>(0));
}
#endif

int main(int argc, char *argv[])
{
#ifdef __APPLE__
	std::string enginePath{argv[0]};
	if (const auto originalPath = GetNonTranslocatedPath(argv[0]); originalPath)
	{
		enginePath = *originalPath;
	}
	chdir(dirname(dirname(dirname(dirname(enginePath.data())))));
#elif defined(__linux__)
	if (std::getenv("APPIMAGE"))
	{
		const auto argv0 = std::getenv("ARGV0");
		if (argv0)
		{
			argv[0] = argv0;
		}
	}
#endif

	if (!geteuid())
	{
		printf("Do not run %s as root!\n", argc ? argv[0] : "this program");
		return C4XRV_Failure;
	}
#ifdef HAVE_EXECINFO_H
	// Set up debugging facilities
	signal(SIGBUS,  crash_handler);
	signal(SIGILL,  crash_handler);
	signal(SIGSEGV, crash_handler);
	signal(SIGABRT, crash_handler);
	signal(SIGINT,  crash_handler);
	signal(SIGQUIT, crash_handler);
	signal(SIGFPE,  crash_handler);
	signal(SIGTERM, crash_handler);
#endif

	// FIXME: This should only be done in developer mode.
#ifdef WITH_DEVELOPER_MODE
	gdk_set_allowed_backends("x11");
	gtk_init(&argc, &argv);
#endif

	// Init application
	try
	{
		Application.Init(argc, argv);
	}
	catch (const CStdApp::StartupException &e)
	{
		Application.Clear();
		fprintf(stderr, "%s\n", e.what());

#ifdef WITH_DEVELOPER_MODE
		GtkWidget *const dialog{gtk_message_dialog_new(nullptr, GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, "%s", e.what())};
		gtk_dialog_run(GTK_DIALOG(dialog));
		gtk_widget_destroy(dialog);
#endif
		return C4XRV_Failure;
	}

	// Execute application
	Application.Run();
	// free app stuff
	Application.Clear();
	if (Application.restartAtEnd) restart(argv);
	// Return exit code
	return C4XRV_Completed;
}

#endif
