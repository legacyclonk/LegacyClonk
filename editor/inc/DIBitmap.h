/* Copyright (C) 1998-2000  Matthes Bender  RedWolf Design */

/* Loads and draws bitmaps in windows */

class CDIBitmap
	{
	public:
		CDIBitmap();
		~CDIBitmap();
		BITMAPINFO* pBMI;  
		BITMAPINFOHEADER* pBMIH;
		char* pBits;
		bool OwnBMI;
	protected:
		int iSectionX, iSectionY, iSectionWdt, iSectionHgt;
		BYTE *bypMask;
	public:
		bool HasMask();
		void CreateMask();
		void SetColor(int iColor, int iRed, int iGreen, int iBlue);
		bool DrawTiles(CDC *pDC, CRect &Rect);
		void Default();
		bool Clear();
		bool Load(UINT ID, HMODULE hModule=NULL);
		bool Load(BITMAPFILEHEADER* pBMFH, int Size);
		bool Load(CString FileName);
		bool Load(C4Group& Group, CString FileName);
		bool LoadLandscape(char* pBits, int w, int h, RGBQUAD* pPalette);
		bool Draw(HDC hDC, int iX=0, int iY=0, int iWdt=0, int iHgt=0, bool fStretch=true) const;
		bool Draw(CDC* pDC, CWnd* pWnd) const;
		bool DrawTiles(CDC* pDC, CWnd* pWnd) const;
		bool DrawSection(int iSecIndexX, int iSecIndexY, CDC *pDC, CRect Rect, 
										 bool fAspect=false, bool fCentered=false);
		bool DrawFixedAspect(CDC* pDC, CRect Rect, CSize* pSizeDrawn=NULL) const;
		bool HasContent() const { return pBMI != NULL; }
		CSize GetSize() const { return CSize(pBMIH->biWidth, pBMIH->biHeight); }
		void SetSection(int iWdt, int iHgt);
		void SetSection(int iX, int iY, int iWdt, int iHgt);
	protected:
		char* GetBitsPointer() const;
	};