/*
 * LegacyClonk
 *
 * Copyright (c) RedWolf Design
 * Copyright (c) 2017-2020, The LegacyClonk Team and contributors
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

#include <C4Game.h>
#include <C4Config.h>
#include <C4Application.h>

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
	if (Config.Graphics.UpperBoard == Hide) return;
	// Make the time strings
	FormatWithNull(cTimeString, "{:02}:{:02}:{:02}", Game.Time / 3600, (Game.Time % 3600) / 60, Game.Time % 60);
	time_t t = time(nullptr); strftime(cTimeString2, sizeof(cTimeString2), "[%H:%M:%S]", localtime(&t));
	Draw(Output);
}

void C4UpperBoard::Draw(C4Facet &cgo)
{
	if (!cgo.Surface) return;
	const auto mode = Config.Graphics.UpperBoard;
	if (mode != Mini)
	{
		// Background
		Application.DDraw->BlitSurfaceTile(Game.GraphicsResource.fctUpperBoard.Surface, Output.Surface, 0, 0, Output.Wdt, Output.Hgt, 0, 0, false, mode == Small ? 0.5f : 1.f);
		// Logo
		C4Facet cgo2;
		float fLogoZoom;
		if (static_cast<float>(Game.GraphicsResource.fctLogo.Wdt) / Game.GraphicsResource.fctLogo.Hgt != 3.f)
		{
			// old logo factor
			fLogoZoom = 0.25f;
		}
		else
		{
			fLogoZoom = 0.21f;
		}
		fLogoZoom *= 960.f / Game.GraphicsResource.fctLogo.Wdt;
		if (mode == Small) fLogoZoom *= 8.f / 15.f;
		cgo2.Set(cgo.Surface, static_cast<int32_t>(cgo.Wdt / 2 - (Game.GraphicsResource.fctLogo.Wdt / 2) * fLogoZoom), 0,
			static_cast<int32_t>(Game.GraphicsResource.fctLogo.Wdt * fLogoZoom), static_cast<int32_t>(Game.GraphicsResource.fctLogo.Hgt * fLogoZoom));
		Game.GraphicsResource.fctLogo.Draw(cgo2);
	}

	// Right text sections
	int32_t iRightOff = 1;
	// Playing time
	Application.DDraw->TextOut(cTimeString, Game.GraphicsResource.FontRegular, 1.0, cgo.Surface, Output.X + Output.Wdt - (iRightOff++) * TextWidth - 10, TextYPosition, 0xFFFFFFFF);
	// Clock
	if (Config.Graphics.ShowClock)
		Application.DDraw->TextOut(cTimeString2, Game.GraphicsResource.FontRegular, 1.0, cgo.Surface, Output.X + Output.Wdt - (iRightOff++) * TextWidth - 30, TextYPosition, 0xFFFFFFFF);
	// FPS
	if (Config.General.FPS)
	{
		FormatWithNull(cTimeString, "{} FPS", Game.FPS);
		Application.DDraw->TextOut(cTimeString, Game.GraphicsResource.FontRegular, 1.0, cgo.Surface, Output.X + Output.Wdt - (iRightOff++) * TextWidth - 30, TextYPosition, 0xFFFFFFFF);
	}
	if (mode != Mini)
	{
		// Scenario title
		Application.DDraw->TextOut(Game.Parameters.ScenarioTitle.getData(), Game.GraphicsResource.FontRegular, 1.0, cgo.Surface, 10, cgo.Hgt / 2 - Game.GraphicsResource.FontRegular.GetLineHeight() / 2, 0xFFFFFFFF);
	}
}

int C4UpperBoard::Height()
{
	const auto mode = Config.Graphics.UpperBoard;
	if (mode == Hide || mode == Mini) return 0;
	if (mode == Small) return C4UpperBoardHeight / 2;
	return C4UpperBoardHeight;
}

void C4UpperBoard::Init(C4Facet &cgo, C4Facet &messageBoardCgo)
{
	FormatWithNull(cTimeString, "{:02}:{:02}:{:02}", Game.Time / 3600, (Game.Time % 3600) / 60, Game.Time % 60);
	TextWidth = Game.GraphicsResource.FontRegular.GetTextWidth(cTimeString);
	if (Config.Graphics.UpperBoard == Mini)
	{
		const auto xStart = messageBoardCgo.Wdt - TextWidth * 3 - 30;
		Output.Set(messageBoardCgo.Surface, xStart + messageBoardCgo.X, messageBoardCgo.Y, messageBoardCgo.Wdt - xStart, messageBoardCgo.Hgt);
		messageBoardCgo.Wdt -= Output.Wdt;

		TextYPosition = Output.Y + Output.Hgt / 2 - Game.GraphicsResource.FontRegular.GetLineHeight() / 2;
	}
	else
	{
		// Save facet
		Output = cgo;
		if (!Game.GraphicsResource.fctUpperBoard.Surface) return;
		// in newgfx, the upperboard may be larger and overlap the scene
		Output.Hgt = (std::max)(Output.Hgt, Config.Graphics.UpperBoard == Small ? Game.GraphicsResource.fctUpperBoard.Hgt / 2 : Game.GraphicsResource.fctUpperBoard.Hgt);
		// surface should not be too small
		Game.GraphicsResource.fctUpperBoard.EnsureSize(128, Output.Hgt);
		// Generate textposition
		TextYPosition = cgo.Hgt / 2 - Game.GraphicsResource.FontRegular.GetLineHeight() / 2;
	}
}
