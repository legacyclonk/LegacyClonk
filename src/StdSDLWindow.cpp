/* by survivor/qualle, 2006 */

/* A wrapper class to OS dependent event and window interfaces, SDL version */

#include <Standard.h>
#include <StdWindow.h>
#include <StdGL.h>
#include <StdDDraw2.h>
#include <StdFile.h>
#include <StdBuf.h>

/* CStdWindow */

CStdWindow::CStdWindow ():
	Active(false)
{
}

CStdWindow::~CStdWindow () {
	Clear();
}

// Only set title.
// FIXME: Read from application bundle on the Mac.

CStdWindow * CStdWindow::Init(CStdApp * pApp) {
	return Init(pApp, STD_PRODUCT);
}

CStdWindow * CStdWindow::Init(CStdApp * pApp, const char * Title, CStdWindow * pParent, bool HideCursor) {
	Active = true;
    SetTitle(Title);
	return this;
}

void CStdWindow::Clear() {}

bool CStdWindow::StorePosition(const char *, const char *, bool) { return true; }

bool CStdWindow::RestorePosition(const char *, const char *, bool) { return true; }

// Window size is automatically managed by CStdApp's display mode management.
// Just remember the size for others to query.

bool CStdWindow::GetSize(RECT * pRect) {
 	pRect->left = pRect->top = 0;
	pRect->right = width, pRect->bottom = height;
	return true;
}

bool CStdWindow::SetFullScreen(bool fFullscreen, int BPP) {
	this->BPP = BPP; this->fFullscreen=fFullscreen;
}

void CStdWindow::SetSize(unsigned int X, unsigned int Y) {
	width = X, height = Y;
	SDL_SetVideoMode(X, Y, BPP, SDL_OPENGL | (fFullscreen ? SDL_FULLSCREEN : 0));
	SDL_ShowCursor(SDL_DISABLE);
}

void CStdWindow::SetTitle(const char * Title) {
	SDL_WM_SetCaption(Title, 0);
}

void CStdWindow::FlashWindow() {
#ifdef __APPLE__
    void requestUserAttention();
    requestUserAttention();
#endif	
}
