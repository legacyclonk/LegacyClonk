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

/* Operates viewports, message board and draws the game */

#include <C4GraphicsSystem.h>

#include <C4Viewport.h>
#include <C4Application.h>
#include <C4Console.h>
#include <C4Random.h>
#include <C4SurfaceFile.h>
#include <C4FullScreen.h>
#include <C4Gui.h>
#include <C4LoaderScreen.h>
#include <C4Wrappers.h>
#include <C4Player.h>
#include <C4Object.h>
#include <C4SoundSystem.h>

#include <StdBitmap.h>
#include <StdPNG.h>

#ifdef _WIN32
#include "res/engine_resource.h"
#endif

#include <algorithm>
#include <format>
#include <ranges>
#include <stdexcept>

C4GraphicsSystem::C4GraphicsSystem()
{
	Default();
}

C4GraphicsSystem::~C4GraphicsSystem()
{
	Clear();
}

bool C4GraphicsSystem::Init()
{
	// Success
	return true;
}

void C4GraphicsSystem::Clear()
{
	// Clear message board
	MessageBoard.Clear();
	// Clear upper board
	UpperBoard.Clear();
	// clear loader
	delete pLoaderScreen; pLoaderScreen = nullptr;
	// Close viewports
	Viewports.clear();
	// No debug stuff
	DeactivateDebugOutput();
}

bool C4GraphicsSystem::SetPalette()
{
	// Set primary palette by game palette
	if (!Application.DDraw->SetPrimaryPalette(Game.GraphicsResource.GamePalette, Game.GraphicsResource.AlphaPalette)) return false;
	return true;
}

extern int32_t iLastControlSize, iPacketDelay;
extern int32_t ControlQueueSize, ControlQueueDataSize;

int32_t ScreenTick = 0, ScreenRate = 1;

bool C4GraphicsSystem::StartDrawing()
{
	// only if ddraw is ready
	if (!Application.DDraw) return false;
	if (!Application.DDraw->Active) return false;

	// only in main thread
	if (!Application.IsMainThread()) return false;

	// Don't draw if application is inactive (unless enabled by config)
	if (!Application.Active)
	{
		// Player mode
		const auto &renderInactive = Config.Graphics.RenderInactive;
		if (Application.isFullScreen && !(renderInactive & C4ConfigGraphics::Fullscreen))
			return false;
		// Developer mode
		if (!Application.isFullScreen && !(renderInactive & C4ConfigGraphics::Console))
			return false;
	}

	// drawing OK
	return true;
}

void C4GraphicsSystem::FinishDrawing()
{
	if (Application.isFullScreen) Application.DDraw->PageFlip();
}

void C4GraphicsSystem::Execute()
{
	// activity check
	if (!StartDrawing()) return;

	bool fBGDrawn = false;

	// If lobby running, message board only (page flip done by startup message board)
	if (!Game.pGUI || !Game.pGUI->HasFullscreenDialog(true)) // allow for message board behind GUI
		if (Game.Network.isLobbyActive() || !Game.IsRunning)
			if (Application.isFullScreen)
			{
				// Message board
				if (iRedrawBackground) ClearFullscreenBackground();
				MessageBoard.Execute();
				if (!Game.pGUI || !C4GUI::IsActive())
				{
					FinishDrawing(); return;
				}
				fBGDrawn = true;
			}

	// fullscreen GUI?
	if (Application.isFullScreen && Game.pGUI && C4GUI::IsActive() && (Game.pGUI->HasFullscreenDialog(false) || !Game.IsRunning))
	{
		if (!fBGDrawn && iRedrawBackground) ClearFullscreenBackground();
		Game.pGUI->Render(!fBGDrawn);
		FinishDrawing();
		return;
	}

	// Fixed screen rate in old network
	ScreenRate = 1;

	// Background redraw
	if (Application.isFullScreen)
		if (iRedrawBackground)
			DrawFullscreenBackground();

	// Screen rate skip frame draw
	ScreenTick++; if (ScreenTick >= ScreenRate) ScreenTick = 0;

	// Reset object audibility
	Game.ResetAudibility();

	// some hack to ensure the mouse is drawn after a dialog close and before any
	// movement messages
	if (Game.pGUI && !C4GUI::IsActive())
		SetMouseInGUI(false, false);

	// Viewports
	for (const auto &cvp : Viewports)
		cvp->Execute();

	if (Application.isFullScreen)
	{
		// Message board
		MessageBoard.Execute();

		// Upper board
		UpperBoard.Execute();

		// Help & Messages
		DrawHelp();
		DrawHoldMessages();
		DrawFlashMessage();
	}

	// InGame-GUI
	if (Game.pGUI && C4GUI::IsActive())
	{
		Game.pGUI->Render(false);
	}

	// Palette update
	if (fSetPalette) { SetPalette(); fSetPalette = false; }

	// gamma update
	if (fSetGamma)
	{
		ApplyGamma();
		fSetGamma = false;
	}

	// done
	FinishDrawing();
}

