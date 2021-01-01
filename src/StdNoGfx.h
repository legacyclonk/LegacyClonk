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

/* Implemention of NewGfx - without gfx */

#pragma once

#include <StdDDraw2.h>

class CStdNoGfx : public CStdDDraw
{
public:
	CStdNoGfx();
	virtual ~CStdNoGfx();
	virtual bool CreateDirectDraw() override;

public:
	virtual bool PageFlip(RECT *pSrcRt = nullptr, RECT *pDstRt = nullptr, CStdWindow *pWindow = nullptr) override { return true; }
	virtual int GetEngine() override { return GFXENGN_NOGFX; }
	virtual bool UpdateClipper() override { return true; }
	virtual bool OnResolutionChanged() override { return true; }
	virtual bool PrepareRendering(C4Surface *) override { return true; }
	virtual void FillBG(uint32_t dwClr = 0) override {}
	virtual void PerformBlt(CBltData &, C4TexRef *, uint32_t, bool, bool) override {}
	virtual void DrawLineDw(C4Surface *, float, float, float, float, uint32_t) override {}
	virtual void DrawQuadDw(C4Surface *, int *, uint32_t, uint32_t, uint32_t, uint32_t) override {}
	virtual void DrawPixInt(C4Surface *, float, float, uint32_t) override {}
	virtual bool ApplyGammaRamp(CGammaControl &, bool) override { return true; }
	virtual bool SaveDefaultGammaRamp(CStdWindow *) override { return true; }
	virtual void SetTexture() override {}
	virtual void ResetTexture() override {}
	virtual bool RestoreDeviceObjects() override { return true; }
	virtual bool InvalidateDeviceObjects() override { return true; }
	virtual bool DeviceReady() override { return true; }
	virtual bool CreatePrimarySurfaces() override;
};
