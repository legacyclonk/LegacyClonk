/*
 * LegacyClonk
 *
 * Copyright (c) RedWolf Design
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

#include <C4Include.h>
#include <C4UpperBoard.h>

#ifndef BIG_C4INCLUDE
#include <C4Game.h>
#include <C4Config.h>
#include <C4Application.h>
#endif

C4UpperBoard::C4UpperBoard()
{
	Default();
}

C4UpperBoard::~C4UpperBoard()
{
	Clear();
}

void C4UpperBoard::Default() {}

void C4UpperBoard::Clear() {}

void C4UpperBoard::Execute()
{
	if (!Config.Graphics.UpperBoard) return;
	// Make the time strings
	sprintf(cTimeString, "%02d:%02d:%02d", Game.Time / 3600, (Game.Time % 3600) / 60, Game.Time % 60);
	time_t t = time(0); strftime(cTimeString2, sizeof(cTimeString2), "[%H:%M:%S]", localtime(&t));
	Draw(Output);
}

void C4UpperBoard::Draw(C4Facet &cgo)
{
	if (!cgo.Surface) return;
	// Background
	Application.DDraw->BlitSurfaceTile(Game.GraphicsResource.fctUpperBoard.Surface, Output.Surface, 0, 0, Output.Wdt, Output.Hgt);
	// Logo
	C4Facet cgo2;
	float fLogoZoom = 0.75f;
	cgo2.Set(cgo.Surface, (int32_t)(cgo.Wdt / 2 - (Game.GraphicsResource.fctLogo.Wdt / 2) * fLogoZoom), 0,
		(int32_t)(Game.GraphicsResource.fctLogo.Wdt * fLogoZoom), (int32_t)(Game.GraphicsResource.fctLogo.Hgt * fLogoZoom));
	Game.GraphicsResource.fctLogo.Draw(cgo2);
	// Right text sections
	int32_t iRightOff = 1;
	// Playing time
	Application.DDraw->TextOut(cTimeString, Game.GraphicsResource.FontRegular, 1.0, cgo.Surface, Config.Graphics.ResX - (iRightOff++) * TextWidth - 10, TextYPosition, 0xFFFFFFFF);
	// Clock
	if (Config.Graphics.ShowClock)
		Application.DDraw->TextOut(cTimeString2, Game.GraphicsResource.FontRegular, 1.0, cgo.Surface, Config.Graphics.ResX - (iRightOff++) * TextWidth - 30, TextYPosition, 0xFFFFFFFF);
	// FPS
	if (Config.General.FPS)
	{
		sprintf(cTimeString, "%d FPS", Game.FPS);
		Application.DDraw->TextOut(cTimeString, Game.GraphicsResource.FontRegular, 1.0, cgo.Surface, Config.Graphics.ResX - (iRightOff++) * TextWidth - 30, TextYPosition, 0xFFFFFFFF);
	}
	// Scenario title
	Application.DDraw->TextOut(Game.ScenarioTitle.getData(), Game.GraphicsResource.FontRegular, 1.0, cgo.Surface, 10, cgo.Hgt / 2 - Game.GraphicsResource.FontRegular.GetLineHeight() / 2, 0xFFFFFFFF);
}

void C4UpperBoard::Init(C4Facet &cgo)
{
	// Save facet
	Output = cgo;
	if (!Game.GraphicsResource.fctUpperBoard.Surface) return;
	// in newgfx, the upperboard may be larger and overlap the scene
	Output.Hgt = (std::max)(Output.Hgt, Game.GraphicsResource.fctUpperBoard.Hgt);
	// surface should not be too small
	Game.GraphicsResource.fctUpperBoard.EnsureSize(128, Output.Hgt);
	// Generate textposition
	sprintf(cTimeString, "%02d:%02d:%02d", Game.Time / 3600, (Game.Time % 3600) / 60, Game.Time % 60);
	TextWidth = Game.GraphicsResource.FontRegular.GetTextWidth(cTimeString);
	TextYPosition = cgo.Hgt / 2 - Game.GraphicsResource.FontRegular.GetLineHeight() / 2;
}