bool C4GraphicsSystem::CloseViewport(C4Viewport *cvp)
{
	if (!cvp) return false;
	if (const auto it = std::find_if(Viewports.begin(), Viewports.end(), [cvp](const auto &viewport)
		{
			return viewport.get() == cvp;
		}); it != Viewports.end())
	{
		Viewports.erase(it);
	}
	else
	{
		return false;
	}
	StartSoundEffect("CloseViewport");
	// Recalculate viewports
	RecalculateViewports();
	// Done
	return true;
}

bool C4GraphicsSystem::CreateViewport(int32_t iPlayer, bool fSilent)
{
	// Create and init new viewport, add to viewport list
	C4Viewport *nvp = new C4Viewport;
	bool fOkay = false;
	if (Application.isFullScreen)
		fOkay = nvp->Init(iPlayer, false);
	else
		fOkay = nvp->Init(&Console, &Application, iPlayer);
	if (!fOkay)
	{
		delete nvp;
		return false;
	}
	Viewports.emplace_back(nvp);
	// Recalculate viewports
	RecalculateViewports();
	// Viewports start off at centered position
	nvp->CenterPosition();
	// Action sound
	if (!fSilent)
	{
		StartSoundEffect("CloseViewport");
	}
	return true;
}

void C4GraphicsSystem::ClearPointers(C4Object *pObj)
{
	for (const auto &cvp : Viewports)
		cvp->ClearPointers(pObj);
}

void C4GraphicsSystem::Default()
{
	UpperBoard.Default();
	MessageBoard.Default();
	Viewports.clear();
	InvalidateBg();
	ViewportArea.Default();
	ShowVertices = false;
	ShowAction = false;
	ShowCommand = false;
	ShowEntrance = false;
	ShowPathfinder = false;
	ShowNetstatus = false;
	ShowSolidMask = false;
	ShowHelp = false;
	FlashMessageText[0] = 0;
	FlashMessageTime = 0; FlashMessageX = FlashMessageY = 0;
	fSetPalette = false;
	for (int32_t iRamp = 0; iRamp < 3 * C4MaxGammaRamps; iRamp += 3)
	{
		dwGamma[iRamp + 0] = 0; dwGamma[iRamp + 1] = 0x808080; dwGamma[iRamp + 2] = 0xffffff;
	}
	fSetGamma = false;
	pLoaderScreen = nullptr;
}

void C4GraphicsSystem::DrawFullscreenBackground()
{
	for (size_t i = 0, iNum = BackgroundAreas.GetCount(); i < iNum; ++i)
	{
		const C4Rect &rc = BackgroundAreas.Get(i);
		Application.DDraw->BlitSurfaceTile(Game.GraphicsResource.fctBackground.Surface, Application.DDraw->lpBack, rc.x, rc.y, rc.Wdt, rc.Hgt, -rc.x, -rc.y);
	}
	--iRedrawBackground;
}

void C4GraphicsSystem::ClearFullscreenBackground()
{
	Application.DDraw->FillBG(0);
	--iRedrawBackground;
}

bool C4GraphicsSystem::InitLoaderScreen(const char *szLoaderSpec)
{
	// create new loader; overwrite current only if successful
	C4LoaderScreen *pNewLoader = new C4LoaderScreen();
	if (!pNewLoader->Init(szLoaderSpec)) { delete pNewLoader; return false; }
	delete pLoaderScreen;
	pLoaderScreen = pNewLoader;
	// apply user gamma for loader
	ApplyGamma();
	// done, success
	return true;
}

bool C4GraphicsSystem::CloseViewport(int32_t iPlayer, bool fSilent)
{
	// Close all matching viewports
	if (const auto it = std::remove_if(Viewports.begin(), Viewports.end(), [iPlayer](const auto &viewport)
		{
			return viewport->Player == iPlayer || (iPlayer == NO_OWNER && viewport->fIsNoOwnerViewport);
		}); it != Viewports.end())
	{
		Viewports.erase(it, Viewports.end());
		// Recalculate viewports
		RecalculateViewports();
		if (!fSilent)
		{
			// Action sound
			StartSoundEffect("CloseViewport");
		}
	}
	return true;
}

