/*
 * LegacyClonk
 *
 * Copyright (c) 2001, Matthes Bender(RedWolf Design GmbH)
 * Copyright (c) 2017-2022, The LegacyClonk Team and contributors
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

/* C4Group command line executable */

// Version    1.0 November  1997
//            1.1 November  1997
//            1.2 February  1998
//            1.3 March     1998
//            1.4 April     1998
//            1.5 May       1998
//            1.6 November  1998
//            1.7 December  1998
//            1.8 February  1999
//            1.9 May       1999
//            2.0 June      1999
//            2.6 March     2001
//            2.7 June      2001
//            2.8 June      2002
//         4.95.0 November  2003
//         4.95.4 July      2005 PORT/HEAD mixmax
//         4.95.4 September 2005 Unix-flavour

#include <C4Group.h>
#include <C4Version.h>
#include <C4Update.h>
#include <C4Config.h>

#ifdef _WIN32
#include "StdRegistry.h"

#include <shellapi.h>
#include <conio.h>

#define getch _getch
#else

#include <format>
#include <print>
#include <string_view>

#include <fmt/printf.h>

// from http://cboard.cprogramming.com/archive/index.php/t-27714.html
#include <stdio.h>
#include <termios.h>
#include <unistd.h>
int mygetch()
{
	struct termios oldt, newt;
	int ch;
	tcgetattr(STDIN_FILENO, &oldt);
	newt = oldt;
	newt.c_lflag &= ~(ICANON | ECHO);
	tcsetattr(STDIN_FILENO, TCSANOW, &newt);
	ch = getchar();
	tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
	return ch;
}

#define getch mygetch

#endif

int globalArgC;
char **globalArgV;
int iFirstCommand = 0;

bool fQuiet = true;
bool fRecursive = false;
bool fRegisterShell = false;
bool fUnregisterShell = false;
bool fPromptAtEnd = false;
char strExecuteAtEnd[_MAX_PATH + 1] = "";

int iResult = 0;

C4Config Config;
C4Config *GetCfg()
{
	return &Config;
}

bool Log(const std::string_view msg)
{
	if (!fQuiet)
		std::println("{}", msg);
	return 1;
}

template<typename... Args>
bool Log(const std::format_string<Args...> fmt, Args &&...args)
{
	return Log(std::format(fmt, std::forward<Args>(args)...));
}

