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

// Version    1.0 November 1997
//            1.1 November 1997
//            1.2 February 1998
//            1.3 March    1998
//            1.4 April    1998
//            1.5 May      1998
//            1.6 November 1998
//            1.7 December 1998
//            1.8 February 1999
//            1.9 May      1999
//            2.0 June     1999
//            2.6 March    2001
//            2.7 June     2001
//            2.8 June     2002
//         4.95.0 November 2003
//         4.95.4 July     2005 PORT/HEAD mixmax

#include <C4Config.h>
#include <StdRegistry.h>
#include <C4Group.h>
#include <C4Version.h>
#include <C4Update.h>

#include <format>
#include <print>
#include <string_view>

#include <fmt/printf.h>

#include <shellapi.h>
#include <conio.h>

int globalArgC;
char **globalArgV;
int iFirstCommand = -1;

bool fQuiet = false;
bool fRecursive = false;
bool fRegisterShell = false;
bool fUnregisterShell = false;
bool fPromptAtEnd = false;
char strExecuteAtEnd[_MAX_PATH + 1] = "";

int iResult = 0;

C4Config Config;
C4Config *GetCfg() { return &Config; }

#ifdef _WIN32
#ifndef NDEBUG
template<typename... Args>
void dbg_printf(const std::format_string<Args...> fmt, Args &&... args)
{
	// Compose formatted message
	const std::string output{std::format(fmt, std::forward<Args>(args)...)};
	// Log
	OutputDebugString(output.c_str());
	std::print("{}", output);
}
#define printf dbg_printf
#endif
#endif