void C4GraphicsSystem::RecalculateViewports()
{
	// Fullscreen only
	if (!Application.isFullScreen) return;

	// Sort viewports
	SortViewportsByPlayerControl();

	// Viewport area
	int32_t iBorderTop = 0, iBorderBottom = 0;
	if (Config.Graphics.UpperBoard)
		iBorderTop = C4UpperBoard::Height();
	iBorderBottom = MessageBoard.Output.Hgt;
	ViewportArea.Set(Application.DDraw->lpBack, 0, iBorderTop, Config.Graphics.ResX, Config.Graphics.ResY - iBorderTop - iBorderBottom);

	// Redraw flag
	InvalidateBg();
#ifdef _WIN32
	// reset mouse clipping
	ClipCursor(nullptr);
#else
	// StdWindow handles this.
#endif
	// reset GUI dlg pos
	if (Game.pGUI)
		Game.pGUI->SetPreferredDlgRect(C4Rect(ViewportArea.X, ViewportArea.Y, ViewportArea.Wdt, ViewportArea.Hgt));

	// fullscreen background: First, cover all of screen
	BackgroundAreas.Clear();
	BackgroundAreas.AddRect(C4Rect(ViewportArea.X, ViewportArea.Y, ViewportArea.Wdt, ViewportArea.Hgt));

	// Viewports
	const int32_t iViews{GetViewportCount()};
	if (!iViews) return;
	int32_t iViewsH = static_cast<int32_t>(sqrt(float(iViews)));
	int32_t iViewsX = iViews / iViewsH;
	int32_t iViewsL = iViews % iViewsH;
	int32_t cViewH, cViewX, ciViewsX;
	int32_t cViewWdt, cViewHgt, cOffWdt, cOffHgt, cOffX, cOffY;
	auto cvp = Viewports.begin();
	for (cViewH = 0; cViewH < iViewsH; cViewH++)
	{
		ciViewsX = iViewsX; if (cViewH < iViewsL) ciViewsX++;
		for (cViewX = 0; cViewX < ciViewsX; cViewX++)
		{
			cViewWdt = ViewportArea.Wdt / ciViewsX;
			cViewHgt = ViewportArea.Hgt / iViewsH;
			cOffX = ViewportArea.X;
			cOffY = ViewportArea.Y;
			cOffWdt = cOffHgt = 0;
			int32_t ViewportScrollBorder = Application.isFullScreen ? C4ViewportScrollBorder : 0;

			C4Section &section{cvp->get()->GetViewSection()};
			if (ciViewsX * std::min<int32_t>(cViewWdt, section.Landscape.Width + 2 * ViewportScrollBorder) < ViewportArea.Wdt)
				cOffX = (ViewportArea.Wdt - ciViewsX * std::min<int32_t>(cViewWdt, section.Landscape.Width + 2 * ViewportScrollBorder)) / 2;
			if (iViewsH * std::min<int32_t>(cViewHgt, section.Landscape.Height + 2 * ViewportScrollBorder) < ViewportArea.Hgt)
				cOffY = (ViewportArea.Hgt - iViewsH * std::min<int32_t>(cViewHgt, section.Landscape.Height + 2 * ViewportScrollBorder)) / 2 + ViewportArea.Y;
			if (Config.Graphics.SplitscreenDividers)
			{
				if (cViewX < ciViewsX - 1) cOffWdt = 4;
				if (cViewH < iViewsH - 1) cOffHgt = 4;
			}
			int32_t coViewWdt = cViewWdt - cOffWdt; if (coViewWdt > section.Landscape.Width + 2 * ViewportScrollBorder) { coViewWdt = section.Landscape.Width + 2 * ViewportScrollBorder; }
			int32_t coViewHgt = cViewHgt - cOffHgt; if (coViewHgt > section.Landscape.Height + 2 * ViewportScrollBorder) { coViewHgt = section.Landscape.Height + 2 * ViewportScrollBorder; }
			C4Rect rcOut(cOffX + cViewX * cViewWdt, cOffY + cViewH * cViewHgt, coViewWdt, coViewHgt);
			(*cvp)->SetOutputSize(rcOut.x, rcOut.y, rcOut.x, rcOut.y, rcOut.Wdt, rcOut.Hgt);
			++cvp;
			// clip down area avaiable for background drawing
			BackgroundAreas.ClipByRect(rcOut);
		}
	}
}

int32_t C4GraphicsSystem::GetViewportCount()
{
	return Viewports.size();
}

