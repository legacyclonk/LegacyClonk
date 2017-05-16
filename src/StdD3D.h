/* by Sven2, 2001 */

/* Direct3D implementation of NewGfx */

#pragma once

#ifdef USE_DIRECTX

#include <d3d9.h>
#include <StdDDraw2.h>

// default Clonk vertex format
struct C4VERTEX
{
	FLOAT x, y, z, rhw; // transformed vertex pos
	FLOAT tu, tv;       // texture offsets
};

// vertex format for solid blits
struct C4CLRVERTEX
{
	FLOAT x, y, z, rhw; // transformed vertex pos
	DWORD color;        // blit color
};

// vertex format for ColorByOwner-blits
struct C4CTVERTEX
{
	FLOAT x, y, z, rhw; // transformed vertex pos
	DWORD color;        // overlay color
	FLOAT tu, tv;       // texture offsets
};

#define D3DFVF_C4VERTEX    (D3DFVF_XYZRHW |                  D3DFVF_TEX1)
#define D3DFVF_C4CLRVERTEX (D3DFVF_XYZRHW | D3DFVF_DIFFUSE)
#define D3DFVF_C4CTVERTEX  (D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_TEX1)

// direct draw encapsulation
class CStdD3D : public CStdDDraw
{
public:
	CStdD3D(bool fSoftware);
	~CStdD3D();

protected:
	IDirect3D9       *lpD3D;
	IDirect3DDevice9 *lpDevice;
	IDirect3DVertexBuffer9 *pVB;       // prepared vertex buffer for blitting
	IDirect3DVertexBuffer9 *pVBClr;    // prepared vertex buffer for drawing in solid color
	IDirect3DVertexBuffer9 *pVBClrTex; // prepared vertex buffer for blitting iwth color/tex-modulation
	C4VERTEX    bltVertices[8];    // prepared vertex data; need to insert x/y and u/v
	C4CLRVERTEX clrVertices[8];    // prepared vertex data; need to insert x/y and color
	C4CTVERTEX  bltClrVertices[8]; // prepared vertex data; need to insert x/y, color and u/v
	IDirect3DStateBlock9 *bltState[3];       // saved state block for blitting (0: copy; 1: blit; 2: blit additive)
	IDirect3DStateBlock9 *bltBaseState[4];   // saved state block for blitting with a base face (0: normal; 1: additive; 2: mod2; 3: mod2+additive)
	IDirect3DStateBlock9 *drawSolidState[2]; // saved state block for drawing in solid color (0: normal; 1: additive)
	IDirect3DStateBlock9 *savedState; // state block to backup current state
	D3DDISPLAYMODE dspMode;
	D3DPRESENT_PARAMETERS d3dpp; // device present parameters
	D3DFORMAT dwSurfaceType; // surface format for new textures
	D3DFORMAT PrimarySrfcFormat; // surace format of primary surface
	bool fSoftware; // software rendering
	struct
	{
		BITMAPINFOHEADER bmiHeader; // surface bits as bitmap bits info
		RGBQUAD bmiColors[3];
	} sfcBmpInfo;
	bool SceneOpen; // set if a scene has begun

public:
	// General
	void Clear();
	void Default();
	bool PageFlip(RECT *pSrcRt = nullptr, RECT *pDstRt = nullptr, CStdWindow *pWindow = nullptr);
	bool BeginScene(); // prepare device for drawing
	void EndScene(); // prepare device for surface locking, flipping etc.
	virtual int GetEngine() { return fSoftware ? 2 : 0; } // get indexed engine
	void TaskOut(); // user taskswitched the app away
	void TaskIn(); // user tasked back
	virtual bool OnResolutionChanged(); // reinit window for new resolution

	// Clipper
	bool UpdateClipper(); // set current clipper to render target

	// Surface
	bool PrepareRendering(SURFACE sfcToSurface); // check if/make rendering possible to given surface

	// Blit
	void PerformBlt(CBltData &rBltData, CTexRef *pTex, uint32_t dwModClr, bool fMod2, bool fExact);
	bool BlitTex2Window(CTexRef *pTexRef, HDC hdcTarget, RECT &rtFrom, RECT &rtTo);
	bool BlitSurface2Window(SURFACE sfcSource, int fX, int fY, int fWdt, int fHgt, HWND hWnd, int tX, int tY, int tWdt, int tHgt);
	void FillBG(uint32_t dwClr = 0);

	// Drawing
	void DrawQuadDw(SURFACE sfcTarget, int *ipVtx, uint32_t dwClr1, uint32_t dwClr2, uint32_t dwClr3, uint32_t dwClr4);
	void DrawLineDw(SURFACE sfcTarget, float x1, float y1, float x2, float y2, uint32_t dwClr);
	void DrawPixInt(SURFACE sfcDest, float tx, float ty, uint32_t dwCol);
	void DrawPixPrimary(SURFACE sfcDest, int tx, int ty, uint32_t dwCol);

	// Gamma
	virtual bool ApplyGammaRamp(CGammaControl &ramp, bool fForce);
	virtual bool SaveDefaultGammaRamp(CStdWindow *pWindow);

	// device objects
	bool InitDeviceObjects();       // init device dependent objects
	bool RestoreDeviceObjects();    // restore device dependent objects
	bool InvalidateDeviceObjects(); // free device dependent objects
	bool DeleteDeviceObjects();     // free device dependent objects
	bool StoreStateBlock();
	void SetTexture();
	void ResetTexture();
	bool RestoreStateBlock();
	bool DeviceReady() { return !!lpDevice; }

	IDirect3DStateBlock9 *CreateStateBlock(bool fTransparent, bool fSolid, bool fBaseTex, bool fAdditive, bool fMod2); // capture state blocks for blitting

protected:
	bool FindDisplayMode(unsigned int iXRes, unsigned int iYRes, unsigned int iColorDepth, unsigned int iMonitor);
	virtual bool CreatePrimarySurfaces(bool Fullscreen, int iColorDepth, unsigned int iMonitor);
	bool SetOutputAdapter(unsigned int iMonitor);
	bool CreateDirectDraw();

	friend class CSurface;
	friend class CTexRef;
	friend class CPattern;
};

// Global D3D access pointer
extern CStdD3D *pD3D;

#endif // ifdef USE_DIRECTX