bool ProcessGroup(const char *FilenamePar)
{
	C4Group hGroup;
	hGroup.SetStdOutput(!fQuiet);
	bool fDeleteGroup = false;

	int argc = globalArgC;
	char **argv = globalArgV;

	// Strip trailing slash
	char *szFilename = strdup(FilenamePar);
	size_t len = strlen(szFilename);
	if (szFilename[len - 1] == DirectorySeparator) szFilename[len - 1] = 0;
	// Current filename
	Log("Group: {}", szFilename);

	// Open group file
	if (hGroup.Open(szFilename, true))
	{
		// No commands: display contents
		if (iFirstCommand >= argc)
		{
			hGroup.SetStdOutput(true);
			hGroup.View("*");
			hGroup.SetStdOutput(!fQuiet);
		}
		// Process commands
		else
		{
			for (int iArg = iFirstCommand; iArg < argc; ++iArg)
			{
				// This argument is a command
				if (argv[iArg][0] == '-')
				{
					// Handle commands
					switch (argv[iArg][1])
					{
					// Add
					case 'a':
						if ((iArg + 1 >= argc) || (argv[iArg + 1][0] == '-'))
							std::println(stderr, "Missing argument for add command");
						else
						{
							if ((argv[iArg][2] == 's') || (argv[iArg][2] && (argv[iArg][3] == 's')))
							{
								if ((iArg + 2 >= argc) || (argv[iArg + 2][0] == '-'))
								{
									std::println(stderr, "Missing argument for add as command");
								}
								else
								{
									hGroup.Add(argv[iArg + 1], argv[iArg + 2]);
									iArg += 2;
								}
							}
							else
							{
								while (iArg + 1 < argc && argv[iArg + 1][0] != '-')
								{
									++iArg;
#ifdef _WIN32
									// manually expand wildcards
									hGroup.Add(argv[iArg]);
#else
									hGroup.Add(argv[iArg], GetFilename(argv[iArg]));
#endif
								}
							}
						}
						break;
					// Move
					case 'm':
						if ((iArg + 1 >= argc) || (argv[iArg + 1][0] == '-'))
						{
							std::println(stderr, "Missing argument for move command");
						}
						else
						{
							while (iArg + 1 < argc && argv[iArg + 1][0] != '-')
							{
								++iArg;
#ifdef _WIN32
								// manually expand wildcards
								hGroup.Move(argv[iArg]);
#else
								hGroup.Move(argv[iArg], argv[iArg]);
#endif
							}
						}
						break;
					// Extract
					case 'e':
						if ((iArg + 1 >= argc) || (argv[iArg + 1][0] == '-'))
						{
							std::println(stderr, "Missing argument for extract command");
						}
						else
						{
							if ((argv[iArg][2] == 't') || (argv[iArg][2] && (argv[iArg][3] == 's')))
							{
								if ((iArg + 2 >= argc) || (argv[iArg + 2][0] == '-'))
								{
									std::println(stderr, "Missing argument for extract as command");
								}
								else
								{
									hGroup.Extract(argv[iArg + 1], argv[iArg + 2]);
									iArg += 2;
								}
							}
							else
							{
								hGroup.Extract(argv[iArg + 1]);
								iArg++;
							}
						}
						break;
					// Delete
					case 'd':
						if ((iArg + 1 >= argc) || (argv[iArg + 1][0] == '-'))
						{
							std::println(stderr, "Missing argument for delete command");
						}
						else
						{
							hGroup.Delete(argv[iArg + 1], fRecursive);
							iArg++;
						}
						break;
					// Sort
					case 's':
						// First sort parameter overrides default Clonk sort list
						C4Group_SetSortList(nullptr);
						// Missing argument
						if ((iArg + 1 >= argc) || (argv[iArg + 1][0] == '-'))
						{
							std::println(stderr, "Missing argument for sort command");
						}
						// Sort, advance to next argument
						else
						{
							hGroup.Sort(argv[iArg + 1]);
							hGroup.Save(true);
							iArg++;
						}
						break;
					// Rename
					case 'r':
						if ((iArg + 2 >= argc) || (argv[iArg + 1][0] == '-')
							|| (argv[iArg + 2][0] == '-'))
						{
							std::println(stderr, "Missing argument(s) for rename command");
						}
						else
						{
							hGroup.Rename(argv[iArg + 1], argv[iArg + 2]);
							iArg += 2;
						}
						break;
					// View
					case 'l':
					case 'v':
						hGroup.SetStdOutput(true);
						if ((iArg + 1 >= argc) || (argv[iArg + 1][0] == '-'))
						{
							hGroup.View("*");
						}
						else
						{
							hGroup.View(argv[iArg + 1]);
							iArg++;
						}
						hGroup.SetStdOutput(!fQuiet);
						break;
					// Make original
					case 'o':
						hGroup.MakeOriginal(true);
						break;
					// Pack
					case 'p':
						std::println("Packing...");
						// Close
						if (!hGroup.Close())
						{
							std::println(stderr, "Closing failed: {}", hGroup.GetError());
						}
						// Pack
						else if (!C4Group_PackDirectory(szFilename))
						{
							std::println(stderr, "Pack failed");
						}
						// Reopen
						else if (!hGroup.Open(szFilename))
						{
							std::println(stderr, "Reopen failed: {}", hGroup.GetError());
						}
						break;
					// Unpack
					case 'u':
						Log("Unpacking...");
						// Close
						if (!hGroup.Close())
						{
							std::println(stderr, "Closing failed: {}", hGroup.GetError());
						}
						// Pack
						else if (!C4Group_UnpackDirectory(szFilename))
						{
							std::println(stderr, "Unpack failed");
						}
						// Reopen
						else if (!hGroup.Open(szFilename))
						{
							std::println(stderr, "Reopen failed: {}", hGroup.GetError());
						}
						break;
					// Unpack
					case 'x':
						std::println("Exploding...");
						// Close
						if (!hGroup.Close())
						{
							std::println(stderr, "Closing failed: {}", hGroup.GetError());
						}
						// Pack
						else if (!C4Group_ExplodeDirectory(szFilename))
						{
							std::println(stderr, "Unpack failed");
						}
						// Reopen
						else if (!hGroup.Open(szFilename))
						{
							std::println(stderr, "Reopen failed: {}", hGroup.GetError());
						}
						break;
					// Print maker
					case 'k':
						std::println("{}", hGroup.GetMaker());
						break;
					// Generate update
					case 'g':
						if ((iArg + 3 >= argc) || (argv[iArg + 1][0] == '-')
							|| (argv[iArg + 2][0] == '-')
							|| (argv[iArg + 3][0] == '-'))
						{
							std::println(stderr, "Update generation failed: too few arguments");
						}
						else
						{
							C4UpdatePackage Upd;
							// Close
							if (!hGroup.Close())
							{
								std::println(stderr, "Closing failed: {}", hGroup.GetError());
							}
							// generate
							else if (!Upd.MakeUpdate(argv[iArg + 1], argv[iArg + 2], szFilename, argv[iArg + 3], argv[iArg][2] == 'a'))
							{
								std::println(stderr, "Update generation failed.");
							}
							// Reopen
							else if (!hGroup.Open(szFilename))
							{
								std::println(stderr, "Reopen failed: {}", hGroup.GetError());
							}
							iArg += 3;
						}
						break;

					// Apply an update
					case 'y':
						std::println("Applying update...");
						if (C4Group_ApplyUpdate(hGroup))
						{
							if (argv[iArg][2] == 'd') fDeleteGroup = true;
						}
						else
							std::println(stderr, "Update failed.");
						break;
#ifdef _DEBUG
					case 'z':
						hGroup.PrintInternals();
						break;
#endif

#ifdef _WIN32
					// Wait
					case 'w':
						if (iArg + 1 >= argc || argv[iArg + 1][0] == '-')
						{
							std::println("Missing argument for wait command");
						}
						else
						{
							int milliseconds{0};
							sscanf(argv[iArg + 1], "%d", &milliseconds);

							if (milliseconds > 0)
							{
								// Wait for specified time
								std::println("Waiting..");
								Sleep(milliseconds);
							}
							else
							{
								// Wait for specified process to end
								std::println("Waiting for {} to end", argv[iArg + 1]);

								for (std::size_t i{0}; i < 5 && FindWindowA(nullptr, argv[iArg + 1]); ++i)
								{
									Sleep(1000);
									std::print(".");
								}

								std::println("");
							}

							++iArg;
						}
						break;
#endif
					// Undefined
					default:
						std::println(stderr, "Unknown command: {}", argv[iArg]);
						break;
					}
				}
				else
				{
					std::println(stderr, "Invalid parameter {}", argv[iArg]);
				}
			}
		}
		// Error: output status
		if (!SEqual(hGroup.GetError(), "No Error"))
		{
			std::println(stderr, "Status: {}", hGroup.GetError());
		}
		// Close group file
		if (!hGroup.Close())
		{
			std::println(stderr, "Closing: {}", hGroup.GetError());
		}
		// Delete group file if desired (i.e. after apply update)
		if (fDeleteGroup)
		{
			Log("Deleting {}...", GetFilename(szFilename));
			EraseItem(szFilename);
		}
	}
	// Couldn't open group
	else
	{
		std::println(stderr, "Status: {}", hGroup.GetError());
	}
	free(szFilename);
	// Done
	return true;
}