C4Viewport *C4GraphicsSystem::GetViewport(int32_t iPlayer)
{
	if (const auto cvp = std::find_if(Viewports.begin(), Viewports.end(), [iPlayer](const auto &viewport)
		{
			return viewport->Player == iPlayer || (iPlayer == NO_OWNER && viewport->fIsNoOwnerViewport);
		}); cvp != Viewports.end())
	{
		return cvp->get();
	}
	return nullptr;
}

int32_t LayoutOrder(int32_t iControl)
{
	// Convert keyboard control index to keyboard layout order
	switch (iControl)
	{
	case C4P_Control_Keyboard1: return 0;
	case C4P_Control_Keyboard2: return 3;
	case C4P_Control_Keyboard3: return 1;
	case C4P_Control_Keyboard4: return 2;
	}
	return iControl;
}

void C4GraphicsSystem::SortViewportsByPlayerControl()
{
	std::sort(Viewports.begin(), Viewports.end(), [](const auto &a, const auto &b)
		{
			const auto plrA = Game.Players.Get(a->Player);
			const auto plrB = Game.Players.Get(b->Player);
			return plrA && plrB && LayoutOrder(plrA->Control) < LayoutOrder(plrB->Control);
		});
}

void C4GraphicsSystem::MouseMove(int32_t iButton, int32_t iX, int32_t iY, uint32_t dwKeyParam, class C4Viewport *pVP)
{
	const auto scale = Application.GetScale();
	iX = static_cast<int32_t>(ceilf(iX / scale));
	iY = static_cast<int32_t>(ceilf(iY / scale));
	// pass on to GUI
	// Special: Don't pass if dragging and button is not upped
	if (Game.pGUI && Game.pGUI->IsActive() && !Game.MouseControl.IsDragging())
	{
		bool fResult = Game.pGUI->MouseInput(iButton, iX, iY, dwKeyParam, nullptr, pVP);
		if (Game.pGUI && Game.pGUI->HasMouseFocus()) { SetMouseInGUI(true, true); return; }
		// non-exclusive GUI: inform mouse-control about GUI-result
		SetMouseInGUI(fResult, true);
		// abort if GUI processed it
		if (fResult) return;
	}
	else
		// no GUI: mouse is not in GUI
		SetMouseInGUI(false, true);
	// mouse control enabled?
	if (!Game.MouseControl.IsActive())
	{
		// enable mouse in GUI, if a mouse-only-dlg is displayed
		if (Game.pGUI && Game.pGUI->GetMouseControlledDialogCount())
			SetMouseInGUI(true, true);
		return;
	}
	// Pass on to mouse controlled viewport
	MouseMoveToViewport(iButton, iX, iY, dwKeyParam);
}

void C4GraphicsSystem::MouseMoveToViewport(int32_t iButton, int32_t iX, int32_t iY, uint32_t dwKeyParam)
{
	// Pass on to mouse controlled viewport
	for (const auto &cvp : Viewports)
		if (Game.MouseControl.IsViewport(cvp.get()))
			Game.MouseControl.Move(iButton,
				BoundBy<int32_t>(iX - cvp->OutX, 0, cvp->ViewWdt - 1),
				BoundBy<int32_t>(iY - cvp->OutY, 0, cvp->ViewHgt - 1),
				dwKeyParam);
}

void C4GraphicsSystem::SetMouseInGUI(bool fInGUI, bool fByMouse)
{
	// inform mouse control and GUI
	if (Game.pGUI)
	{
		Game.pGUI->Mouse.SetOwnedMouse(fInGUI);
		// initial movement to ensure mouse control pos is correct
		if (!Game.MouseControl.IsMouseOwned() && !fInGUI && !fByMouse)
		{
			Game.MouseControl.SetOwnedMouse(true);
			MouseMoveToViewport(C4MC_Button_None, Game.pGUI->Mouse.x, Game.pGUI->Mouse.y, Game.pGUI->Mouse.dwKeys);
		}
	}
	Game.MouseControl.SetOwnedMouse(!fInGUI);
}

bool C4GraphicsSystem::SaveScreenshot(bool fSaveAll)
{
	// Filename
	char szFilename[_MAX_PATH + 1];
	int32_t iScreenshotIndex = 1;
	const char *strFilePath = nullptr;
	do
		FormatWithNull(szFilename, "Screenshot{:03}.png", iScreenshotIndex++);
	while (FileExists(strFilePath = Config.AtScreenshotPath(szFilename)));
	bool fSuccess = DoSaveScreenshot(fSaveAll, strFilePath);

	// log if successful/where it has been stored
	if (fSuccess)
	{
		Log(C4ResStrTableKey::IDS_PRC_SCREENSHOT, Config.AtExeRelativePath(Config.AtScreenshotPath(szFilename)));
	}
	else
	{
		Log(C4ResStrTableKey::IDS_PRC_SCREENSHOTERR, Config.AtExeRelativePath(Config.AtScreenshotPath(szFilename)));
	}

	// return success
	return !!fSuccess;
}

