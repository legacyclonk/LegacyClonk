/* Implemention of NewGfx - without gfx */

#pragma once

#include <StdDDraw2.h>

class CStdNoGfx : public CStdDDraw
{
public:
	CStdNoGfx();
	virtual ~CStdNoGfx();
	virtual BOOL CreateDirectDraw();

public:
	virtual bool PageFlip(RECT *pSrcRt = nullptr, RECT *pDstRt = nullptr, CStdWindow *pWindow = nullptr) { return true; }
	virtual bool BeginScene() { return true; }
	virtual void EndScene() {}
	virtual int GetEngine() { return GFXENGN_NOGFX; }
	virtual void TaskOut() {}
	virtual void TaskIn() {}
	virtual bool UpdateClipper() { return true; }
	virtual bool OnResolutionChanged() { return true; }
	virtual bool PrepareRendering(SURFACE) { return true; }
	virtual void FillBG(DWORD dwClr = 0) {}
	virtual void PerformBlt(CBltData &, CTexRef *, DWORD, bool, bool) {}
	virtual void DrawLineDw(SURFACE, float, float, float, float, DWORD) {}
	virtual void DrawQuadDw(SURFACE, int *, DWORD, DWORD, DWORD, DWORD) {}
	virtual void DrawPixInt(SURFACE, float, float, DWORD) {}
	virtual bool ApplyGammaRamp(CGammaControl &, bool) { return true; }
	virtual bool SaveDefaultGammaRamp(CStdWindow *) { return true; }
	virtual bool StoreStateBlock() { return true; }
	virtual void SetTexture() {}
	virtual void ResetTexture() {}
	virtual bool RestoreStateBlock() { return true; }
	virtual bool InitDeviceObjects() { return true; }
	virtual bool RestoreDeviceObjects() { return true; }
	virtual bool InvalidateDeviceObjects() { return true; }
	virtual bool DeleteDeviceObjects() { return true; }
	virtual bool DeviceReady() { return true; }
	virtual bool CreatePrimarySurfaces(BOOL, int, unsigned int);
	virtual bool SetOutputAdapter(unsigned int) { return true; }
};
