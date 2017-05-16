#include <Standard.h>
#include <StdNoGfx.h>

BOOL CStdNoGfx::CreateDirectDraw()
{
	Log("  Graphics disabled");
	return TRUE;
}

CStdNoGfx::CStdNoGfx()
{
	Default();
}

CStdNoGfx::~CStdNoGfx()
{
	delete lpPrimary; lpPrimary = NULL;
	Clear();
}

bool CStdNoGfx::CreatePrimarySurfaces(BOOL Fullscreen, int iColorDepth, unsigned int iMonitor)
{
	// Save back color depth
	byByteCnt = iColorDepth / 8;
	// Create dummy surface
	lpPrimary = lpBack = new CSurface();
	return true;
}