bool C4GraphicsSystem::DoSaveScreenshot(bool fSaveAll, const char *szFilename)
{
	// Fullscreen only
	if (!Application.isFullScreen) return false;
	// back surface must be present
	if (!Application.DDraw->lpBack) return false;

	const auto scale = Application.GetScale();

	// save landscape
	if (fSaveAll)
	{
		if (Viewports.empty()) return false;
		// get viewport to draw in
		const auto &pVP = Viewports.front();
		// create image large enough to hold the landcape
		C4Landscape &landscape{pVP->GetViewSection().Landscape};
		int32_t lWdt = static_cast<int32_t>(ceilf(landscape.Width * scale)), lHgt = static_cast<int32_t>(ceilf(landscape.Height * scale));
		StdBitmap bmp(lWdt, lHgt, false);
		// get backbuffer size
		int32_t bkWdt = static_cast<int32_t>(ceilf(Config.Graphics.ResX * scale)), bkHgt = static_cast<int32_t>(ceilf(Config.Graphics.ResY * scale));
		if (!bkWdt || !bkHgt) return false;
		// facet for blitting
		C4FacetEx bkFct;
		// mark background to be redrawn
		InvalidateBg();
		// backup and clear sky parallaxity
		int32_t iParX = landscape.Sky.ParX; landscape.Sky.ParX = 10;
		int32_t iParY = landscape.Sky.ParY; landscape.Sky.ParY = 10;
		// temporarily change viewport player
		int32_t iVpPlr = pVP->Player; pVP->Player = NO_OWNER;
		// blit all tiles needed
		for (int32_t iY = 0, iRealY = 0; iY < lHgt; iY += Config.Graphics.ResY, iRealY += bkHgt) for (int32_t iX = 0, iRealX = 0; iX < lWdt; iX += Config.Graphics.ResX, iRealX += bkWdt)
		{
			// get max width/height
			int32_t bkWdt2 = bkWdt, bkHgt2 = bkHgt;
			if (iRealX + bkWdt2 > lWdt) bkWdt2 -= iRealX + bkWdt2 - lWdt;
			if (iRealY + bkHgt2 > lHgt) bkHgt2 -= iRealY + bkHgt2 - lHgt;
			// update facet
			bkFct.Set(Application.DDraw->lpBack, 0, 0, static_cast<int32_t>(ceilf(bkWdt2 / scale)), static_cast<int32_t>(ceilf(bkHgt2 / scale)), iX, iY);
			// draw there
			pVP->Draw(bkFct, false);
			// render
			Application.DDraw->PageFlip(); Application.DDraw->PageFlip();
			// get output (locking primary!)
			if (Application.DDraw->lpBack->Lock())
			{
				// transfer each pixel - slooow...
				for (int32_t iY2 = 0; iY2 < bkHgt2; ++iY2)
					for (int32_t iX2 = 0; iX2 < bkWdt2; ++iX2)
						bmp.SetPixel24(iRealX + iX2, iRealY + iY2, Application.DDraw->ApplyGammaTo(Application.DDraw->lpBack->GetPixDw(iX2, iY2, false, scale)));
				// done; unlock
				Application.DDraw->lpBack->Unlock();
			}
		}
		// restore viewport player
		pVP->Player = iVpPlr;
		// restore parallaxity
		landscape.Sky.ParX = iParX;
		landscape.Sky.ParY = iParY;
		// Save bitmap to PNG file
		try
		{
			CPNGFile(szFilename, lWdt, lHgt, false).Encode(bmp.GetBytes());
		}
		catch (const std::runtime_error &e)
		{
			LogNTr(spdlog::level::err, "Could not write screenshot to PNG file: {}", e.what());
			return false;
		}
		return true;
	}
	// Save primary surface
	return Application.DDraw->lpBack->SavePNG(szFilename, false, true, false, scale);
}

void C4GraphicsSystem::DeactivateDebugOutput()
{
	ShowVertices = false;
	ShowAction = false;
	ShowCommand = false;
	ShowEntrance = false;
	ShowPathfinder = false; // allow pathfinder! - why this??
	ShowSolidMask = false;
	ShowNetstatus = false;
}

