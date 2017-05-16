#include <Standard.h>
#include <StdNoGfx.h>

bool CStdNoGfx::CreateDirectDraw()
{
	Log("  Graphics disabled");
	return true;
}

CStdNoGfx::CStdNoGfx()
{
	Default();
}

CStdNoGfx::~CStdNoGfx()
{
	delete lpPrimary; lpPrimary = nullptr;
	Clear();
}

bool CStdNoGfx::CreatePrimarySurfaces(bool Fullscreen, int iColorDepth, unsigned int iMonitor)
{
	// Save back color depth
	byByteCnt = iColorDepth / 8;
	// Create dummy surface
	lpPrimary = lpBack = new CSurface();
	return true;
}
