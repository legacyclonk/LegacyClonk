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
	virtual bool CreateDirectDraw();

public:
	virtual CStdDDrawContext *GetMainContext() override { return nullptr; }
	virtual bool PageFlip(RECT *pSrcRt = nullptr, RECT *pDstRt = nullptr, CStdWindow *pWindow = nullptr) { return true; }
	virtual bool BeginScene() { return true; }
	virtual void EndScene() {}
	virtual int GetEngine() { return GFXENGN_NOGFX; }
	virtual bool UpdateClipper() { return true; }
	virtual bool OnResolutionChanged() { return true; }
	virtual bool PrepareRendering(CSurface *) { return true; }
	virtual void FillBG(uint32_t dwClr = 0) {}
	virtual void PerformBlt(CBltData &, CTexRef *, uint32_t, bool, bool) {}
	virtual void DrawLineDw(CSurface *, float, float, float, float, uint32_t) {}
	virtual void DrawQuadDw(CSurface *, int *, uint32_t, uint32_t, uint32_t, uint32_t) {}
	virtual void DrawPixInt(CSurface *, float, float, uint32_t) {}
	virtual bool ApplyGammaRamp(CGammaControl &, bool) { return true; }
	virtual bool SaveDefaultGammaRamp(CStdWindow *) { return true; }
	virtual void SetTexture() {}
	virtual void ResetTexture() {}
	virtual bool RestoreDeviceObjects() { return true; }
	virtual bool InvalidateDeviceObjects() { return true; }
	virtual bool DeviceReady() { return true; }
	virtual bool CreatePrimarySurfaces();
	virtual CStdShader *CreateShader(CStdShader::Type, CStdDDraw::ShaderLanguage, const std::string &) override { return nullptr; }
	virtual CStdShaderProgram *CreateShaderProgram() override { return nullptr; }
	virtual void ShaderProgramSet(DrawMode mode, CStdShaderProgram *shaderProgram) override {}
	virtual void ShaderProgramsCleared() override {}
};
