/*
 * LegacyClonk
 *
 * Copyright (c) 1998-2000, Matthes Bender (RedWolf Design)
 * Copyright (c) 2017-2024, The LegacyClonk Team and contributors
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

/* Handles engine execution in developer mode */

#pragma once

#include "C4PropertyDlg.h"
#include "C4ToolsDlg.h"
#include "C4ObjectListDlg.h"
#include "C4EditCursor.h"
#include "C4Game.h"

#include <StdWindow.h>

#ifdef WITH_DEVELOPER_MODE
#endif

#ifdef WITH_DEVELOPER_MODE
#include <StdGtkWindow.h>
typedef CStdGtkWindow C4ConsoleBase;
#else
typedef CStdWindow C4ConsoleBase;
#endif

#include "imgui/textselect.hpp"

class C4Console : public C4ConsoleBase
{
public:
	C4Console();
	virtual ~C4Console() override;
	bool Editing;
	C4PropertyDlg   PropertyDlg;
	C4ToolsDlg      ToolsDlg;
	C4ObjectListDlg ObjectListDlg;
	C4EditCursor    EditCursor;
	void Default();
	virtual void Clear() override;
	virtual void Close() override;
	virtual bool Init(CStdApp *app, const char *title, const C4Rect &bounds = DefaultBounds, CStdWindow *parent = nullptr, std::uint32_t AdditionalFlags = 0, std::int32_t MinWidth = 250, std::int32_t MinHeight = 250) override;
	void InitGUI();
	void Execute();
	void ClearPointers(C4Object *pObj);
	void Message(const char *message);
	void SetCaption(const char *szCaption);
	void In(const char *szText);
	void Out(const char *szText);
	void ClearLog(); // empty log text
	void DoPlay();
	void DoHalt();
	void UpdateHaltCtrls(bool fHalt);
	bool OpenGame(const char *szCmdLine = nullptr);
	bool TogglePause(); // key callpack: pause

protected:
	void CloseGame();
	bool fGameOpen;
	void Sec1Timer() override { Game.Sec1Timer(); Application.DoSec1Timers(); }
	// Menu
	void PlayerJoin();
	void EditObjects();
	void EditInfo();
	void EditScript();
	void EditTitle();
	void ViewportNew();
#ifdef _WIN32
	bool FileSelect(char *sFilename, int iSize, const char *szFilter, DWORD    dwFlags, bool fSave = false);
#else
	bool FileSelect(char *sFilename, int iSize, const char *szFilter, uint32_t dwFlags, bool fSave = false);
#endif
public:
	static void SDLCALL FileSaveGameAsCallback(void* userdata, const char* const* filelist, int filter);
	static void SDLCALL FileSaveScenarioAsCallback(void* userdata, const char* const* filelist, int filter);
	static void SDLCALL FileOpenCallback(void* userdata, const char* const* filelist, int filter);
protected:
	void SaveGame(bool fSaveGame);
	void FileSaveAs(bool fSaveGame);
	void FileSaveMainThread(std::string Filename, bool fSaveGame);
	void FileSave(bool fSaveGame);
	void FileOpen();
	void FileOpenWPlrs();
	void FileClose();
	void FileQuit();
	void FileRecord();
	void Draw();

	static int TextEditCallbackStub(ImGuiInputTextCallbackData* data);
	int TextEditCallback(ImGuiInputTextCallbackData* data);

	std::string_view GetLineAtIdx(std::size_t idx);
	std::size_t GetNumLines();
	TextSelect* LogTextSelect;
	std::vector<std::string> logBuffer;
	static constexpr std::size_t LogBufferSize{1024};
	ImGuiListClipper LogClipper;


	bool halt{false};
	C4AulFunc *selectedFunction{nullptr};
	std::string popupMessage;

	void HandleMessage(SDL_Event&) override;
#ifdef _WIN32
	friend LRESULT CALLBACK ConsoleWinProc(HWND wnd, UINT Msg, WPARAM wParam, LPARAM lParam);
#endif
private:
	std::uint32_t ToolCursorImage;
	std::uint32_t ToolMouseImage;
	std::uint32_t ToolBrushImage;
	std::uint32_t PlayImage;
	std::uint32_t HaltImage;
};

extern C4Console Console;