int RegisterShellExtensions()
{
#ifdef _WIN32
	char strModule[2048];
	char strCommand[2048];
	char strClass[128];
	GetModuleFileNameA(nullptr, strModule, 2048);
	// Groups
	const char *strClasses = "Clonk4.Definition;Clonk4.Folder;Clonk4.Group;Clonk4.Player;Clonk4.Scenario;Clonk4.Update;Clonk4.Weblink;Clonk4.Object";
	for (int i = 0; SCopySegment(strClasses, i, strClass); i++)
	{
		// Unpack
		FormatWithNull(strCommand, "\"{}\" \"%1\" \"-u\"", strModule);
		if (!SetRegShell(strClass, "MakeFolder", "C4Group Unpack", strCommand))
			return 0;
		// Explode
		FormatWithNull(strCommand, "\"{}\" \"%1\" \"-x\"", strModule);
		if (!SetRegShell(strClass, "ExplodeFolder", "C4Group Explode", strCommand))
			return 0;
	}
	// Directories
	const char *strClasses2 = "Directory";
	for (int i = 0; SCopySegment(strClasses2, i, strClass); i++)
	{
		// Pack
		FormatWithNull(strCommand, "\"{}\" \"%1\" \"-p\"", strModule);
		if (!SetRegShell(strClass, "MakeGroupFile", "C4Group Pack", strCommand))
			return 0;
	}
	// Done
#endif
	return 1;
}

