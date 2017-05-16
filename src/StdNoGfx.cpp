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

bool CStdNoGfx::CreatePrimarySurfaces(bool Fullscreen, unsigned int iMonitor)
{
	// Create dummy surface
	lpPrimary = lpBack = new CSurface();
	return true;
}