bool ProcessGroup(const char *szFilename)
{
	C4Group hGroup;
	int iArg;
	bool fDeleteGroup = false;
	hGroup.SetStdOutput(true);

	int argc = globalArgC;
	char **argv = globalArgV;

	// Current filename
	if (!fQuiet)
		std::println("Group: {}", szFilename);

	// Open group file
	if (hGroup.Open(szFilename, true))
	{
		// No commands: display contents
		if (iFirstCommand < 0)
		{
			hGroup.View("*");
		}

		// Process commands
		else
			for (iArg = iFirstCommand; iArg < argc; iArg++)
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
							std::println("Missing argument for add command");
						else
						{
							if ((argv[iArg][2] == 's') || (argv[iArg][2] && (argv[iArg][3] == 's')))
							{
								if ((iArg + 2 >= argc) || (argv[iArg + 2][0] == '-'))
									std::println("Missing argument for add as command");
								else
								{
									hGroup.Add(argv[iArg + 1], argv[iArg + 2]); iArg += 2;
								}
							}
							else
#ifdef _WIN32
							{
								hGroup.Add(argv[iArg + 1]); iArg++;
							}
#else
							{ hGroup.Add(argv[iArg + 1], argv[iArg + 1]); iArg++; }
#endif
						}
						break;
					// Move
					case 'm':
						if ((iArg + 1 >= argc) || (argv[iArg + 1][0] == '-'))
							std::println("Missing argument for move command");
						else
#ifdef _WIN32
						{
							hGroup.Move(argv[iArg + 1]); iArg++;
						}
#else
						{ hGroup.Move(argv[iArg + 1], argv[iArg + 1]); iArg++; }
#endif
						break;
					// Extract
					case 'e':
						if ((iArg + 1 >= argc) || (argv[iArg + 1][0] == '-'))
							std::println("Missing argument for extract command");
						else
						{
							if ((argv[iArg][2] == 't') || (argv[iArg][2] && (argv[iArg][3] == 's')))
							{
								if ((iArg + 2 >= argc) || (argv[iArg + 2][0] == '-'))
									std::println("Missing argument for extract as command");
								else
								{
									hGroup.Extract(argv[iArg + 1], argv[iArg + 2]); iArg += 2;
								}
							}
							else
							{
								hGroup.Extract(argv[iArg + 1]); iArg++;
							}
						}
						break;
					// Delete
					case 'd':
						if ((iArg + 1 >= argc) || (argv[iArg + 1][0] == '-'))
							std::println("Missing argument for delete command");
						else
						{
							hGroup.Delete(argv[iArg + 1], fRecursive); iArg++;
						}
						break;
					// Sort
					case 's':
						// First sort parameter overrides default Clonk sort list
						C4Group_SetSortList(nullptr);
						// Missing argument
						if ((iArg + 1 >= argc) || (argv[iArg + 1][0] == '-'))
							std::println("Missing argument for sort command");
						// Sort, advance to next argument
						else
						{
							hGroup.Sort(argv[iArg + 1]); iArg++;
							hGroup.Save(true);
						}
						break;
					// Rename
					case 'r':
						if ((iArg + 2 >= argc) || (argv[iArg + 1][0] == '-') || (argv[iArg + 2][0] == '-'))
							std::println("Missing argument(s) for rename command");
						else
						{
							hGroup.Rename(argv[iArg + 1], argv[iArg + 2]); iArg += 2;
						}
						break;
					// View
					case 'v':
						if ((iArg + 1 >= argc) || (argv[iArg + 1][0] == '-'))
						{
							hGroup.View("*");
						}
						else
						{
							hGroup.View(argv[iArg + 1]); iArg++;
						}
						break;
					// Make original
					case 'o':
						hGroup.MakeOriginal(true);
						break;
					// Pack
					case 'p':
						std::println("Packing...");
						// Close
						if (!hGroup.Close()) std::println("Closing failed: {}", hGroup.GetError());
						// Pack
						else if (!C4Group_PackDirectory(szFilename)) std::println("Pack failed");
						// Reopen
						else if (!hGroup.Open(szFilename)) std::println("Reopen failed: {}", hGroup.GetError());
						break;
					// Unpack
					case 'u':
						std::println("Unpacking...");
						// Close
						if (!hGroup.Close()) std::println("Closing failed: {}", hGroup.GetError());
						// Unpack
						else if (!C4Group_UnpackDirectory(szFilename)) std::println("Unpack failed");
						// Reopen
						else if (!hGroup.Open(szFilename)) std::println("Reopen failed: {}\n", hGroup.GetError());
						break;
					// Unpack
					case 'x':
						std::println("Exploding...");
						// Close
						if (!hGroup.Close()) std::println("Closing failed: {}\n", hGroup.GetError());
						// Explode
						else if (!C4Group_ExplodeDirectory(szFilename)) std::println("Unpack failed");
						// Reopen
						else if (!hGroup.Open(szFilename)) std::println("Reopen failed: {}", hGroup.GetError());
						break;
					// Print maker
					case 'k':
						std::println("{}", hGroup.GetMaker());
						break;
					// Generate update
					case 'g':
						if ((iArg + 3 >= argc) || (argv[iArg + 1][0] == '-') || (argv[iArg + 2][0] == '-') || (argv[iArg + 3][0] == '-'))
							std::println("Update generation failed: too few arguments");
						else
						{
							C4UpdatePackage Upd;
							// Close
							if (!hGroup.Close()) std::println("Closing failed: {}", hGroup.GetError());
							// generate
							else if (!Upd.MakeUpdate(argv[iArg + 1], argv[iArg + 2], szFilename, argv[iArg + 3]))
								std::println("Update generation failed.");
							// Reopen
							else if (!hGroup.Open(szFilename)) std::println("Reopen failed: {}", hGroup.GetError());
							iArg += 3;
						}
						break;
					// Apply update
					case 'y':
						std::println("Applying update...");
						if (C4Group_ApplyUpdate(hGroup))
						{
							if (argv[iArg][2] == 'd') fDeleteGroup = true;
						}
						else
							std::println("Update failed.");
						break;
					// Optimize update generation target
					case 'z':
						if ((iArg + 1 >= argc) || (argv[iArg + 1][0] == '-'))
							std::println("Missing parameter for optimization");
						else
						{
							std::println("Optimizing {}...", argv[iArg + 1]);
							if (!C4UpdatePackage::Optimize(&hGroup, argv[iArg + 1]))
								std::println("Optimization failed.");
							iArg++;
						}
						break;
#ifndef NDEBUG
					// Print internals
					case 'q':
						hGroup.PrintInternals();
						break;
#endif
					// Wait
					case 'w':
						if ((iArg + 1 >= argc) || (argv[iArg + 1][0] == '-'))
							std::println("Missing argument for wait command");
						else
						{
							int iMilliseconds = 0;
							sscanf(argv[iArg + 1], "%d", &iMilliseconds);
							// Wait for specified time
							if (iMilliseconds > 0)
							{
								std::println("Waiting...");
								Sleep(iMilliseconds);
							}
							// Wait for specified process to end
							else
							{
								std::print("Waiting for {} to end", argv[iArg + 1]);
								for (int i = 0; FindWindowA(nullptr, argv[iArg + 1]) && (i < 5); i++)
								{
									Sleep(1000);
									std::print(".");
								}
								std::println("");
							}
							iArg++;
						}
						break;
					// Undefined
					default:
						std::println("Unknown command: {}", argv[iArg]);
						break;
					}
				}
				else
				{
					std::println("Invalid parameter {}", argv[iArg]);
				}
			}

		// Error: output status
		if (!SEqual(hGroup.GetError(), "No Error"))
			std::println("Status: {}", hGroup.GetError());

		// Close group file
		if (!hGroup.Close())
			std::println("Closing: {}", hGroup.GetError());

		// Delete group file if desired (i.e. after apply update)
		if (fDeleteGroup)
		{
			std::println("Deleting {}...", GetFilename(szFilename));
			EraseItem(szFilename);
		}
	}

	// Couldn't open group
	else
	{
		std::println("Status: {}", hGroup.GetError());
	}

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
		if (!SetRegShell(strClass, "MakeFolder", "C4Group Unpack", strCommand)) return 0;
		// Explode
		FormatWithNull(strCommand, "\"{}\" \"%1\" \"-x\"", strModule);
		if (!SetRegShell(strClass, "ExplodeFolder", "C4Group Explode", strCommand)) return 0;
	}
	// Directories
	const char *strClasses2 = "Directory";
	for (int i = 0; SCopySegment(strClasses2, i, strClass); i++)
	{
		// Pack
		FormatWithNull(strCommand, "\"{}\" \"%1\" \"-p\"", strModule);
		if (!SetRegShell(strClass, "MakeGroupFile", "C4Group Pack", strCommand)) return 0;
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

bool Log(const std::string_view msg)
{
	if (!fQuiet)
		std::println("{}", msg);
	return 1;
}

bool LogFatal(const char *msg) { return Log(msg); }

template<typename... Args>
bool LogF(const char *strMessage, Args... args)
{
	return Log(fmt::sprintf(strMessage, args...));
}

void StdCompilerWarnCallback(void *pData, const char *szPosition, const char *szError)
{
	const char *szName = reinterpret_cast<const char *>(pData);
	if (!szPosition || !*szPosition)
		LogF("WARNING: {}: {}", szName, szError);
	else
		LogF("WARNING: {} ({}): {}", szName, szPosition, szError);
}

int main(int argc, char *argv[])
{
	// Scan options (scan including first parameter - this means the group filename cannot start with a '/'...)
	for (int i = 1; i < argc; i++)
	{
		// Option encountered
		if (argv[i][0] == '/')
			switch (argv[i][1])
			{
			// Quiet mode
			case 'q': fQuiet = true; break;
			// Recursive mode
			case 'r': fRecursive = true; break;
			// Register shell
			case 'i': fRegisterShell = true; break;
			// Unregister shell
			case 'u': fUnregisterShell = true; break;
			// Prompt at end
			case 'p': fPromptAtEnd = true; break;
			// Execute at end
			case 'x': SCopy(argv[i] + 3, strExecuteAtEnd, _MAX_PATH); break;
			// Unknown
			default: std::println("Unknown option {}", argv[i]); break;
			}
		// Command encountered: no more options expected
		if (argv[i][0] == '-')
		{
			iFirstCommand = i;
			break;
		}
	}

	// Program info
	if (!fQuiet)
		std::println("LegacyClonk C4Group {}", C4VERSION);

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
		char strWorkingDirectory[_MAX_PATH + 1] = "";
		GetCurrentDirectoryA(_MAX_PATH, strWorkingDirectory);
		std::println("Location: {}", strWorkingDirectory);
	}

	// Store command line parameters
	globalArgC = argc;
	globalArgV = argv;

	// Register shell
	if (fRegisterShell)
		if (RegisterShellExtensions())
			std::println("Shell extensions registered.");
		else
			std::println("Error registering shell extensions. You might need administrator rights.");
	// Unregister shell
	if (fUnregisterShell)
		if (UnregisterShellExtensions())
			std::println("Shell extensions removed.");
		else
			std::println("Error removing shell extensions.");

	// At least one parameter (filename, not option or command): process file(s)
	if ((argc > 1) && (argv[1][0] != '/') && (argv[1][0] != '-')) // ...remember filenames cannot start with a forward slash because of options format
	{
		// Wildcard in filename: use file search
		if (SCharCount('*', argv[1]))
			ForEachFile(argv[1], &ProcessGroup);
		// Only one file
		else
			ProcessGroup(argv[1]);
	}

	// Too few parameters: output help (if we didn't register stuff)
	else if (!fRegisterShell && !fUnregisterShell)
	{
		std::println("");
		std::println("Usage:    c4group group [options] command(s)\n");
		std::println("Commands: -a[s] Add [as]  -m Move  -e[t] Extract [to]");
		std::println("          -v View  -d Delete  -r Rename  -s Sort");
		std::println("          -p Pack  -u Unpack  -x Explode");
		std::println("          -k Print maker");
		std::println("          -g [source] [target] [title] Make update");
		std::println("          -y[d] Apply update [and delete group file]");
		std::println("          -z Optimize a group to be similar (smaller update)");
		std::println("");
		std::println("Options:  /q Quiet /r Recursive /p Prompt at end");
		std::println("          /i Register shell /u Unregister shell");
		std::println("          /x:<command> Execute shell command when done");
		std::println("");
		std::println("Examples: c4group pack.c4g -a myfile.dat -v *.dat");
		std::println("          c4group pack.c4g -as myfile.dat myfile.bin");
		std::println("          c4group pack.c4g -et *.dat \\data\\mydatfiles\\");
		std::println("          c4group pack.c4g -et myfile.dat myfile.bak");
		std::println("          c4group pack.c4g -s \"*.bin|*.dat\"");
		std::println("          c4group pack.c4g -x");
		std::println("          c4group pack.c4g /q -k");
		std::println("          c4group update.c4u -g ver1.c4f ver2.c4f New_Version");
		std::println("          c4group ver1.c4f -z ver2.c4f");
		std::println("          c4group /i");
	}

	// Prompt at end
	if (fPromptAtEnd)
	{
		std::println("\nDone. Press any key to continue.");
		_getch();
	}

	// Execute when done
	if (strExecuteAtEnd[0])
	{
		std::println("Executing: {}", strExecuteAtEnd);

		STARTUPINFOA startInfo{};
		startInfo.cb = sizeof(startInfo);

		PROCESS_INFORMATION procInfo;

		CreateProcessA(strExecuteAtEnd, nullptr, nullptr, nullptr, false, 0, nullptr, nullptr, &startInfo, &procInfo);
	}

	// Done
	return iResult;
}