int UnregisterShellExtensions()
{
#ifdef _WIN32
	char strClass[128];
	// Groups
	const char *strClasses = "Clonk4.Definition;Clonk4.Folder;Clonk4.Group;Clonk4.Player;Clonk4.Scenario;Clonk4.Update;Clonk4.Weblink";
	for (int i = 0; SCopySegment(strClasses, i, strClass); i++)
	{
		// Unpack
		if (!RemoveRegShell(strClass, "MakeFolder")) return 0;
		// Explode
		if (!RemoveRegShell(strClass, "ExplodeFolder")) return 0;
	}
	// Directories
	const char *strClasses2 = "Directory";
	for (int i = 0; SCopySegment(strClasses2, i, strClass); i++)
	{
		// Pack
		if (!RemoveRegShell(strClass, "MakeGroupFile")) return 0;
	}
	// Done
#endif
	return 1;
}

int main(int argc, char *argv[])
{
#ifndef _WIN32
	// Always line buffer mode, even if the output is not sent to a terminal
	setvbuf(stdout, nullptr, _IOLBF, 0);
#endif
	// Scan options
	int iFirstGroup = 0;
	for (int i = 1; i < argc; ++i)
	{
		// Option encountered
#ifdef _WIN32
		if (argv[i][0] == '/' || argv[i][0] == '-')
#else
		if (argv[i][0] == '-')
#endif
		{
			switch (argv[i][1])
			{
			// Quiet mode
			case 'q':
				fQuiet = true;
				break;
			// Verbose mode
			case 'v':
				fQuiet = false;
				break;
			// Recursive mode
			case 'r':
				fRecursive = true;
				break;
			// Register shell
			case 'i':
				fRegisterShell = true;
				break;
			// Unregister shell
			case 'u':
				fUnregisterShell = true;
				break;
			// Prompt at end
			case 'p': fPromptAtEnd = true; break;
			// Execute at end
			case 'x': SCopy(argv[i] + 3, strExecuteAtEnd, _MAX_PATH); break;
			// Unknown
			default:
				std::println(stderr, "Unknown option {}", argv[i]);
				break;
			}
		}
		else
		{
			// filename encountered: no more options expected
			iFirstGroup = i;
			break;
		}
	}
	iFirstCommand = iFirstGroup;
	while (iFirstCommand < argc && argv[iFirstCommand][0] != '-')
		++iFirstCommand;

	// Program info
	Log("LegacyClonk C4Group {}", C4VERSION);

	// Load configuration
	Config.Init();
	Config.Load(false);

	// Init C4Group
	C4Group_SetMaker(Config.General.Name);
	C4Group_SetTempPath(Config.General.TempPath);
	C4Group_SetSortList(C4CFN_FLS);

	// Display current working directory
	if (!fQuiet)
	{
		std::println("Location: {}", GetWorkingDirectory());
	}

	// Store command line parameters
	globalArgC = argc;
	globalArgV = argv;

	// Register shell
	if (fRegisterShell)
		if (RegisterShellExtensions())
			std::println("Shell extensions registered.");
		else
			std::println("Error registering shell extensions.");
	// Unregister shell
	if (fUnregisterShell)
		if (UnregisterShellExtensions())
			std::println("Shell extensions removed.");
		else
			std::println("Error removing shell extensions.");

	// At least one parameter (filename, not option or command): process file(s)
	if (iFirstGroup)
	{
#ifdef _WIN32
		// Wildcard in filename: use file search
		if (SCharCount('*', argv[iFirstGroup]))
			ForEachFile(argv[iFirstGroup], &ProcessGroup);
		// Only one file
		else
			ProcessGroup(argv[iFirstGroup]);
#else
		for (int i = iFirstGroup; i < argc && argv[i][0] != '-'; ++i)
			ProcessGroup(argv[i]);
#endif
	}
	// Too few parameters: output help (if we didn't register stuff)
	else if (!fRegisterShell && !fUnregisterShell)
	{
		std::println("");
		std::println("Usage:    c4group [options] group(s) command(s)\n");
		std::println("Commands: -a[s] Add [as]  -m Move  -e[t] Extract [to]");
		std::println("          -v View  -l List  -d Delete  -r Rename  -s Sort");
		std::println("          -p Pack  -u Unpack  -x Explode");
		std::println("          -k Print maker");
		std::println("          -g[a] [source] [target] [title] Make update [allow missing target]");
		std::println("          -y[d] Apply update [and delete group file]");
		std::println("");
		std::println("Options:  -v Verbose -r Recursive -p Prompt at end");
		std::println("          -i Register shell -u Unregister shell");
		std::println("          -x:<command> Execute shell command when done");
		std::println("");
		std::println("Examples: c4group pack.c4g -a myfile.dat -l \"*.dat\"");
		std::println("          c4group pack.c4g -as myfile.dat myfile.bin");
		std::println("          c4group -v pack.c4g -et \"*.dat\" \\data\\mydatfiles\\");
		std::println("          c4group pack.c4g -et myfile.dat myfile.bak");
		std::println("          c4group pack.c4g -s \"*.bin|*.dat\"");
		std::println("          c4group pack.c4g -x");
		std::println("          c4group pack.c4g -k");
		std::println("          c4group update.c4u -g ver1.c4f ver2.c4f New_Version");
		std::println("          c4group -i");
	}

	// Prompt at end
	if (fPromptAtEnd)
	{
		std::println("\nDone. Press any key to continue.");
		getch();
	}

	// Execute when done
	if (strExecuteAtEnd[0])
	{
		std::println("Executing: {}", strExecuteAtEnd);

#ifdef _WIN32
		STARTUPINFOA startInfo{};
		startInfo.cb = sizeof(startInfo);

		PROCESS_INFORMATION procInfo;

		CreateProcessA(strExecuteAtEnd, nullptr, nullptr, nullptr, false, 0, nullptr, nullptr, &startInfo, &procInfo);
#else
		switch (fork())
		{
		// Error
		case -1:
			std::println(stderr, "Error forking.");
			break;
		// Child process
		case 0:
			execl(strExecuteAtEnd, strExecuteAtEnd, static_cast<char *>(0)); // currently no parameters are passed to the executed program
			exit(1);
		// Parent process
		default:
			break;
		}
#endif
	}

	// Done
	return iResult;
}