void C4GraphicsSystem::DrawHoldMessages()
{
	if (Application.isFullScreen)
	{
		if (Game.HaltCount)
		{
			Application.DDraw->TextOut("Pause", Game.GraphicsResource.FontRegular, 1.0, Application.DDraw->lpBack, Config.Graphics.ResX / 2, Config.Graphics.ResY / 2 - Game.GraphicsResource.FontRegular.GetLineHeight() * 2, CStdDDraw::DEFAULT_MESSAGE_COLOR, ACenter);
			Game.GraphicsSystem.OverwriteBg();
		}
	}
}

void C4GraphicsSystem::FlashMessage(const char *szMessage)
{
	// Store message
	SCopy(szMessage, FlashMessageText, C4MaxTitle);
	// Calculate message time
	FlashMessageTime = SLen(FlashMessageText) * 2;
	// Initial position
	FlashMessageX = -1;
	FlashMessageY = 10;
	// Upper board active: stay below upper board
	FlashMessageY += C4UpperBoard::Height();
	// More than one viewport: try to stay below portraits etc.
	if (GetViewportCount() > 1)
		FlashMessageY += 64;
	// New flash message: redraw background (might be drawing one message on top of another)
	InvalidateBg();
}

void C4GraphicsSystem::FlashMessageOnOff(const char *strWhat, bool fOn)
{
	FlashMessage(std::format("{}: {}", strWhat, LoadResStrChoice(fOn, C4ResStrTableKey::IDS_CTL_ON, C4ResStrTableKey::IDS_CTL_OFF)).c_str());
}

void C4GraphicsSystem::DrawFlashMessage()
{
	if (!FlashMessageTime) return;
	if (!Application.isFullScreen) return;
	Application.DDraw->TextOut(FlashMessageText, Game.GraphicsResource.FontRegular, 1.0, Application.DDraw->lpBack,
		(FlashMessageX == -1) ? Config.Graphics.ResX / 2 : FlashMessageX,
		(FlashMessageY == -1) ? Config.Graphics.ResY / 2 : FlashMessageY,
		CStdDDraw::DEFAULT_MESSAGE_COLOR,
		(FlashMessageX == -1) ? ACenter : ALeft);
	FlashMessageTime--;
	// Flash message timed out: redraw background
	if (!FlashMessageTime) InvalidateBg();
}

