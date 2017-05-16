/* by Sven2, 2001 */

/* Direct3D implementation of NewGfx */

#if defined(USE_DIRECTX) && !defined(INC_STDD3D)
#define INC_STDD3D

// debug memmgmt off
#ifdef _DEBUG
#ifdef _MSC_VER
#undef new
#endif // _MSC_VER
#endif // _DEBUG

#include <d3d8.h>
#include <d3dx8tex.h>
#include <StdDDraw2.h>

// debug memmgmt on
#ifdef _DEBUG
#ifdef _MSC_VER
#define new new(_NORMAL_BLOCK, __FILE__, __LINE__)
#endif // _MSC_VER
#endif // _DEBUG

// check version
#if (DIRECT3D_VERSION > 0x0800)
#error "Using DirectX > 8.0 headers! Program won't run on computers using DX 8.0!"
#endif

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

#define D3DFVF_C4VERTEX (D3DFVF_XYZRHW|D3DFVF_TEX1)
#define D3DFVF_C4CLRVERTEX (D3DFVF_XYZRHW|D3DFVF_DIFFUSE)
#define D3DFVF_C4CTVERTEX (D3DFVF_XYZRHW|D3DFVF_DIFFUSE|D3DFVF_TEX1)

typedef C4VERTEX C4VERTEXQUAD[4];
typedef C4CLRVERTEX C4CLRVERTEXQUAD[4];
typedef C4CTVERTEX C4CTVERTEXQUAD[4];

// direct draw encapsulation
class CStdD3D : public CStdDDraw
  {
  public:
    CStdD3D(bool fSoftware);
    ~CStdD3D();
  protected:
    IDirect3D8          *lpD3D;
    IDirect3DDevice8    *lpDevice;
    IDirect3DVertexBuffer8 *pVB;      // prepared vertex buffer for blitting
    IDirect3DVertexBuffer8 *pVBClr;   // prepared vertex buffer for drawing in solid color
    IDirect3DVertexBuffer8 *pVBClrTex;// prepared vertex buffer for blitting iwth color/tex-modulation
    C4VERTEX bltVertices[8];          // prepared vertex data; need to insert x/y and u/v
    C4CLRVERTEX clrVertices[8];       // prepared vertex data; need to insert x/y and color
    C4CTVERTEX bltClrVertices[8];     // prepared vertex data; need to insert x/y, color and u/v
    DWORD bltState[3];                // saved state block for blitting (0: copy; 1: blit; 2: blit additive)
    DWORD bltBaseState[4];            // saved state block for blitting with a base face (0: normal; 1: additive; 2: mod2; 3: mod2+additive)
    DWORD drawSolidState[2];          // saved state block for drawing in solid color (0: normal; 1: additive)
    DWORD dwSavedState;               // state block to backup current state
    D3DVIEWPORT8        WindowClipper;
    D3DDISPLAYMODE      dspMode;
    D3DPRESENT_PARAMETERS d3dpp;      // device present parameters
    D3DFORMAT           dwSurfaceType;// surface format for new textures
    D3DFORMAT           PrimarySrfcFormat;// surace format of primary surface
    bool                fSoftware;        // software rendering
	struct
	{
		BITMAPINFOHEADER bmiHeader;		// surface bits as bitmap bits info
		RGBQUAD bmiColors[3];
	} sfcBmpInfo;
		bool SceneOpen;									// set if a scene has begun
  public:
    // General
    void Clear();
    void Default();
    bool PageFlip(RECT *pSrcRt=NULL, RECT *pDstRt=NULL, CStdWindow * pWindow = NULL);
    bool BeginScene(); // prepare device for drawing
    void EndScene();   // prepare device for surface locking, flipping etc.
    virtual int GetEngine() { return fSoftware ? 2 : 0; }   // get indexed engine
    void TaskOut(); // user taskswitched the app away
    void TaskIn();  // user tasked back
		virtual bool OnResolutionChanged(); // reinit window for new resolution
    // Clipper
    bool UpdateClipper(); // set current clipper to render target
    // Surface
    bool PrepareRendering(SURFACE sfcToSurface); // check if/make rendering possible to given surface
    // Blit
    void PerformBlt(CBltData &rBltData, CTexRef *pTex, DWORD dwModClr, bool fMod2, bool fExact);
    BOOL BlitTex2Window(CTexRef *pTexRef, HDC hdcTarget, RECT &rtFrom, RECT &rtTo);
    BOOL BlitSurface2Window(SURFACE sfcSource, int fX, int fY, int fWdt, int fHgt, HWND hWnd, int tX, int tY, int tWdt, int tHgt);
    void FillBG(DWORD dwClr=0);
    // Drawing
    void DrawQuadDw(SURFACE sfcTarget, int *ipVtx, DWORD dwClr1, DWORD dwClr2, DWORD dwClr3, DWORD dwClr4);
    void DrawLineDw(SURFACE sfcTarget, float x1, float y1, float x2, float y2, DWORD dwClr);
    void DrawPixInt(SURFACE sfcDest, float tx, float ty, DWORD dwCol);
		void DrawPixPrimary(SURFACE sfcDest, int tx, int ty, DWORD dwCol);
    // Gamma
    virtual bool ApplyGammaRamp(CGammaControl &ramp, bool fForce);
    virtual bool SaveDefaultGammaRamp(CStdWindow * pWindow);
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

    DWORD CreateStateBlock(bool fTransparent, bool fSolid, bool fBaseTex, bool fAdditive, bool fMod2); // capture state blocks for blitting

  protected:
    BOOL FindDisplayMode(unsigned int iXRes, unsigned int iYRes, unsigned int iColorDepth, unsigned int iMonitor);
    virtual bool CreatePrimarySurfaces(BOOL Fullscreen, int iColorDepth, unsigned int iMonitor);
    bool SetOutputAdapter(unsigned int iMonitor);
    BOOL CreateDirectDraw();


  friend class CSurface;
  friend class CTexRef;
  friend class CPattern;
  };

// Global D3D access pointer
extern CStdD3D *pD3D;

#endif // defined(USE_DIRECTX) && !defined(INC_STDD3D)
