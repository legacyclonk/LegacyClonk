/* Copyright (C) 1998-2000  Matthes Bender  RedWolf Design */

/* Landscape preview in scenario properties */

#include "C4Explorer.h"

#include "C4LandscapeCtrl.h"

//---------------------------------------------------------------------------

C4LandscapeCtrl::C4LandscapeCtrl()
	{
	pMapBuffer = NULL;
	ShowScreen = true;
	ShowLayers = false;
	nPreviewPlayers = 1;
  StaticMap = false;
  CreatedMap = false;
	MapBufferWdt=0;
	MapBufferHgt=0;
	LocalMaterials = false;
	pDynMap=NULL;
	}

//---------------------------------------------------------------------------

C4LandscapeCtrl::~C4LandscapeCtrl()
	{
	if (pDynMap) delete pDynMap;
	if (pMapBuffer) delete [] pMapBuffer;
	pMapBuffer = NULL;
	}

//---------------------------------------------------------------------------

BEGIN_MESSAGE_MAP(C4LandscapeCtrl, CStatic)
	//{{AFX_MSG_MAP(C4LandscapeCtrl)
	ON_WM_PAINT()
	ON_WM_ERASEBKGND()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

//---------------------------------------------------------------------------

void C4LandscapeCtrl::Init(C4Group &hScenarioFile, C4SLandscape &hLandscape) // Nur einmal aufrufen
	{
	// alte dynamsiche Karte löschen
	if (pDynMap) delete pDynMap; pDynMap=NULL;

	// Texture/Material-Daten laden, Palette erstellen
	InitMaterialTexture(hScenarioFile);

	// Statische Karte laden
	if (!LoadStaticMap(hScenarioFile))
		// dynamische Karte laden
		LoadDynamicMap(hScenarioFile, hLandscape);

	}

//---------------------------------------------------------------------------

// Aufrufen, nachdem der Dialog die aktuellen Werte von den Controls in die EditRound geladen hat

void C4LandscapeCtrl::Update( C4SLandscape &rLandscape)
	{

	// Hintergrundfarbe
	UpdateSkyColor( rLandscape );

	if (StaticMap)  // Bitmap is schon fertig
		{
		MapZoom = rLandscape.MapZoom.Evaluate();
		Invalidate();
		return;
		}

	// Aktuelle Anzahl Spieler für Kartenvorschau
	int nPlayers = 1;
	if (rLandscape.MapPlayerExtend) nPlayers = nPreviewPlayers;

	// Aktuelle Kartenwerte
	rLandscape.GetMapSize(MapWdt,MapHgt,nPlayers);
	MapZoom = rLandscape.MapZoom.Evaluate();

	// Dynamische Karte?
	C4MCMap *pMap=NULL;
	if (pDynMap) pMap=pDynMap->GetMap(NULL);
	if (pMap)
		{
		// Größe übermitteln
		pMap->SetSize(MapWdt, MapHgt);
		}

	// Puffer prüfen: wenn nicht vorhanden oder zu klein, neuen Puffer erzeugen
	if ( !pMapBuffer 
		|| (DWordAligned(MapWdt) > MapBufferWdt)
		|| (DWordAligned(MapHgt) > MapBufferHgt))
		
		{
		// Alten Puffer löschen
		if (pMapBuffer) delete [] pMapBuffer; pMapBuffer=NULL;
		// Neuen Puffer erzeugen
		MapBufferWdt = MapWdt;
		DWordAlign(MapBufferWdt);
		MapBufferHgt = MapHgt;
		pMapBuffer = new BYTE [MapBufferWdt * MapBufferHgt];
		}

	// Pufferbreite
	int BmpBufWdt = MapWdt;
	DWordAlign(BmpBufWdt);

	// neue dynamsiche Landschaft?
	if (pMap)
		{
		pMap->RenderTo(pMapBuffer, BmpBufWdt);
		}
	else
		{
		// alte Landschaft
		C4MapCreator MapCreator;
    CSurface8 sfc;
    sfc.SetBuffer(pMapBuffer, MapWdt, MapHgt, BmpBufWdt);
		MapCreator.Create(&sfc, rLandscape, TextureMap, ShowLayers, nPreviewPlayers);
    sfc.ReleaseBuffer();
		}

	Bitmap.LoadLandscape((char*) pMapBuffer, MapWdt, MapHgt, Palette);

	Invalidate();
	}

//---------------------------------------------------------------------------

void C4LandscapeCtrl::OnPaint() 
{
	CPaintDC DC(this);
	CSize MapSize;
	CRect ClientRect;
	GetClientRect(ClientRect);

	// Landkarte malen
	Bitmap.DrawFixedAspect(&DC, ClientRect, &MapSize);

	// Screensize Frame
	if (ShowScreen)
		if ((MapWdt!=0) && (MapHgt!=0) && (MapZoom!=0))
		{
			CRect ScreenRect;
			/*int Res = GetApp()->GetProfileInt(Str("IDS_RK_GRAPHICS"), "Resolution", 1);
			Res = max(min(Res,3),0);
			static int ResXs[4] = {320, 640, 800, 1024};
			static int ResYs[4] = {200, 480, 600, 768};*/
			int ResX = GetCfg()->Graphics.ResX;
			int ResY = GetCfg()->Graphics.ResY;
			ScreenRect.left = 0;
			ScreenRect.top = 0;
			ScreenRect.right = ResX * MapSize.cx / (MapWdt * MapZoom);
			ScreenRect.bottom = ResY * MapSize.cy / (MapHgt * MapZoom);
			ScreenRect.right = min(ScreenRect.right, MapSize.cx );
			ScreenRect.bottom = min(ScreenRect.bottom, MapSize.cy );

			CBrush BlackBrush(HS_BDIAGONAL, COLORREF(0));
			DC.FrameRect(ScreenRect, &BlackBrush);
		}
}