void C4GraphicsSystem::DrawHelp()
{
	if (!ShowHelp) return;
	if (!Application.isFullScreen) return;
	int32_t iX = ViewportArea.X, iY = ViewportArea.Y;
	int32_t iWdt = ViewportArea.Wdt;

	std::string text{std::format(
				// left coloumn
				"[{}]\n\n",
				// main functions
				"<c ffff00>{}</c> - {}\n"
				"<c ffff00>{}</c> - {}\n"
				"<c ffff00>{}</c> - {}\n"
				"<c ffff00>{}</c> - {}\n"
				// messages
				"\n<c ffff00>{}/{}</c> - {}\n"
				"<c ffff00>{}</c> - {}\n"
				"<c ffff00>{}</c> - {}\n"
				// irc chat
				"\n<c ffff00>{}</c> - {}\n"
				// scoreboard
				"\n<c ffff00>{}</c> - {}\n"
				// screenshots
				"\n<c ffff00>{}</c> - {}\n"
				"<c ffff00>{}</c> - {}\n",

				LoadResStr(C4ResStrTableKey::IDS_CTL_GAMEFUNCTIONS),
				GetKeyboardInputName("ToggleShowHelp"), LoadResStr(C4ResStrTableKey::IDS_CON_HELP),
				GetKeyboardInputName("MusicToggle"), LoadResStr(C4ResStrTableKey::IDS_CTL_MUSIC),
				GetKeyboardInputName("SoundToggle"), LoadResStr(C4ResStrTableKey::IDS_CTL_SOUND),
				GetKeyboardInputName("NetClientListDlgToggle"), LoadResStr(C4ResStrTableKey::IDS_DLG_NETWORK),
				GetKeyboardInputName("ChatOpen", false, 1), GetKeyboardInputName("ChatOpen", false, 0), LoadResStr(C4ResStrTableKey::IDS_CTL_SENDMESSAGE),
				GetKeyboardInputName("MsgBoardScrollUp"), LoadResStr(C4ResStrTableKey::IDS_CTL_MESSAGEBOARDBACK),
				GetKeyboardInputName("MsgBoardScrollDown"), LoadResStr(C4ResStrTableKey::IDS_CTL_MESSAGEBOARDFORWARD),
				GetKeyboardInputName("ToggleChat"), LoadResStr(C4ResStrTableKey::IDS_CTL_IRCCHAT),
				GetKeyboardInputName("ScoreboardToggle"), LoadResStr(C4ResStrTableKey::IDS_CTL_SCOREBOARD),
				GetKeyboardInputName("Screenshot"), LoadResStr(C4ResStrTableKey::IDS_CTL_SCREENSHOT),
				GetKeyboardInputName("ScreenshotEx"), LoadResStr(C4ResStrTableKey::IDS_CTL_SCREENSHOTEX)
				)};

	Application.DDraw->TextOut(text.c_str(), Game.GraphicsResource.FontRegular, 1.0, Application.DDraw->lpBack,
		iX + 128, iY + 64, CStdDDraw::DEFAULT_MESSAGE_COLOR, ALeft);

	text = std::format(
			   // right coloumn
			   // game speed
			   "\n\n<c ffff00>{}</c> - {}\n"
			   "<c ffff00>{}</c> - {}\n"
				// debug
			   "\n\n[Debug]\n\n"
			   "<c ffff00>{}</c> - {}\n"
			   "<c ffff00>{}</c> - {}\n"
			   "<c ffff00>{}</c> - {}\n"
			   "<c ffff00>{}</c> - {}\n",

			   GetKeyboardInputName("GameSpeedUp"), LoadResStr(C4ResStrTableKey::IDS_CTL_GAMESPEEDUP),
			   GetKeyboardInputName("GameSpeedDown"), LoadResStr(C4ResStrTableKey::IDS_CTL_GAMESPEEDDOWN),
			   GetKeyboardInputName("DbgModeToggle"), LoadResStr(C4ResStrTableKey::IDS_CTL_DEBUGMODE),
			   GetKeyboardInputName("DbgShowVtxToggle"), "Entrance+Vertices",
			   GetKeyboardInputName("DbgShowActionToggle"), "Actions/Commands/Pathfinder",
			   GetKeyboardInputName("DbgShowSolidMaskToggle"), "SolidMasks"
			   );

	Application.DDraw->TextOut(text.c_str(), Game.GraphicsResource.FontRegular, 1.0, Application.DDraw->lpBack,
		iX + iWdt / 2 + 64, iY + 64, CStdDDraw::DEFAULT_MESSAGE_COLOR, ALeft);
}

int32_t C4GraphicsSystem::GetAudibility(C4Section &section, int32_t iX, int32_t iY, int32_t *iPan, int32_t iAudibilityRadius)
{
	// default audibility radius
	if (!iAudibilityRadius) iAudibilityRadius = C4SoundSystem::AudibilityRadius;
	// Accumulate audibility by viewports
	int32_t iAudible = 0; *iPan = 0;
	for (const auto &cvp : Viewports | std::views::filter([&section](const auto &cvp) { return &cvp->GetViewSection() == &section; }))
	{
		auto listenerX = cvp->ViewX + cvp->ViewWdt / 2;
		auto listenerY = cvp->ViewY + cvp->ViewHgt / 2;

		const auto player = Game.Players.Get(cvp->GetPlayer());
		if (player)
		{
			auto cursor = player->ViewCursor;
			if (!cursor)
			{
				cursor = player->ViewTarget;
				if (!cursor)
				{
					cursor = player->Cursor;
				}
			}
			if (cursor)
			{
				listenerX = cursor->x;
				listenerY = cursor->y;
			}
		}

		iAudible = (std::max)(iAudible,
			BoundBy<int32_t>(100 - 100 * Distance(listenerX, listenerY, iX, iY) / C4SoundSystem::AudibilityRadius, 0, 100));
		*iPan += (iX - (cvp->ViewX + cvp->ViewWdt / 2)) / 5;
	}
	*iPan = BoundBy<int32_t>(*iPan, -100, 100);
	return iAudible;
}

void C4GraphicsSystem::SetGamma(uint32_t dwClr1, uint32_t dwClr2, uint32_t dwClr3, int32_t iRampIndex)
{
	// No gamma effects
	if (Config.Graphics.DisableGamma) return;
	if (iRampIndex < 0 || iRampIndex >= C4MaxGammaRamps) return;
	// turn ramp index into array offset
	iRampIndex *= 3;
	// set array members
	dwGamma[iRampIndex + 0] = dwClr1;
	dwGamma[iRampIndex + 1] = dwClr2;
	dwGamma[iRampIndex + 2] = dwClr3;
	// mark gamma ramp to be recalculated
	fSetGamma = true;
}

