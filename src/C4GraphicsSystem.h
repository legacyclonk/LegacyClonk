/*
 * LegacyClonk
 *
 * Copyright (c) 1998-2000, Matthes Bender (RedWolf Design)
 * Copyright (c) 2017-2019, The LegacyClonk Team and contributors
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

#pragma once

#include <C4FacetEx.h>
#include <C4MessageBoard.h>
#include <C4UpperBoard.h>
#include <C4Video.h>
#include <C4Shape.h>

class C4Game;
class C4LoaderScreen;
extern C4Game Game;

class C4GraphicsSystem
{
public:
	C4GraphicsSystem();
	~C4GraphicsSystem();
	C4MessageBoard MessageBoard;
	C4UpperBoard UpperBoard;
	int32_t iRedrawBackground;
	bool ShowHelp;
	bool ShowVertices;
	bool ShowAction;
	bool ShowCommand;
	bool ShowEntrance;
	bool ShowPathfinder;
	bool ShowNetstatus;
	bool ShowSolidMask;
	bool fSetPalette;
	uint32_t dwGamma[C4MaxGammaRamps * 3]; // gamma ramps
	bool fSetGamma; // must gamma ramp be reassigned?
	C4Video Video;
	C4LoaderScreen *pLoaderScreen;
	void Default();
	void Clear();
	bool StartDrawing();
	void FinishDrawing();
	void Execute();
	void FlashMessage(const char *szMessage);
	void FlashMessageOnOff(const char *strWhat, bool fOn);
	void DeactivateDebugOutput();
	void MouseMove(int32_t iButton, int32_t iX, int32_t iY, uint32_t dwKeyParam, class C4Viewport *pVP); // pVP specified for console mode viewports only
	void SetMouseInGUI(bool fInGUI, bool fByMouse);
	void SortViewportsByPlayerControl();
	void ClearPointers(C4Object *pObj);
	void RecalculateViewports();
	bool Init();
	bool InitLoaderScreen(const char *szLoaderSpec, bool fDrawBlackScreenFirst);
	void EnableLoaderDrawing(); // reset black screen loader flag
	bool SaveScreenshot(bool fSaveAll);
	bool DoSaveScreenshot(bool fSaveAll, const char *szFilename);
	bool SetPalette();
	bool CreateViewport(int32_t iPlayer, bool fSilent);
	bool CloseViewport(int32_t iPlayer, bool fSilent);
	int32_t GetAudibility(int32_t iX, int32_t iY, int32_t *iPan, int32_t iAudibilityRadius = 0);
	int32_t GetViewportCount();
	C4Viewport *GetViewport(int32_t iPlayer);
	C4Viewport *GetFirstViewport() { return FirstViewport; }
	inline void InvalidateBg() { iRedrawBackground = 2; }
	inline void OverwriteBg() { InvalidateBg(); }
	void SetGamma(uint32_t dwClr1, uint32_t dwClr2, uint32_t dwClr3, int32_t iRampIndex); // set gamma ramp
	void ApplyGamma(); // apply gamma ramp to ddraw
	bool CloseViewport(C4Viewport *cvp);
#ifdef _WIN32
	C4Viewport *GetViewport(HWND hwnd);
	bool RegisterViewportClass(HINSTANCE hInst);
#endif

protected:
	C4Viewport *FirstViewport;
	bool fViewportClassRegistered;
	C4Facet ViewportArea;
#ifdef C4ENGINE
	C4RectList BackgroundAreas; // rectangles covering background without viewports in fullscreen
#endif
	char FlashMessageText[C4MaxTitle + 1];
	int32_t FlashMessageTime, FlashMessageX, FlashMessageY;
	void DrawHelp();
	void DrawFlashMessage();
	void DrawHoldMessages();
	void DrawFullscreenBackground();
	void ClearFullscreenBackground();
	void MouseMoveToViewport(int32_t iButton, int32_t iX, int32_t iY, uint32_t dwKeyParam);

public:
	bool ToggleShowSolidMask();
	bool ToggleShowNetStatus();
	bool ToggleShowVertices();
	bool ToggleShowAction();
	bool ViewportNextPlayer();
	bool ToggleShowHelp();

	bool FreeScroll(C4Vec2D vScrollBy); // key callback: Scroll ownerless viewport by some offset
};