//---------------------------------------------------------------------------

/*BOOL C4LandscapeCtrl::OnEraseBkgnd(CDC* pDC) 
{
	return true;
}*/

//---------------------------------------------------------------------------

bool C4LandscapeCtrl::LoadStaticMap(C4Group &hGroup)
	{

	StaticMap = false;
	
	if (Bitmap.Load(hGroup, C4CFN_Landscape))
		{
		StaticMap = true; 
		MapWdt = Bitmap.GetSize().cx;
		MapHgt = Bitmap.GetSize().cy;
		}
	
	return StaticMap;
	}

bool C4LandscapeCtrl::LoadDynamicMap(C4Group &hGroup, C4SLandscape &hLandscape)
	{
	// Datei vorhanden?
	if (!hGroup.AccessEntry(C4CFN_DynLandscape)) return false;
	// Laden
	pDynMap = new C4MapCreatorS2(&hLandscape, &TextureMap, &MaterialMap, nPreviewPlayers);
	if (!pDynMap->ReadFile(C4CFN_DynLandscape, &hGroup))
		{
		delete pDynMap; pDynMap=NULL;
		return false;
		}
	CreatedMap = true;
	return true;
	}

//---------------------------------------------------------------------------

void C4LandscapeCtrl::UpdateSkyColor(C4SLandscape &rLandscape)
	{
	// Wenn alle Farbverlaufwerte 0 bedeutet das blauer Default
	if (0 == (rLandscape.SkyDefFade[3] + rLandscape.SkyDefFade[4] + rLandscape.SkyDefFade[5]))
		{
		Palette[0].rgbRed   = 100;
		Palette[0].rgbGreen = 100;
		Palette[0].rgbBlue  = 255;

		}
	else
		{
		Palette[0].rgbRed   = (BYTE) rLandscape.SkyDefFade[3];
		Palette[0].rgbGreen = (BYTE) rLandscape.SkyDefFade[4];
		Palette[0].rgbBlue  = (BYTE) rLandscape.SkyDefFade[5];
		}
	}

void C4LandscapeCtrl::EnableOptions(BOOL fShowScreen, BOOL fShowLayers, int iShowPlayers)
	{
	ShowScreen = fShowScreen;
	ShowLayers = fShowLayers;
	nPreviewPlayers = iShowPlayers;
	}

bool C4LandscapeCtrl::IsStaticMap()
	{
	return StaticMap;
	}	

void C4LandscapeCtrl::InitMaterialTexture(C4Group &hScenarioFile) // Nur einmal aufrufen
	{

	// Open local and global material files
	C4Group hGroupLocal, hGroupGlobal;
	if (hGroupLocal.OpenAsChild(&hScenarioFile,C4CFN_Material)) // Lokal
		LocalMaterials = true;
	hGroupGlobal.Open( GetCfg()->AtExePath(C4CFN_Material) ); // Global

	// Load TextureMap (and determine whether to overload materials/textures)
	int nTexNum, nMatNum=0;
	BOOL fOverloadMaterials,fOverloadTextures;
	nTexNum = TextureMap.LoadMap(LocalMaterials ? hGroupLocal : hGroupGlobal, C4CFN_TexMap, &fOverloadMaterials, &fOverloadTextures);

	// Load MaterialMap
  if (fOverloadMaterials) 
		nMatNum = MaterialMap.Load(hGroupGlobal, &hGroupLocal);
	else 
		nMatNum = MaterialMap.Load(LocalMaterials ? hGroupLocal : hGroupGlobal);  

	// Old implementation - wasn't proper overloading
	/*if (LocalMaterials) nMatNum = MaterialMap.Load( hGroupLocal );
	if (!nMatNum) nMatNum=MaterialMap.Load( hGroupGlobal );*/

	TRACE("\nLandscapeCtrl: %d Texturen und %d Materialien geladen\n", nTexNum, nMatNum);
	if (LocalMaterials) hGroupLocal.Close();
	hGroupGlobal.Close();
	if (!nMatNum) C4MessageBox("IDS_MSG_NOMATS");

	// Palette machen
	int i, Mat;
	for (i=0; i < C4M_MaxTexIndex; i++)
	{
		const C4TexMapEntry *pTex = TextureMap.GetEntry(i);
		Mat = pTex ? MaterialMap.Get(pTex->GetMaterialName()) : MNone;
		if (Mat != MNone)
		{
			Palette[i].rgbBlue  = (BYTE) MaterialMap.Map[Mat].Color[2];
			Palette[i].rgbGreen = (BYTE) MaterialMap.Map[Mat].Color[1];
			Palette[i].rgbRed   = (BYTE) MaterialMap.Map[Mat].Color[0];
			Palette[i].rgbReserved = 0;
			Palette[128+i] = Palette[i];
		}
		else
		{
			Palette[i].rgbBlue  = (BYTE) i;
			Palette[i].rgbGreen = (BYTE) i;
			Palette[i].rgbRed   = (BYTE) i;
			Palette[i].rgbReserved = 0;
		}
	}
	
	// Himmel
	Palette[0].rgbBlue  = 255; // TODO: Wenn EditRound-abhängig, auch EditRound-SkyDef benutzen
	Palette[0].rgbGreen = 100;
	Palette[0].rgbRed   = 100;
	Palette[0].rgbReserved = 0;
	}

bool C4LandscapeCtrl::IsLocalMaterials()
	{
	return LocalMaterials;
	}

bool C4LandscapeCtrl::IsCreatedMap()
{
	return CreatedMap;
}