void C4GraphicsSystem::ApplyGamma()
{
	// No gamma effects
	if (Config.Graphics.DisableGamma) return;
	// calculate color channels by adding the difference between the gamma ramps to their normals
	int32_t ChanOff[3];
	uint32_t Gamma[3];
	const int32_t DefChanVal[3] = { 0x00, 0x80, 0xff };
	// calc offset for curve points
	for (int32_t iCurve = 0; iCurve < 3; ++iCurve)
	{
		std::fill(ChanOff, std::end(ChanOff), 0);
		// ...channels...
		for (int32_t iChan = 0; iChan < 3; ++iChan)
			// ...ramps...
			for (int32_t iRamp = 0; iRamp < C4MaxGammaRamps; ++iRamp)
				// add offset
				ChanOff[iChan] += uint8_t(dwGamma[iRamp * 3 + iCurve] >> (16 - iChan * 8)) - DefChanVal[iCurve];
		// calc curve point
		Gamma[iCurve] = C4RGB(BoundBy<int32_t>(DefChanVal[iCurve] + ChanOff[0], 0, 255), BoundBy<int32_t>(DefChanVal[iCurve] + ChanOff[1], 0, 255), BoundBy<int32_t>(DefChanVal[iCurve] + ChanOff[2], 0, 255));
	}
	// set gamma
	Application.DDraw->SetGamma(Gamma[0], Gamma[1], Gamma[2]);
}

bool C4GraphicsSystem::ToggleShowNetStatus()
{
	ShowNetstatus = !ShowNetstatus;
	return true;
}

bool C4GraphicsSystem::ToggleShowVertices()
{
	if (!Game.DebugMode && !Console.Active) { FlashMessage(LoadResStr(C4ResStrTableKey::IDS_MSG_NODEBUGMODE)); return false; }
	Toggle(ShowVertices);
	Toggle(ShowEntrance); // vertices and entrance now toggled together
	FlashMessageOnOff("Entrance+Vertices", ShowVertices || ShowEntrance);
	return true;
}

bool C4GraphicsSystem::ToggleShowAction()
{
	if (!Game.DebugMode && !Console.Active) { FlashMessage(LoadResStr(C4ResStrTableKey::IDS_MSG_NODEBUGMODE)); return false; }
	if (!(ShowAction || ShowCommand || ShowPathfinder))
	{
		ShowAction = true; FlashMessage("Actions");
	}
	else if (ShowAction)
	{
		ShowAction = false; ShowCommand = true; FlashMessage("Commands");
	}
	else if (ShowCommand)
	{
		ShowCommand = false; ShowPathfinder = true; FlashMessage("Pathfinder");
	}
	else if (ShowPathfinder)
	{
		ShowPathfinder = false; FlashMessageOnOff("Actions/Commands/Pathfinder", false);
	}
	return true;
}

bool C4GraphicsSystem::ToggleShowSolidMask()
{
	if (!Game.DebugMode && !Console.Active) { FlashMessage(LoadResStr(C4ResStrTableKey::IDS_MSG_NODEBUGMODE)); return false; }
	Toggle(ShowSolidMask);
	FlashMessageOnOff("SolidMasks", !!ShowSolidMask);
	return true;
}

bool C4GraphicsSystem::ToggleShowHelp()
{
	Toggle(ShowHelp);
	// Turned off? Invalidate background.
	if (!ShowHelp) InvalidateBg();
	return true;
}

bool C4GraphicsSystem::ViewportNextPlayer()
{
	// safety: switch valid?
	if ((!Game.C4S.Head.Film || !Game.C4S.Head.Replay) && !Game.GraphicsSystem.GetViewport(NO_OWNER)) return false;
	// do switch then
	if (Viewports.empty()) return false;
	Viewports.front()->NextPlayer();
	return true;
}

bool C4GraphicsSystem::FreeScroll(C4Vec2D vScrollBy)
{
	// safety: move valid?
	if ((!Game.C4S.Head.Replay || !Game.C4S.Head.Film) && !Game.GraphicsSystem.GetViewport(NO_OWNER)) return false;
	if (Viewports.empty()) return false;
	const auto &vp = Viewports.front();
	// move then (old static code crap...)
	static int32_t vp_vx = 0; static int32_t vp_vy = 0;
	int32_t dx = vScrollBy.x; int32_t dy = vScrollBy.y;
	if (!MostRecentScrolling.Elapsed())
	{
		dx += vp_vx; dy += vp_vy;
	}
	vp_vx = dx; vp_vy = dy;
	vp->ViewX += dx; vp->ViewY += dy;
	MostRecentScrolling.Reset();
	return true;
}
