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

#include <ctime>
#include <format>
#include <iomanip>
#include <sstream>

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

#include <boost/json.hpp>

#define C4XVERBUILD_STRING C4XVERTOC4XVERS(C4XVERBUILD)

static constexpr auto UpdateFileName = "lc_" C4XVERBUILD_STRING "_" C4_OS ".c4u";

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

bool C4UpdateDlg::DoUpdate(const std::int64_t assetId, C4GUI::Screen *pScreen)
{
	const std::string filePath{Config.AtTempPath(UpdateFileName)};
	// Download update group
	if (!C4DownloadDlg::DownloadFile(LoadResStr(C4ResStrTableKey::IDS_TYPE_UPDATE), pScreen, std::format("https://api.github.com/repos/legacyclonk/LegacyClonk/releases/assets/{}", assetId).c_str(), filePath.c_str(), LoadResStr(C4ResStrTableKey::IDS_MSG_UPDATENOTAVAILABLE)))
		// Download failed (return success, because error message has already been shown)
		return true;
	// Apply downloaded update
	return ApplyUpdate(filePath.c_str(), true, pScreen);
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
		if (UpdateGroup.FindEntry(std::format("cr_*_%s.c4u", C4_OS).c_str(), strSubGroup))
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

bool C4UpdateDlg::CheckForUpdates(C4GUI::Screen *pScreen, bool fAutomatic)
{
	// Automatic update only once a day
	if (fAutomatic)
		if (time(nullptr) - Config.Network.LastUpdateTime < 60 * 60 * 24)
			return false;
	// Store the time of this update check (whether it's automatic or not or successful or not)
	Config.Network.LastUpdateTime = static_cast<int32_t>(time(nullptr));
	// Get current update version from server
	C4GUI::Dialog *pWaitDlg = nullptr;

	const auto showErrorDialog = [pScreen](const char *const body, const char *const title)
	{
		if (pScreen)
		{
			std::string message{LoadResStr(C4ResStrTableKey::IDS_MSG_UPDATEFAILED)};
			if (body)
			{
				message.append(": ").append(body);
			}

			pScreen->ShowMessage(message.c_str(), title, C4GUI::Ico_Ex_Update);
		}

		return false;
	};

	C4Network2HTTPClient client;

	if (!client.Init() || !client.SetServer("https://api.github.com/repos/legacyclonk/LegacyClonk/releases/tags/v" C4XVERBUILD_STRING "_multiple_sections_open_beta"))
	{
		return showErrorDialog(client.GetError(), LoadResStr(C4ResStrTableKey::IDS_MSG_LOOKINGFORUPDATES));
	}

	if (pScreen && C4GUI::IsGUIValid())
	{
		pWaitDlg = new C4GUI::MessageDialog(LoadResStr(C4ResStrTableKey::IDS_MSG_LOOKINGFORUPDATES), client.getServerName(), C4GUI::MessageDialog::btnAbort, C4GUI::Ico_Ex_Update, C4GUI::MessageDialog::dsRegular);
		pWaitDlg->SetDelOnClose(false);
		pScreen->ShowDialog(pWaitDlg, false);
	}

	bool fSuccess = false, fAborted = false;
	if (client.Query(StdBuf{}, false))
	{
		// wait for version check to terminate
		while (client.isBusy())
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
			fSuccess = client.isSuccess();
		}
	}
	if (pScreen && C4GUI::IsGUIValid()) delete pWaitDlg;
	// User abort
	if (fAborted)
	{
		return false;
	}

	if (!fSuccess)
	{
		return showErrorDialog(client.GetError(), client.getServerName());
	}

	std::tm timePoint{};
	std::int64_t assetId{0};

	try
	{
		const auto parseDateTime = [](const std::string_view string)
		{
			std::stringstream stream;
			stream << string;

			std::tm timePoint;
			stream >> std::get_time(&timePoint, "%Y-%m-%dT%TZ");
			timePoint.tm_isdst = -1;
			return timePoint;
		};

		const auto assets = boost::json::parse(client.getResultString().getData()).as_object().at("assets").as_array();

		if (const auto it = std::find_if(assets.begin(), assets.end(), [](const auto &asset) { return asset.as_object().at("name").as_string() == UpdateFileName; }); it != assets.end())
		{
			const auto &asset = it->get_object();

			auto updatedAt = parseDateTime(static_cast<std::string_view>(asset.at("updated_at").as_string()));
			auto engineBuiltAt = parseDateTime(C4UPDATE_BUILD_TIMESTAMP);

			if (std::mktime(&updatedAt) - std::mktime(&engineBuiltAt) > 10 * 60)
			{
				timePoint = updatedAt;
				assetId = asset.at("id").to_number<std::int64_t>();
			}
		}
	}
	catch (const std::exception &e)
	{
		return showErrorDialog(e.what(), client.getServerName());
	}

	if (!pScreen)
	{
		return timePoint.tm_year > 0;
	}

	// Applicable update available
	if (timePoint.tm_year > 0)
	{
		// Prompt user, then apply update
		std::ostringstream stream;
		stream << std::put_time(&timePoint, "%c");
		const std::string msg{LoadResStr(C4ResStrTableKey::IDS_MSG_ANUPDATETOVERSIONISAVAILA, stream.str())};
		if (pScreen->ShowMessageModal(msg.c_str(), client.getServerName(), C4GUI::MessageDialog::btnYesNo, C4GUI::Ico_Ex_Update))
			if (!DoUpdate(assetId, pScreen))
				pScreen->ShowMessage(LoadResStr(C4ResStrTableKey::IDS_MSG_UPDATEFAILED), client.getServerName(), C4GUI::Ico_Ex_Update);
			else
				return true;
	}
	// No applicable update available
	else
	{
		// Message (if not automatic)
		if (!fAutomatic)
			pScreen->ShowMessage(LoadResStr(C4ResStrTableKey::IDS_MSG_NOUPDATEAVAILABLEFORTHISV), client.getServerName(), C4GUI::Ico_Ex_Update);
	}
	// Done (and no update has been done)
	return false;
}
