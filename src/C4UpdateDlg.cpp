/*
 * LegacyClonk
 *
 * Copyright (c) RedWolf Design
 * Copyright (c) 2007, matthes
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

// dialogs for update, and the actual update application code

#include "C4Include.h"
#include "C4UpdateDlg.h"
#include "C4DownloadDlg.h"

#include <C4Log.h>

#include <format>

#ifdef _WIN32
#include <shellapi.h>
#else
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

int C4UpdateDlg::pid;
int C4UpdateDlg::c4group_output[2];
bool C4UpdateDlg::succeeded;

// C4UpdateDlg

C4UpdateDlg::C4UpdateDlg() : C4GUI::InfoDialog(LoadResStr(C4ResStrTableKey::IDS_TYPE_UPDATE), 10)
{
	// initial text update
	UpdateText();
	// assume update running
	UpdateRunning = true;
}

void C4UpdateDlg::UpdateText()
{
	if (!UpdateRunning)
		return;
#ifdef _WIN32
	AddLine("Win32 is currently not using the C4UpdateDlg!");
	UpdateRunning = false;
#else
	char c4group_output_buf[513];
	ssize_t amount_read;
	// transfer output to log window
	amount_read = read(c4group_output[0], c4group_output_buf, 512);
	// error
	if (amount_read == -1)
	{
		if (errno == EAGAIN)
			return;
		const std::string errorMessage{std::format("read error from c4group: {}", strerror(errno))};
		LogNTr(spdlog::level::err, errorMessage);
		AddLine(errorMessage.c_str());
		UpdateRunning = false;
		succeeded = false;
	}
	// EOF: Update done.
	else if (amount_read == 0)
	{
		// Close c4group output
		close(c4group_output[0]);
		// If c4group did not exit but something else caused EOF, then that's bad. But don't hang.
		int child_status = 0;
		if (waitpid(pid, &child_status, WNOHANG) == -1)
		{
			LogNTr(spdlog::level::err, "error in waitpid: {}", strerror(errno));
			AddLine(std::format("error in waitpid: {}", strerror(errno)).c_str());
			succeeded = false;
		}
		// check if c4group failed.
		else if (WIFEXITED(child_status) && WEXITSTATUS(child_status))
		{
			LogNTr(spdlog::level::err, "c4group returned status {}", WEXITSTATUS(child_status));
			AddLine(std::format("c4group returned status {}", WEXITSTATUS(child_status)).c_str());
			succeeded = false;
		}
		else if (WIFSIGNALED(child_status))
		{
			LogNTr(spdlog::level::err, "c4group killed with signal {}", WTERMSIG(child_status));
			AddLine(std::format("c4group killed with signal {}", WTERMSIG(child_status)).c_str());
			succeeded = false;
		}
		else
		{
			LogNTr("Done.");
			AddLine("Done.");
		}
		UpdateRunning = false;
	}
	else
	{
		c4group_output_buf[amount_read] = 0;
		// Fixme: This adds spurious newlines in the middle of the output.
		LogNTr(c4group_output_buf);
		AddLine(c4group_output_buf);
	}
#endif

	// Scroll to bottom
	if (pTextWin)
	{
		pTextWin->UpdateHeight();
		pTextWin->ScrollToBottom();
	}
}

// static update application function

bool C4UpdateDlg::DoUpdate(const C4GameVersion &rUpdateVersion, C4GUI::Screen *pScreen)
{
	std::string updateFile;
	std::string updateURL;
	// Double check for valid update
	if (!IsValidUpdate(rUpdateVersion)) return false;
	// Objects major update: we will update to the first minor of the next major version - we can not skip major versions or jump directly to a higher minor of the next major version.
	if (rUpdateVersion.iVer[2] > C4XVER3)
		updateFile = std::format(C4CFG_UpdateMajor, rUpdateVersion.iVer[0], rUpdateVersion.iVer[1], C4XVER3 + 1, 0, C4_OS);
	// Objects version match: engine update only
	else if ((rUpdateVersion.iVer[2] == C4XVER3) && (rUpdateVersion.iVer[3] == C4XVER4))
		updateFile = std::format(C4CFG_UpdateEngine, rUpdateVersion.iBuild, C4_OS);
	// Objects version mismatch: full objects update
	else
		updateFile = std::format(C4CFG_UpdateObjects, rUpdateVersion.iVer[0], rUpdateVersion.iVer[1], rUpdateVersion.iVer[2], rUpdateVersion.iVer[3], rUpdateVersion.iBuild, C4_OS);
	// Compose full update URL by using update server address and replacing last path element name with the update file
	int iLastElement = SCharLastPos('/', Config.Network.UpdateServerAddress);
	if (iLastElement > -1)
	{
		updateURL = Config.Network.UpdateServerAddress;
		updateURL.resize(iLastElement + 1);
		updateURL += updateFile;
	}
	else
	{
		// No last slash in update server address?
		// Append update file as new segment instead - maybe somebody wants
		// to set up their update server this way
		updateURL = Config.Network.UpdateServerAddress;
		updateURL += '/';
		updateURL += updateFile;
	}
	// Determine local filename for update group
	StdStrBuf strLocalFilename; strLocalFilename.Copy(GetFilename(updateFile.c_str()));
	// Download update group to temp path
	strLocalFilename.Copy(Config.AtTempPath(strLocalFilename.getData()));
	// Download update group
	if (!C4DownloadDlg::DownloadFile(LoadResStr(C4ResStrTableKey::IDS_TYPE_UPDATE), pScreen, updateURL.c_str(), strLocalFilename.getData(), LoadResStr(C4ResStrTableKey::IDS_MSG_UPDATENOTAVAILABLE)))
		// Download failed (return success, because error message has already been shown)
		return true;
	// Apply downloaded update
	return ApplyUpdate(strLocalFilename.getData(), true, pScreen);
}

bool C4UpdateDlg::ApplyUpdate(const char *strUpdateFile, bool fDeleteUpdate, C4GUI::Screen *pScreen)
{
	// Determine name of update program
	StdStrBuf strUpdateProg; strUpdateProg.Copy(C4CFN_UpdateProgram);
	// Windows: manually append extension because ExtractEntry() cannot properly glob and Extract() doesn't return failure values
#ifdef _WIN32
	strUpdateProg += ".exe";
#endif
	// Extract update program (the update should be applied using the new version)
	C4Group UpdateGroup, SubGroup;
	char strSubGroup[1024 + 1];
	if (!UpdateGroup.Open(strUpdateFile)) return false;
	// Look for update program at top level
	if (!UpdateGroup.ExtractEntry(strUpdateProg.getData(), strUpdateProg.getData()))
		// Not found: look for an engine update pack one level down
		if (UpdateGroup.FindEntry(std::format("cr_*_{}.c4u", C4_OS).c_str(), strSubGroup))
			// Extract update program from sub group
			if (SubGroup.OpenAsChild(&UpdateGroup, strSubGroup))
			{
				SubGroup.ExtractEntry(strUpdateProg.getData(), strUpdateProg.getData());
				SubGroup.Close();
			}
	UpdateGroup.Close();
	// Execute update program
	Log(C4ResStrTableKey::IDS_PRC_LAUNCHINGUPDATE);
	succeeded = true;
#ifdef _WIN32
	// Close editor if open
	HWND hwnd = FindWindowA(nullptr, C4EDITORCAPTION);
	if (hwnd) PostMessage(hwnd, WM_CLOSE, 0, 0);
	const std::string updateArgs{std::format("\"{}\" /p -w \"" C4ENGINECAPTION "\" -w \"" C4EDITORCAPTION "\" -w 2000 {}", strUpdateFile, fDeleteUpdate ? "-yd" : "-y")};
	const auto iError = ShellExecuteA(nullptr, "runas", strUpdateProg.getData(), updateArgs.c_str(), Config.General.ExePath, SW_SHOW);
	if (reinterpret_cast<intptr_t>(iError) <= 32) return false;
	// must quit ourselves for update program to work
	if (succeeded) Application.Quit();
#else
	if (pipe(c4group_output) == -1)
	{
		LogNTr(spdlog::level::err, "Error creating pipe");
		return false;
	}
	switch (pid = fork())
	{
	// Error
	case -1:
		LogNTr(spdlog::level::err, "Error creating update child process.");
		return false;
	// Child process
	case 0:
		// Close unused read end
		close(c4group_output[0]);
		// redirect stdout and stderr to the parent
		dup2(c4group_output[1], STDOUT_FILENO);
		dup2(c4group_output[1], STDERR_FILENO);
		if (c4group_output[1] != STDOUT_FILENO && c4group_output[1] != STDERR_FILENO)
			close(c4group_output[1]);
		execl(C4CFN_UpdateProgram, C4CFN_UpdateProgram, "-v", strUpdateFile, (fDeleteUpdate ? "-yd" : "-y"), static_cast<char *>(0));
		printf("execl failed: %s\n", strerror(errno));
		exit(1);
	// Parent process
	default:
		// Close unused write end
		close(c4group_output[1]);
		// disable blocking
		fcntl(c4group_output[0], F_SETFL, O_NONBLOCK);
		// Open the update log dialog (this will update itself automatically from c4group_output)
		pScreen->ShowRemoveDlg(new C4UpdateDlg());
		break;
	}
#endif
	// done
	return succeeded;
}

bool C4UpdateDlg::IsValidUpdate(const C4GameVersion &rNewVer)
{
	// Engine or game version mismatch
	if ((rNewVer.iVer[0] != C4XVER1) || (rNewVer.iVer[1] != C4XVER2)) return false;
	// Objects major is higher...
	if ((rNewVer.iVer[2] > C4XVER3)
		// ...or objects major is the same and objects minor is higher...
		|| ((rNewVer.iVer[2] == C4XVER3) && (rNewVer.iVer[3] > C4XVER4))
		// ...or build number is higher
		|| (rNewVer.iBuild > C4XVERBUILD))
		// Update okay
		return true;
	// Otherwise
	return false;
}

bool C4UpdateDlg::CheckForUpdates(C4GUI::Screen *pScreen, bool fAutomatic)
{
	// Automatic update only once a day
	if (fAutomatic)
		if (time(nullptr) - Config.Network.LastUpdateTime < 60 * 60 * 24)
			return false;
	// Store the time of this update check (whether it's automatic or not or successful or not)
	Config.Network.LastUpdateTime = static_cast<int32_t>(time(nullptr));
	// Get current update version from server
	C4GameVersion UpdateVersion;
	C4GUI::Dialog *pWaitDlg = nullptr;
	if (pScreen && C4GUI::IsGUIValid())
	{
		pWaitDlg = new C4GUI::MessageDialog(LoadResStr(C4ResStrTableKey::IDS_MSG_LOOKINGFORUPDATES), Config.Network.UpdateServerAddress, C4GUI::MessageDialog::btnAbort, C4GUI::Ico_Ex_Update, C4GUI::MessageDialog::dsRegular);
		pWaitDlg->SetDelOnClose(false);
		pScreen->ShowDialog(pWaitDlg, false);
	}
	C4Network2VersionInfoClient VerChecker;
	bool fSuccess = false, fAborted = false;
	StdStrBuf strUpdateRedirect;
	const std::string query{std::format("{}?action=version", +Config.Network.UpdateServerAddress)};
	if (VerChecker.Init() && VerChecker.SetServer(query) && VerChecker.QueryVersion())
	{
		Application.InteractiveThread.AddProc(&VerChecker);
		// wait for version check to terminate
		while (VerChecker.isBusy())
		{
			C4AppHandleResult hr;
			while ((hr = Application.HandleMessage()) == HR_Message) {}
			// check for program abort
			if (hr == HR_Failure) { fAborted = true; break; }
			// check for dialog close
			if (pScreen && pWaitDlg) if (!C4GUI::IsGUIValid() || !pWaitDlg->IsShown()) { fAborted = true; break; }
		}
		if (!fAborted)
		{
			fSuccess = VerChecker.GetVersion(&UpdateVersion);
			VerChecker.GetRedirect(strUpdateRedirect);
		}
		Application.InteractiveThread.RemoveProc(&VerChecker);
	}
	if (pScreen && C4GUI::IsGUIValid()) delete pWaitDlg;
	// User abort
	if (fAborted)
	{
		return false;
	}
	// Error during update check
	if (!fSuccess)
	{
		if (pScreen)
		{
			StdStrBuf sError; sError.Copy(LoadResStr(C4ResStrTableKey::IDS_MSG_UPDATEFAILED));
			const char *szErrMsg = VerChecker.GetError();
			if (szErrMsg)
			{
				sError.Append(": ");
				sError.Append(szErrMsg);
			}
			pScreen->ShowMessage(sError.getData(), Config.Network.UpdateServerAddress, C4GUI::Ico_Ex_Update);
		}
		return false;
	}

	// UpdateServer Redirection
	if ((strUpdateRedirect.getLength() > 0) && !SEqual(strUpdateRedirect.getData(), Config.Network.UpdateServerAddress))
	{
		// this is a new redirect. Inform the user and auto-change servers if desired
		const char *newServer = strUpdateRedirect.getData();
		if (pScreen)
		{
			const std::string message{LoadResStr(C4ResStrTableKey::IDS_NET_SERVERREDIRECTMSG, newServer)};
			if (!pScreen->ShowMessageModal(message.c_str(), LoadResStr(C4ResStrTableKey::IDS_NET_SERVERREDIRECT), C4GUI::MessageDialog::btnYesNo, C4GUI::Ico_OfficialServer))
			{
				// apply new server setting
				SCopy(newServer, Config.Network.UpdateServerAddress, CFG_MaxString);
				Config.Save();
				pScreen->ShowMessageModal(LoadResStr(C4ResStrTableKey::IDS_NET_SERVERREDIRECTDONE), LoadResStr(C4ResStrTableKey::IDS_NET_SERVERREDIRECT), C4GUI::MessageDialog::btnOK, C4GUI::Ico_OfficialServer);
				// abort the update check - user should try again
				return false;
			}
		}
		else
		{
			SCopy(newServer, Config.Network.UpdateServerAddress, CFG_MaxString);
			Config.Save();
			return false;
		}
	}

	if (!pScreen)
	{
		return C4UpdateDlg::IsValidUpdate(UpdateVersion);
	}

	// Applicable update available
	if (C4UpdateDlg::IsValidUpdate(UpdateVersion))
	{
		// Prompt user, then apply update
		const std::string message{LoadResStr(C4ResStrTableKey::IDS_MSG_ANUPDATETOVERSIONISAVAILA, UpdateVersion.GetString())};
		if (pScreen->ShowMessageModal(message.c_str(), Config.Network.UpdateServerAddress, C4GUI::MessageDialog::btnYesNo, C4GUI::Ico_Ex_Update))
			if (!DoUpdate(UpdateVersion, pScreen))
				pScreen->ShowMessage(LoadResStr(C4ResStrTableKey::IDS_MSG_UPDATEFAILED), Config.Network.UpdateServerAddress, C4GUI::Ico_Ex_Update);
			else
				return true;
	}
	// No applicable update available
	else
	{
		// Message (if not automatic)
		if (!fAutomatic)
			pScreen->ShowMessage(LoadResStr(C4ResStrTableKey::IDS_MSG_NOUPDATEAVAILABLEFORTHISV), Config.Network.UpdateServerAddress, C4GUI::Ico_Ex_Update);
	}
	// Done (and no update has been done)
	return false;
}

// *** C4Network2VersionInfoClient

bool C4Network2VersionInfoClient::QueryVersion()
{
	// Perform an Query query
	return Query(StdBuf{}, false);
}

bool C4Network2VersionInfoClient::GetVersion(C4GameVersion *piVerOut)
{
	// Sanity check
	if (isBusy() || !isSuccess()) return false;
	// Parse response
	piVerOut->Set("", 0, 0, 0, 0, 0);
	try
	{
		CompileFromBuf<StdCompilerINIRead>(mkNamingAdapt(
			mkNamingAdapt(
				mkParAdapt(*piVerOut, false),
				"Version"),
			C4ENGINENAME), getResultString());
	}
	catch (const StdCompiler::Exception &e)
	{
		SetError(e.what());
		return false;
	}
	// validate version
	if (!piVerOut->iVer[0])
	{
		SetError(LoadResStr(C4ResStrTableKey::IDS_ERR_INVALIDREPLYFROMSERVER));
		return false;
	}
	// done; version OK!
	return true;
}

bool C4Network2VersionInfoClient::GetRedirect(StdStrBuf &rRedirect)
{
	// Sanity check
	if (isBusy() || !isSuccess()) return false;
	StdStrBuf strUpdateRedirect;
	try
	{
		CompileFromBuf<StdCompilerINIRead>(mkNamingAdapt(
			mkNamingAdapt(mkParAdapt(strUpdateRedirect, StdCompiler::RCT_All), "UpdateServerRedirect", ""),
			C4ENGINENAME), getResultString());
	}
	catch (const StdCompiler::Exception &e)
	{
		SetError(e.what());
		return false;
	}
	// did we get something?
	if (strUpdateRedirect.getLength() > 0)
	{
		rRedirect.Copy(strUpdateRedirect);
		return true;
	}
	// no, we didn't
	rRedirect.Clear();
	return false;
}
