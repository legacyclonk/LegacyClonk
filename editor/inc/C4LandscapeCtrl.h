/* Copyright (C) 1998-2000  Matthes Bender  RedWolf Design */

/* Landscape preview in scenario properties */

#include <C4MapCreatorS2.h>

class C4LandscapeCtrl: public CStatic
	{
	public:
		C4LandscapeCtrl();
		~C4LandscapeCtrl();

	public:
		bool IsCreatedMap();
		bool IsLocalMaterials();
		void Init(C4Group &hGroup, C4SLandscape &hLandscape);
		void Update(C4SLandscape &rLandscape);
		void EnableOptions(BOOL fShowScreen, BOOL fShowLayers, int iShowPlayers);
		bool IsStaticMap();

	protected:
		bool LoadStaticMap(C4Group &hGroup);
		bool LoadDynamicMap(C4Group &hGroup, C4SLandscape &hLandscape);
		void UpdateSkyColor(C4SLandscape &rLandscape);
		void InitMaterialTexture(C4Group &hScenarioFile);

		BOOL ShowScreen;
		BOOL ShowLayers;
		int nPreviewPlayers;
		CDIBitmap Bitmap;
		bool StaticMap;  // Wenn gesetzt, bei Update() Kartenwerte und Creator ignorieren.
		bool LocalMaterials;
		bool CreatedMap; // Karte mit Landscape.txt-Kartengenerator

	private:
		unsigned char* pMapBuffer;
		int MapBufferWdt, MapBufferHgt;
		int32_t MapWdt, MapHgt, MapZoom;
		C4TextureMap TextureMap;
		C4MaterialMap MaterialMap;
		RGBQUAD Palette[256];
		C4MapCreatorS2 *pDynMap;

		//{{AFX_VIRTUAL(C4LandscapeCtrl)
		//}}AFX_VIRTUAL

	protected:
		//{{AFX_MSG(C4LandscapeCtrl)
		afx_msg void OnPaint();
		//afx_msg BOOL OnEraseBkgnd(CDC* pDC);
		//}}AFX_MSG
		DECLARE_MESSAGE_MAP()
	};
