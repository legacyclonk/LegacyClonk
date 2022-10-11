/*
 * LegacyClonk
 *
 * Copyright (c) RedWolf Design
 * Copyright (c) 2005, Sven2
 * Copyright (c) 2017-2022, The LegacyClonk Team and contributors
 *
 * Distributed under the terms of the ISC license; see accompanying file
 * "COPYING" for details.
 *
 * "Clonk" is a registered trademark of Matthes Bender, used with permission.
 * See accompanying file "TRADEMARK" for details.
 *
 * To redistribute this file separately, substitute the full license texts
 * for the above references.
 */

// Startup screen for non-parameterized engine start: Scenario selection dialog

#include <C4Include.h>
#include <C4StartupScenSelDlg.h>

#include <C4Network2Dialogs.h>
#include <C4StartupMainDlg.h>
#include <C4StartupNetDlg.h>
#include <C4ComponentHost.h>
#include <C4Components.h>
#include <C4RTF.h>
#include <C4Wrappers.h>
#include <C4Log.h>
#include <C4Game.h>
#include <C4GameDialogs.h>
#include <C4Language.h>
#include <C4FileSelDlg.h>

#include <set>

// singleton
C4StartupScenSelDlg *C4StartupScenSelDlg::pInstance = nullptr;

// Map folder data

void C4MapFolderData::Scenario::CompileFunc(StdCompiler *pComp)
{
	pComp->Value(mkNamingAdapt(sFilename,       "File",               StdStrBuf()));
	pComp->Value(mkNamingAdapt(sBaseImage,      "BaseImage",          StdStrBuf()));
	pComp->Value(mkNamingAdapt(sOverlayImage,   "OverlayImage",       StdStrBuf()));
	pComp->Value(mkNamingAdapt(singleClick,     "SingleClick",        false));
	pComp->Value(mkNamingAdapt(rcOverlayPos,    "Area",               C4Rect()));
	pComp->Value(mkNamingAdapt(sTitle,          "Title",              StdStrBuf()));
	pComp->Value(mkNamingAdapt(iTitleFontSize,  "TitleFontSize",      20));
	pComp->Value(mkNamingAdapt(dwTitleInactClr, "TitleColorInactive", 0x7fffffffu));
	pComp->Value(mkNamingAdapt(dwTitleActClr,   "TitleColorActive",   0x0fffffffu));
	pComp->Value(mkNamingAdapt(iTitleOffX,      "TitleOffX",          0));
	pComp->Value(mkNamingAdapt(iTitleOffY,      "TitleOffY",          0));
	pComp->Value(mkNamingAdapt(byTitleAlign,    "TitleAlign",         ACenter));
	pComp->Value(mkNamingAdapt(fTitleBookFont,  "TitleUseBookFont",   true));
	pComp->Value(mkNamingAdapt(fImgDump,        "ImageDump",          false)); // developer help
}

void C4MapFolderData::AccessGfx::CompileFunc(StdCompiler *pComp)
{
	pComp->Value(mkNamingAdapt(sPassword,     "Access",       StdStrBuf()));
	pComp->Value(mkNamingAdapt(sOverlayImage, "OverlayImage", StdStrBuf()));
	pComp->Value(mkNamingAdapt(rcOverlayPos,  "Area",         C4Rect()));
}

C4MapFolderData::MapPic::MapPic(const FLOAT_RECT &rcfBounds, const C4Facet &rfct) : C4GUI::Picture(C4Rect(rcfBounds), false), rcfBounds(rcfBounds)
{
	SetFacet(rfct);
	SetToolTip(LoadResStr("IDS_MSG_MAP_DESC"));
}

void C4MapFolderData::MapPic::MouseInput(C4GUI::CMouse &rMouse, int32_t iButton, int32_t iX, int32_t iY, uint32_t dwKeyParam)
{
	typedef C4GUI::Picture Parent;
	Parent::MouseInput(rMouse, iButton, iX, iY, dwKeyParam);
	// input: mouse movement or buttons - deselect everything if clicked
	if (iButton == C4MC_Button_LeftDown && C4StartupScenSelDlg::pInstance)
	{
		C4StartupScenSelDlg::pInstance->DeselectAll();
	}
}

void C4MapFolderData::MapPic::DrawElement(C4FacetEx &cgo)
{
	// get drawing bounds
	float x0 = rcfBounds.left + cgo.TargetX, y0 = rcfBounds.top + cgo.TargetY;
	// draw the image
	GetFacet().DrawXFloat(cgo.Surface, x0, y0, rcfBounds.right - rcfBounds.left, rcfBounds.bottom - rcfBounds.top);
}

void C4MapFolderData::Clear()
{
	fCoordinatesAdjusted = false;
	fctBackgroundPicture.Clear();
	pScenarioFolder = nullptr;
	pSelectedEntry = nullptr;
	pSelectionInfoBox = nullptr;
	rcScenInfoArea.Set(0, 0, 0, 0);
	MinResX = MinResY = 0;
	fUseFullscreenMap = false;
	int i;
	for (i = 0; i < iScenCount; ++i) delete ppScenList[i];
	iScenCount = 0;
	delete[] ppScenList; ppScenList = nullptr;
	for (i = 0; i < iAccessGfxCount; ++i) delete ppAccessGfxList[i];
	iAccessGfxCount = 0;
	delete[] ppAccessGfxList; ppAccessGfxList = nullptr;
	pMainDlg = nullptr;
}

bool C4MapFolderData::Load(C4Group &hGroup, C4ScenarioListLoader::Folder *pScenLoaderFolder)
{
	// clear previous
	Clear();
	// load localization info
	C4LangStringTable LangTable;
	bool fHasLangTable = !!LangTable.LoadEx("StringTbl", hGroup, C4CFN_ScriptStringTbl, Config.General.LanguageEx);
	// load core data
	StdStrBuf Buf;
	if (!hGroup.LoadEntryString(C4CFN_MapFolderData, Buf)) return false;
	if (fHasLangTable) LangTable.ReplaceStrings(Buf);
	if (!CompileFromBuf_LogWarn<StdCompilerINIRead>(mkNamingAdapt(*this, "FolderMap"), Buf, C4CFN_MapFolderData)) return false;
	// check resolution requirement
	if (MinResX && MinResX > C4GUI::GetScreenWdt()) return false;
	if (MinResY && MinResY > C4GUI::GetScreenHgt()) return false;
	// load images
	if (!fctBackgroundPicture.Load(hGroup, C4CFN_MapFolderBG))
	{
		DebugLogF("C4MapFolderData::Load(%s): Could not load background graphic \"%s\"", hGroup.GetName(), C4CFN_MapFolderBG);
		return false;
	}
	int i;
	for (i = 0; i < iScenCount; ++i)
	{
		// init scenario entry stuff
		Scenario *pScen = ppScenList[i];
		pScen->pScenEntry = pScenLoaderFolder->FindEntryByName(pScen->sFilename.getData());
		pScen->pBtn = nullptr;
		pScen->sTitle.Replace("TITLE", pScen->pScenEntry ? pScen->pScenEntry->GetName().getData() : "<c ff0000>ERROR</c>" /* scenario not loaded; title cannot be referenced */);
		// developer image dump
		if (pScen->fImgDump)
		{
			C4FacetExSurface fctDump; bool fSuccess = false;
			if (fctDump.Create(pScen->rcOverlayPos.Wdt, pScen->rcOverlayPos.Hgt, C4FCT_Full, C4FCT_Full))
			{
				lpDDraw->Blit(fctBackgroundPicture.Surface,
					static_cast<float>(pScen->rcOverlayPos.x), static_cast<float>(pScen->rcOverlayPos.y),
					static_cast<float>(pScen->rcOverlayPos.Wdt), static_cast<float>(pScen->rcOverlayPos.Hgt),
					fctDump.Surface,
					0, 0,
					fctDump.Wdt, fctDump.Hgt);
				fSuccess = fctDump.Surface->SavePNG(pScen->sBaseImage.getData(), true, false, false);
			}
			if (!fSuccess)
				DebugLogF("C4MapFolderData::Load(%s): Could not dump graphic \"%s\"", hGroup.GetName(), pScen->sBaseImage.getData());
			continue;
		}
		// load images
		if (pScen->sBaseImage.getLength() > 0) if (!pScen->fctBase.Load(hGroup, pScen->sBaseImage.getData()))
		{
			DebugLogF("C4MapFolderData::Load(%s): Could not load base graphic \"%s\"", hGroup.GetName(), pScen->sBaseImage.getData());
			return false;
		}
	}
	for (i = 0; i < iScenCount; ++i)
	{
		Scenario *pScen = ppScenList[i];
		if (pScen->sOverlayImage.getLength() > 0) if (!pScen->fctOverlay.Load(hGroup, pScen->sOverlayImage.getData()))
		{
			DebugLogF("C4MapFolderData::Load(%s): Could not load graphic \"%s\"", hGroup.GetName(), pScen->sOverlayImage.getData());
			return false;
		}
	}
	for (i = 0; i < iAccessGfxCount; ++i)
	{
		AccessGfx *pGfx = ppAccessGfxList[i];
		if (pGfx->sOverlayImage.getLength() > 0) if (!pGfx->fctOverlay.Load(hGroup, pGfx->sOverlayImage.getData()))
		{
			DebugLogF("C4MapFolderData::Load(%s): Could not load graphic \"%s\"", hGroup.GetName(), pGfx->sOverlayImage.getData());
			return false;
		}
	}
	// all loaded
	pScenarioFolder = pScenLoaderFolder;
	return true;
}

void C4MapFolderData::CompileFunc(StdCompiler *pComp)
{
	// core values
	pComp->Value(mkNamingAdapt(rcScenInfoArea,    "ScenInfoArea", C4Rect(0, 0, 0, 0)));
	pComp->Value(mkNamingAdapt(MinResX,           "MinResX",      0));
	pComp->Value(mkNamingAdapt(MinResY,           "MinResY",      0));
	pComp->Value(mkNamingAdapt(fUseFullscreenMap, "FullscreenBG", false));
	pComp->Value(mkNamingAdapt(hideTitle,         "HideTitle",    false));
	// compile scenario list
	int32_t iOldScenCount = iScenCount;
	pComp->Value(mkNamingCountAdapt(iScenCount, "Scenario"));
	if (pComp->isCompiler())
	{
		while (iOldScenCount--) delete ppScenList[iOldScenCount];
		delete[] ppScenList;
		if (iScenCount)
		{
			ppScenList = new Scenario *[iScenCount]{};
		}
		else
			ppScenList = nullptr;
	}
	if (iScenCount)
	{
		mkPtrAdaptNoNull(*ppScenList);
		pComp->Value(mkNamingAdapt(mkArrayAdaptMapS(ppScenList, iScenCount, mkPtrAdaptNoNull<Scenario>), "Scenario"));
	}
	// compile access gfx list
	int32_t iOldAccesGfxCount = iAccessGfxCount;
	pComp->Value(mkNamingCountAdapt(iAccessGfxCount, "AccessGfx"));
	if (pComp->isCompiler())
	{
		while (iOldAccesGfxCount--) delete ppAccessGfxList[iOldAccesGfxCount];
		delete[] ppAccessGfxList;
		if (iAccessGfxCount)
		{
			ppAccessGfxList = new AccessGfx *[iAccessGfxCount]{};
		}
		else
			ppAccessGfxList = nullptr;
	}
	if (iAccessGfxCount)
	{
		mkPtrAdaptNoNull(*ppAccessGfxList);
		pComp->Value(mkNamingAdapt(mkArrayAdaptMapS(ppAccessGfxList, iAccessGfxCount, mkPtrAdaptNoNull<AccessGfx>), "AccessGfx"));
	}
}

void C4MapFolderData::ConvertFacet2ScreenCoord(const C4Rect &rc, FLOAT_RECT *pfrc, float fBGZoomX, float fBGZoomY, int iOffX, int iOffY)
{
	pfrc->left = (fBGZoomX * rc.x) + iOffX;
	pfrc->top = (fBGZoomY * rc.y) + iOffY;
	pfrc->right = pfrc->left + (fBGZoomX * rc.Wdt);
	pfrc->bottom = pfrc->top + (fBGZoomY * rc.Hgt);
}

void C4MapFolderData::ConvertFacet2ScreenCoord(int32_t *piValue, float fBGZoom, int iOff)
{
	*piValue = int32_t(floorf(fBGZoom * *piValue + 0.5f)) + iOff;
}

void C4MapFolderData::ConvertFacet2ScreenCoord(C4Rect &rcMapArea, bool fAspect)
{
	if (!fctBackgroundPicture.Wdt || !fctBackgroundPicture.Hgt) return; // invalid BG - should not happen
	// get zoom of background image
	float fBGZoomX = 1.0f, fBGZoomY = 1.0f; int iOffX = 0, iOffY = 0;
	if (fAspect)
	{
		if (fctBackgroundPicture.Wdt * rcMapArea.Hgt > rcMapArea.Wdt * fctBackgroundPicture.Hgt)
		{
			// background image is limited by width
			fBGZoomX = fBGZoomY = static_cast<float>(rcMapArea.Wdt) / fctBackgroundPicture.Wdt;
			iOffY = std::max<int>(0, static_cast<int>(rcMapArea.Hgt - (fBGZoomX * fctBackgroundPicture.Hgt))) / 2;
		}
		else
		{
			// background image is limited by height
			fBGZoomX = fBGZoomY = static_cast<float>(rcMapArea.Hgt) / fctBackgroundPicture.Hgt;
			iOffX = std::max<int>(0, static_cast<int>(rcMapArea.Wdt - (fBGZoomY * fctBackgroundPicture.Wdt))) / 2;
		}
	}
	else
	{
		// do not keep aspect: Independent X and Y zoom
		fBGZoomX = static_cast<float>(rcMapArea.Wdt) / fctBackgroundPicture.Wdt;;
		fBGZoomY = static_cast<float>(rcMapArea.Hgt) / fctBackgroundPicture.Hgt;;
	}
	iOffX -= rcMapArea.x; iOffY -= rcMapArea.y;
	C4Rect rcBG; rcBG.Set(0, 0, fctBackgroundPicture.Wdt, fctBackgroundPicture.Hgt);
	ConvertFacet2ScreenCoord(rcBG, &rcfBG, fBGZoomX, fBGZoomY, iOffX, iOffY);
	// default for scenario info area: 1/3rd of right area
	if (!rcScenInfoArea.Wdt)
		rcScenInfoArea.Set(static_cast<int32_t>(fctBackgroundPicture.Wdt * 2 / 3), static_cast<int32_t>(fctBackgroundPicture.Hgt / 16), static_cast<int32_t>(fctBackgroundPicture.Wdt / 3), static_cast<int32_t>(fctBackgroundPicture.Hgt * 7 / 8));
	// assume all facet coordinates are referring to background image zoom; convert them to screen coordinates by applying zoom and offset
	FLOAT_RECT rcfScenInfoArea;
	ConvertFacet2ScreenCoord(rcScenInfoArea, &rcfScenInfoArea, fBGZoomX, fBGZoomY, iOffX, iOffY);
	rcScenInfoArea.x = static_cast<int32_t>(rcfScenInfoArea.left); rcScenInfoArea.y = static_cast<int32_t>(rcfScenInfoArea.top);
	rcScenInfoArea.Wdt = static_cast<int32_t>(rcfScenInfoArea.right - rcfScenInfoArea.left);
	rcScenInfoArea.Hgt = static_cast<int32_t>(rcfScenInfoArea.bottom - rcfScenInfoArea.top);
	int i;
	for (i = 0; i < iScenCount; ++i)
	{
		Scenario *pScen = ppScenList[i];
		ConvertFacet2ScreenCoord(pScen->rcOverlayPos, &(pScen->rcfOverlayPos), fBGZoomX, fBGZoomY, iOffX, iOffY);
		// title sizes
		ConvertFacet2ScreenCoord(&(pScen->iTitleFontSize), fBGZoomY, 0);
		// title position: Relative to title rect; so do not add offset here
		ConvertFacet2ScreenCoord(&(pScen->iTitleOffX), fBGZoomX, 0);
		ConvertFacet2ScreenCoord(&(pScen->iTitleOffY), fBGZoomY, 0);
	}
	for (i = 0; i < iAccessGfxCount; ++i) ConvertFacet2ScreenCoord(ppAccessGfxList[i]->rcOverlayPos, &(ppAccessGfxList[i]->rcfOverlayPos), fBGZoomX, fBGZoomY, iOffX, iOffY);
	// done
	fCoordinatesAdjusted = true;
}

void C4MapFolderData::CreateGUIElements(C4StartupScenSelDlg *pMainDlg, C4GUI::Window &rContainer)
{
	this->pMainDlg = pMainDlg;

	if (hideTitle)
	{
		this->pMainDlg->HideTitle(true);
	}

	// convert all coordinates to match the container sizes
	// do this only once; assume container won't change between loads
	if (!fCoordinatesAdjusted)
		if (!fUseFullscreenMap)
			ConvertFacet2ScreenCoord(rContainer.GetClientRect(), true);
		else
		{
			C4Rect rcMapRect = pMainDlg->GetBounds();
			rContainer.ClientPos2ScreenPos(rcMapRect.x, rcMapRect.y);
			ConvertFacet2ScreenCoord(rcMapRect, false);
		}
	// empty any previous stuff in container
	while (rContainer.GetFirst()) delete rContainer.GetFirst();
	// create background image
	if (!fUseFullscreenMap)
		rContainer.AddElement(new MapPic(rcfBG, fctBackgroundPicture));
	else
	{
		pMainDlg->SetBackground(&fctBackgroundPicture);
	}
	// create mission access overlays
	int i;
	for (i = 0; i < iAccessGfxCount; ++i)
	{
		AccessGfx *pGfx = ppAccessGfxList[i];
		const char *szPassword = pGfx->sPassword.getData();
		if (!szPassword || !*szPassword || SIsModule(Config.General.MissionAccess, szPassword))
		{
			// ACCESS TO GFX GRANTED: draw it
			rContainer.AddElement(new MapPic(pGfx->rcfOverlayPos, pGfx->fctOverlay));
		}
	}
	// create buttons for scenarios
	C4GUI::Button *pBtnFirst = nullptr;
	for (i = 0; i < iScenCount; ++i)
	{
		Scenario *pScen = ppScenList[i];
		if (pScen->pScenEntry && !pScen->pScenEntry->HasMissionAccess())
		{
			// no access to this scenario: Do not create a button at all; not even base image. The scenario is "invisible".
		}
		else
		{
			C4GUI::CallbackButtonEx<C4StartupScenSelDlg, C4GUI::FacetButton> *pBtn = new C4GUI::CallbackButtonEx<C4StartupScenSelDlg, C4GUI::FacetButton>
				(pScen->fctBase, pScen->fctOverlay, pScen->rcfOverlayPos, 0, pMainDlg, &C4StartupScenSelDlg::OnButtonScenario);
			ppScenList[i]->pBtn = pBtn;
			if (pScen->pScenEntry)
				pBtn->SetToolTip(FormatString(LoadResStr("IDS_MSG_MAP_STARTSCEN"), pScen->pScenEntry->GetName().getData()).getData());
			if (pScen->sTitle.getLength() > 0)
			{
				pBtn->SetText(pScen->sTitle.getData());
				pBtn->SetTextColors(InvertRGBAAlpha(pScen->dwTitleInactClr), InvertRGBAAlpha(pScen->dwTitleActClr));
				pBtn->SetTextPos(pScen->iTitleOffX, pScen->iTitleOffY, pScen->byTitleAlign);
				CStdFont *pUseFont; float fFontZoom = 1.0f;
				if (pScen->fTitleBookFont)
					pUseFont = &(C4Startup::Get()->Graphics.GetBlackFontByHeight(pScen->iTitleFontSize, &fFontZoom));
				else
					pUseFont = &(C4GUI::GetRes()->GetFontByHeight(pScen->iTitleFontSize, &fFontZoom));
				if (Inside<float>(fFontZoom, 0.8f, 1.25f)) fFontZoom = 1.0f; // some tolerance for font zoom
				pBtn->SetTextFont(pUseFont, fFontZoom);
			}
			rContainer.AddElement(pBtn);
			if (!pBtnFirst) pBtnFirst = pBtn;
		}
	}
	// create scenario info listbox
	pSelectionInfoBox = new C4GUI::TextWindow(rcScenInfoArea,
		C4StartupScenSel_TitlePictureWdt + 2 * C4StartupScenSel_TitleOverlayMargin, C4StartupScenSel_TitlePictureHgt + 2 * C4StartupScenSel_TitleOverlayMargin,
		C4StartupScenSel_TitlePicturePadding, 100, 4096, nullptr, true, &C4Startup::Get()->Graphics.fctScenSelTitleOverlay, C4StartupScenSel_TitleOverlayMargin);
	pSelectionInfoBox->SetDecoration(false, false, &C4Startup::Get()->Graphics.sfctBookScroll, true);
	rContainer.AddElement(pSelectionInfoBox);
}

bool C4MapFolderData::OnButtonScenario(C4GUI::Control *pEl)
{
	// get associated scenario entry
	int i;
	for (i = 0; i < iScenCount; ++i)
		if (pEl == ppScenList[i]->pBtn)
			break;
	if (i == iScenCount) return false;
	// select the associated entry
	pSelectedEntry = ppScenList[i]->pScenEntry;
	return ppScenList[i]->singleClick;
}

void C4MapFolderData::ResetSelection()
{
	pSelectedEntry = nullptr;
}

// Scenario list loader

// Entry

C4ScenarioListLoader::Entry::Entry(Folder *pParent) : pNext(nullptr), pParent(pParent), fBaseLoaded(false), fExLoaded(false)
{
	// ctor: Put into parent tree node
	if (pParent)
	{
		pNext = pParent->pFirst;
		pParent->pFirst = this;
	}
	iIconIndex = -1;
	iDifficulty = 0;
	iFolderIndex = 0;
}

C4ScenarioListLoader::Entry::~Entry()
{
	// dtor: unlink from parent list (MUST be in there)
	if (pParent)
	{
		Entry **ppCheck = &(pParent->pFirst);
		while (*ppCheck != this)
		{
			ppCheck = &(*ppCheck)->pNext;
		}
		*ppCheck = pNext;
	}
}

bool C4ScenarioListLoader::Entry::Load(C4Group *pFromGrp, const StdStrBuf *psFilename, bool fLoadEx)
{
	// nothing to do if already loaded
	if (fBaseLoaded && (fExLoaded || !fLoadEx)) return true;
	C4Group Group;
	// group specified: Load as child
	if (pFromGrp)
	{
		assert(psFilename);
		if (!Group.OpenAsChild(pFromGrp, psFilename->getData())) return false;
		// set FN by complete entry name
		this->sFilename.Take(Group.GetFullName());
	}
	else
	{
		// set FN by complete entry name
		if (psFilename) this->sFilename.Copy(*psFilename);
		// no parent group: Direct load from filename
		if (!Group.Open(sFilename.getData())) return false;
	}
	// okay; load standard stuff from group
	bool fNameLoaded = false, fIconLoaded = false;
	if (fBaseLoaded)
	{
		fNameLoaded = fIconLoaded = true;
	}
	else
	{
		// Set default name as filename without extension
		sName.Copy(GetFilename(sFilename.getData()));
		char *szBuf = sName.GrabPointer();
		RemoveExtension(szBuf);
		sName.Take(szBuf);
		sName.Take(C4Language::IconvClonk(sName.getData()));
		// load entry specific stuff that's in the front of the group
		if (!LoadCustomPre(Group))
			return false;
		// Load entry name
		C4ComponentHost DefNames;
		if (DefNames.LoadEx("Title", Group, C4CFN_Title, Config.General.LanguageEx))
			if (DefNames.GetLanguageString(Config.General.LanguageEx, sName))
				fNameLoaded = true;
		DefNames.Close();
		// load entry icon
		if (Group.FindEntry(C4CFN_IconPNG) && fctIcon.Load(Group, C4CFN_IconPNG))
			fIconLoaded = true;
		else
		{
			C4FacetExSurface fctTemp;
			if (Group.FindEntry(C4CFN_ScenarioIcon) && fctTemp.Load(Group, C4CFN_ScenarioIcon, C4FCT_Full, C4FCT_Full, true))
			{
				// old style icon: Blit it on a pieace of paper
				fctTemp.Surface->Lock();
				for (int y = 0; y < fctTemp.Hgt; ++y)
					for (int x = 0; x < fctTemp.Wdt; ++x)
					{
						uint32_t dwPix = fctTemp.Surface->GetPixDw(x, y, false);
						// transparency has some tolerance...
						if (Inside<uint8_t>(dwPix & 0xff, 0xb8, 0xff))
							if (Inside<uint8_t>((dwPix >> 0x08) & 0xff, 0x00, 0x0f))
								if (Inside<uint8_t>((dwPix >> 0x10) & 0xff, 0xb8, 0xff))
									fctTemp.Surface->SetPixDw(x, y, 0xffffffff);
					}
				fctTemp.Surface->Unlock();
				int iIconSize = C4Startup::Get()->Graphics.fctScenSelIcons.Hgt;
				fctIcon.Create(iIconSize, iIconSize, C4FCT_Full, C4FCT_Full);
				C4Startup::Get()->Graphics.fctScenSelIcons.GetPhase(C4StartupScenSel_DefaultIcon_OldIconBG).Draw(fctIcon);
				fctTemp.Draw(fctIcon.Surface, (fctIcon.Wdt - fctTemp.Wdt) / 2, (fctIcon.Hgt - fctTemp.Hgt) / 2);
				fctTemp.Clear();
				fIconLoaded = true;
			}
		}
		// load any entryx-type-specific custom data (e.g. fallbacks for scenario title, and icon)
		if (!LoadCustom(Group, fNameLoaded, fIconLoaded)) return false;
		// store maker
		sMaker.Copy(Group.GetMaker());
		fBaseLoaded = true;
	}
	// load extended stuff: title picture
	if (fLoadEx && !fExLoaded)
	{
		// load desc
		C4ComponentHost DefDesc;
		if (DefDesc.LoadEx("Desc", Group, C4CFN_ScenarioDesc, Config.General.LanguageEx))
		{
			C4RTFFile rtf;
			rtf.Load(StdBuf::MakeRef(DefDesc.GetData(), SLen(DefDesc.GetData())));
			sDesc.Take(rtf.GetPlainText());
		}
		DefDesc.Close();
		// load title
		if (!fctTitle.Load(Group, C4CFN_ScenarioTitlePNG, C4FCT_Full, C4FCT_Full, false, true))
			fctTitle.Load(Group, C4CFN_ScenarioTitle, C4FCT_Full, C4FCT_Full, true, true);
		fExLoaded = true;
		// load author
		if (Group.IsPacked())
		{
			const char *strSecAuthors = "RedWolf Design;Clonk History Project;GWE-Team"; // Now hardcoded...
			if (SIsModule(strSecAuthors, Group.GetMaker()) && Group.LoadEntryString(C4CFN_Author, sAuthor))
			{
				// OK; custom author by txt
			}
			else
				// defeault author by group
				sAuthor.Copy(Group.GetMaker());
		}
		else
		{
			// unpacked groups do not have an author
			sAuthor.Clear();
		}
		// load version
		Group.LoadEntryString(C4CFN_Version, sVersion);
	}
	// done, success
	return true;
}

// helper func: Recursive check whether a directory contains a .c4s or .c4f file
bool DirContainsScenarios(const char *szDir)
{
	// create iterator on free store to avoid stack overflow with deeply recursed folders
	DirectoryIterator *pIter = new DirectoryIterator(szDir);
	const char *szChildFilename;
	for (; szChildFilename = **pIter; ++*pIter)
	{
		// Ignore directory navigation entries and CVS folders
		if (!*szChildFilename || *GetFilename(szChildFilename) == '.') continue;
		if (SEqualNoCase(GetFilename(szChildFilename), "CVS")) continue;
		if (WildcardMatch(C4CFN_ScenarioFiles, szChildFilename) || WildcardMatch(C4CFN_FolderFiles, szChildFilename)) break;
		if (DirectoryExists(szChildFilename))
			if (DirContainsScenarios(szChildFilename))
				break;
	}
	delete pIter;
	// return true if loop was broken, in which case a matching entry was found
	return !!szChildFilename;
}

C4ScenarioListLoader::Entry *C4ScenarioListLoader::Entry::CreateEntryForFile(const StdStrBuf &sFilename, Folder *pParent)
{
	// determine entry type by file type
	const char *szFilename = sFilename.getData();
	if (!szFilename || !*szFilename) return nullptr;
	if (WildcardMatch(C4CFN_ScenarioFiles, sFilename.getData())) return new Scenario(pParent);
	if (WildcardMatch(C4CFN_FolderFiles, sFilename.getData())) return new SubFolder(pParent);
	// regular, open folder (C4Group-packed folders without extensions are not regarded, because they could contain anything!)
	const char *szExt = GetExtension(szFilename);
	if ((!szExt || !*szExt) && DirectoryExists(sFilename.getData()))
	{
		// open folders only if they contain a scenario or folder
		if (DirContainsScenarios(szFilename))
			return new RegularFolder(pParent);
	}
	// type not recognized
	return nullptr;
}

bool C4ScenarioListLoader::Entry::RenameTo(const char *szNewName)
{
	// change name+filename
	// some name sanity validation
	if (!szNewName || !*szNewName) return false;
	if (SEqual(szNewName, sName.getData())) return true;
	char fn[_MAX_PATH + 1];
	SCopy(szNewName, fn, _MAX_PATH);
	// generate new file name
	MakeFilenameFromTitle(fn);
	if (!*fn) return false;
	const char *szExt = GetDefaultExtension();
	if (szExt) { SAppend(".", fn, _MAX_PATH); SAppend(szExt, fn, _MAX_PATH); }
	char fullfn[_MAX_PATH + 1];
	SCopy(sFilename.getData(), fullfn, _MAX_PATH);
	char *fullfn_fn = GetFilename(fullfn);
	SCopy(fn, fullfn_fn, _MAX_PATH - (fullfn_fn - fullfn));
	StdStrBuf strErr(LoadResStr("IDS_FAIL_RENAME"));
	// check if a rename is due
	if (!ItemIdentical(sFilename.getData(), fullfn))
	{
		// check for duplicate filename
		if (ItemExists(fullfn))
		{
			StdStrBuf sMsg; sMsg.Format(LoadResStr("IDS_ERR_FILEEXISTS"), fullfn);
			Game.pGUI->ShowMessageModal(sMsg.getData(), strErr.getData(), C4GUI::MessageDialog::btnOK, C4GUI::Ico_Error);
			return false;
		}
		// OK; then rename
		if (!C4Group_MoveItem(sFilename.getData(), fullfn, true))
		{
			StdStrBuf sMsg; sMsg.Format(LoadResStr("IDS_ERR_RENAMEFILE"), sFilename.getData(), fullfn);
			Game.pGUI->ShowMessageModal(sMsg.getData(), strErr.getData(), C4GUI::MessageDialog::btnOK, C4GUI::Ico_Error);
			return false;
		}
		sFilename.Copy(fullfn);
	}
	// update real name in group, if this is a group
	if (C4Group_IsGroup(fullfn))
	{
		C4Group Grp;
		if (!Grp.Open(fullfn))
		{
			StdStrBuf sMsg; sMsg.Format(LoadResStr("IDS_ERR_OPENFILE"), sFilename.getData(), Grp.GetError());
			Game.pGUI->ShowMessageModal(sMsg.getData(), strErr.getData(), C4GUI::MessageDialog::btnOK, C4GUI::Ico_Error);
			return false;
		}
		if (!Grp.Delete(C4CFN_Title))
		{
			StdStrBuf sMsg; sMsg.Format(LoadResStr("IDS_ERR_DELOLDTITLE"), sFilename.getData(), Grp.GetError());
			Game.pGUI->ShowMessageModal(sMsg.getData(), strErr.getData(), C4GUI::MessageDialog::btnOK, C4GUI::Ico_Error);
			return false;
		}
		if (!SetTitleInGroup(Grp, szNewName)) return false;
		if (!Grp.Close())
		{
			StdStrBuf sMsg; sMsg.Format(LoadResStr("IDS_ERR_WRITENEWTITLE"), sFilename.getData(), Grp.GetError());
			Game.pGUI->ShowMessageModal(sMsg.getData(), strErr.getData(), C4GUI::MessageDialog::btnOK, C4GUI::Ico_Error);
			return false;
		}
	}
	// update title
	sName.Copy(szNewName);
	// done
	return true;
}

bool C4ScenarioListLoader::Entry::SetTitleInGroup(C4Group &rGrp, const char *szNewTitle)
{
	// default for group files: Create a title text file and set the title in there
	// no title needed if filename is sufficient - except for scenarios, where a Scenario.txt could overwrite the title
	if (!IsScenario())
	{
		StdStrBuf sNameByFile; sNameByFile.Copy(GetFilename(sFilename.getData()));
		char *szBuf = sNameByFile.GrabPointer();
		RemoveExtension(szBuf);
		sNameByFile.Take(szBuf);
		if (SEqual(szNewTitle, sNameByFile.getData())) return true;
	}
	// okay, make a title
	StdStrBuf sTitle; sTitle.Format("%.2s:%s", Config.General.Language, szNewTitle);
	if (!rGrp.Add(C4CFN_WriteTitle, sTitle, false, true))
	{
		StdStrBuf sMsg; sMsg.Format(LoadResStr("IDS_ERR_ERRORADDINGNEWTITLEFORFIL"), sFilename.getData(), rGrp.GetError());
		Game.pGUI->ShowMessageModal(sMsg.getData(), LoadResStr("IDS_FAIL_RENAME"), C4GUI::MessageDialog::btnOK, C4GUI::Ico_Error);
		return false;
	}
	return true;
}

// Scenario

bool C4ScenarioListLoader::Scenario::LoadCustomPre(C4Group &rGrp)
{
	// load scenario core first
	StdStrBuf sFileContents;
	if (!rGrp.LoadEntryString(C4CFN_ScenarioCore, sFileContents)) return false;
	if (!CompileFromBuf_LogWarn<StdCompilerINIRead>(mkParAdapt(C4S, false), sFileContents, (rGrp.GetFullName() + DirSep C4CFN_ScenarioCore).getData()))
		return false;
	return true;
}

bool C4ScenarioListLoader::Scenario::LoadCustom(C4Group &rGrp, bool fNameLoaded, bool fIconLoaded)
{
	// icon fallback: Standard scenario icon
	if (!fIconLoaded)
	{
		iIconIndex = C4S.Head.Icon;
		int32_t iIcon = C4S.Head.Icon;
		if (!Inside<int32_t>(iIcon, 0, C4StartupScenSel_IconCount - 1)) iIcon = C4StartupScenSel_DefaultIcon_Scenario;
		fctIcon.Set(C4Startup::Get()->Graphics.fctScenSelIcons.GetSection(iIcon));
	}
	// scenario name fallback to core
	if (!fNameLoaded)
		sName.Copy(C4S.Head.Title);
	// difficulty: Set only for regular rounds (not savegame or record) to avoid bogus sorting
	if (!C4S.Head.SaveGame && !C4S.Head.Replay)
		iDifficulty = C4S.Head.Difficulty;
	else
		iDifficulty = 0;

	// minimum required player count
	iMinPlrCount = C4S.GetMinPlayer();
	return true;
}

bool C4ScenarioListLoader::Scenario::Start()
{
	// gogo!
	if (!(C4StartupScenSelDlg::pInstance)) return false;
	return (C4StartupScenSelDlg::pInstance)->StartScenario(this);
}

bool C4ScenarioListLoader::Scenario::CanOpen(StdStrBuf &sErrOut)
{
	// safety
	C4StartupScenSelDlg *pDlg = C4StartupScenSelDlg::pInstance;
	if (!pDlg) return false;
	// check mission access
	if (C4S.Head.MissionAccess[0] && !SIsModule(Config.General.MissionAccess, C4S.Head.MissionAccess))
	{
		sErrOut.Copy(LoadResStr("IDS_PRC_NOMISSIONACCESS"));
		return false;
	}
	// replay
	if (C4S.Head.Replay)
	{
		// replays can currently not be launched in network mode
		if (pDlg->IsNetworkStart())
		{
			sErrOut.Copy(LoadResStr("IDS_PRC_NONETREPLAY"));
			return false;
		}
	}
	// regular game
	else
	{
		// check player count
		int32_t iPlrCount = SModuleCount(Config.General.Participants);
		int32_t iMaxPlrCount = C4S.Head.MaxPlayer;
		if (C4S.Head.SaveGame)
		{
			// Some scenarios have adjusted MaxPlayerCount to 0 after starting to prevent future joins
			// make sure it's possible to start the savegame anyway
			iMaxPlrCount = std::max<int32_t>(iMinPlrCount, iMaxPlrCount);
		}
		// normal scenarios: At least one player except in network mode, where it is possible to wait for the additional players
		// Melees need at least two
		if ((iPlrCount < iMinPlrCount))
		{
			if (pDlg->IsNetworkStart())
			{
				// network game: Players may yet join in lobby
				// only issue a warning for too few players (by setting the error but not returning false here)
				sErrOut.Format(LoadResStr("IDS_MSG_TOOFEWPLAYERSNET"), static_cast<int>(iMinPlrCount));
			}
			else
			{
				// for regular games, this is a fatal no-start-cause
				sErrOut.Format(LoadResStr("IDS_MSG_TOOFEWPLAYERS"), static_cast<int>(iMinPlrCount));
				return false;
			}
		}
		// scenarios (both normal and savegame) may also impose a maximum player restriction
		if (iPlrCount > iMaxPlrCount)
		{
			sErrOut.Format(LoadResStr("IDS_MSG_TOOMANYPLAYERS"), static_cast<int>(C4S.Head.MaxPlayer));
			return false;
		}
	}
	// Okay, start!
	return true;
}

StdStrBuf C4ScenarioListLoader::Scenario::GetOpenText()
{
	return StdStrBuf(LoadResStr("IDS_BTN_STARTGAME"));
}

StdStrBuf C4ScenarioListLoader::Scenario::GetOpenTooltip()
{
	return StdStrBuf(LoadResStr("IDS_DLGTIP_SCENSELNEXT"));
}

// Folder

C4ScenarioListLoader::Folder::~Folder()
{
	delete pMapData;
	ClearChildren();
}

bool C4ScenarioListLoader::Folder::Start()
{
	// open as subfolder
	if (!C4StartupScenSelDlg::pInstance) return false;
	return C4StartupScenSelDlg::pInstance->OpenFolder(this);
}

int
#ifdef _MSC_VER
__cdecl
#endif
EntrySortFunc(const void *pEl1, const void *pEl2)
{
	C4ScenarioListLoader::Entry *pEntry1 = *static_cast<C4ScenarioListLoader::Entry * const *>(pEl1), *pEntry2 = *static_cast<C4ScenarioListLoader::Entry * const *>(pEl2);
	// sort folders before scenarios
	bool fS1, fS2;
	if (!(fS1 = !pEntry1->GetIsFolder()) != !true != !(fS2 = !pEntry2->GetIsFolder())) return fS1 - fS2;
	// sort by folder index (undefined index 0 goes to the end)
	if (!Config.Startup.AlphabeticalSorting) if (pEntry1->GetFolderIndex() || pEntry2->GetFolderIndex())
	{
		if (!pEntry1->GetFolderIndex()) return +1;
		if (!pEntry2->GetFolderIndex()) return -1;
		int32_t iDiff = pEntry1->GetFolderIndex() - pEntry2->GetFolderIndex();
		if (iDiff) return iDiff;
	}
	// sort by numbered standard scenario icons
	if (Inside(pEntry1->GetIconIndex(), 2, 11))
	{
		int32_t iDiff = pEntry1->GetIconIndex() - pEntry2->GetIconIndex();
		if (iDiff) return iDiff;
	}
	// sort by difficulty (undefined difficulty goes to the end)
	if (!Config.Startup.AlphabeticalSorting) if (pEntry1->GetDifficulty() || pEntry2->GetDifficulty())
	{
		if (!pEntry1->GetDifficulty()) return +1;
		if (!pEntry2->GetDifficulty()) return -1;
		int32_t iDiff = pEntry1->GetDifficulty() - pEntry2->GetDifficulty();
		if (iDiff) return iDiff;
	}
	// otherwise, sort by name
	return stricmp(pEntry1->GetName().getData(), pEntry2->GetName().getData());
}

uint32_t C4ScenarioListLoader::Folder::GetEntryCount() const
{
	uint32_t iCount = 0;
	for (Entry *i = pFirst; i; i = i->pNext) ++iCount;
	return iCount;
}

void C4ScenarioListLoader::Folder::Sort()
{
	// use C-Library-QSort on a buffer of entry pointers; then re-link list
	if (!pFirst) return;
	uint32_t iCount, i;
	Entry **ppEntries = new Entry *[i = iCount = GetEntryCount()], **ppI, *pI = pFirst, **ppIThis;
	for (ppI = ppEntries; i--; pI = pI->pNext) * ppI++ = pI;
	qsort(ppEntries, iCount, sizeof(Entry *), &EntrySortFunc);
	ppIThis = &pFirst;
	for (ppI = ppEntries; iCount--; ppIThis = &((*ppIThis)->pNext)) * ppIThis = *ppI++;
	*ppIThis = nullptr;
	delete[] ppEntries;
}

void C4ScenarioListLoader::Folder::ClearChildren()
{
	// folder deletion: del all the tree non-recursively
	Folder *pDelFolder = this, *pCheckFolder;
	for (;;)
	{
		// delete all children as long as they are not folders
		Entry *pChild;
		while (pChild = pDelFolder->pFirst)
			if (pCheckFolder = pChild->GetIsFolder())
				// child entry if folder: Continue delete in there
				pDelFolder = pCheckFolder;
			else
				// regular child entry: del it
				// destructor of child will remove it from list
				delete pChild;
		// this emptied: Done!
		if (pDelFolder == this) break;
		// deepest child recursion reached: Travel up folders
		pDelFolder = (pCheckFolder = pDelFolder)->pParent;
		assert(pDelFolder);
		delete pCheckFolder;
	}
}

bool C4ScenarioListLoader::Folder::LoadContents(C4ScenarioListLoader *pLoader, C4Group *pFromGrp, const StdStrBuf *psFilename, bool fLoadEx, bool fReload)
{
	// contents already loaded?
	if (fContentsLoaded && !fReload) return true;
	// clear previous
	delete pMapData; pMapData = nullptr;
	// if filename is not given, assume it's been loaded in this entry
	if (!psFilename) psFilename = &this->sFilename; else this->sFilename.Ref(*psFilename);
	// nothing loaded: Load now
	if (!DoLoadContents(pLoader, pFromGrp, *psFilename, fLoadEx)) return false;
	// sort loaded stuff by name
	Sort();
	return true;
}

C4ScenarioListLoader::Entry *C4ScenarioListLoader::Folder::FindEntryByName(const char *szFilename) const
{
	// do a case-insensitive filename comparison
	for (Entry *pEntry = pFirst; pEntry; pEntry = pEntry->GetNext())
		if (SEqualNoCase(szFilename, GetFilename(pEntry->GetEntryFilename().getData())))
			return pEntry;
	// nothing found
	return nullptr;
}

StdStrBuf C4ScenarioListLoader::Folder::GetOpenText()
{
	return StdStrBuf(LoadResStr("IDS_BTN_OPEN"));
}

StdStrBuf C4ScenarioListLoader::Folder::GetOpenTooltip()
{
	return StdStrBuf(LoadResStr("IDS_DLGTIP_SCENSELNEXT"));
}

bool C4ScenarioListLoader::Folder::LoadCustomPre(C4Group &rGrp)
{
	// load folder core if available
	StdStrBuf sFileContents;
	if (rGrp.LoadEntryString(C4CFN_FolderCore, sFileContents))
		if (!CompileFromBuf_LogWarn<StdCompilerINIRead>(C4F, sFileContents, (rGrp.GetFullName() + DirSep C4CFN_FolderCore).getData()))
			return false;
	return true;
}

// SubFolder

bool C4ScenarioListLoader::SubFolder::LoadCustom(C4Group &rGrp, bool fNameLoaded, bool fIconLoaded)
{
	// default icon fallback
	if (!fIconLoaded)
		fctIcon.Set(C4Startup::Get()->Graphics.fctScenSelIcons.GetSection(C4StartupScenSel_DefaultIcon_Folder));
	// folder index
	iFolderIndex = C4F.Head.Index;
	return true;
}

bool C4ScenarioListLoader::SubFolder::DoLoadContents(C4ScenarioListLoader *pLoader, C4Group *pFromGrp, const StdStrBuf &sFilename, bool fLoadEx)
{
	assert(pLoader);
	// clear any previous
	ClearChildren();
	// group specified: Load as child
	C4Group Group;
	if (pFromGrp)
	{
		if (!Group.OpenAsChild(pFromGrp, sFilename.getData())) return false;
	}
	else
		// no parent group: Direct load from filename
		if (!Group.Open(sFilename.getData())) return false;
	// get number of entries, to estimate progress
	const char *szC4CFN_ScenarioFiles = C4CFN_ScenarioFiles; // assign values for constant comparison
	const char *szSearchMask; int32_t iEntryCount = 0;
	for (szSearchMask = szC4CFN_ScenarioFiles; szSearchMask;)
	{
		Group.ResetSearch();
		while (Group.FindNextEntry(szSearchMask)) ++iEntryCount;
		// next search mask
		if (szSearchMask == szC4CFN_ScenarioFiles)
			szSearchMask = C4CFN_FolderFiles;
		else
			szSearchMask = nullptr;
	}
	// initial progress estimate
	if (!pLoader->DoProcessCallback(0, iEntryCount)) return false;
	// iterate through group contents
	char ChildFilename[_MAX_FNAME + 1]; StdStrBuf sChildFilename; int32_t iLoadCount = 0;
	for (szSearchMask = szC4CFN_ScenarioFiles; szSearchMask;)
	{
		Group.ResetSearch();
		while (Group.FindNextEntry(szSearchMask, ChildFilename))
		{
			sChildFilename.Ref(ChildFilename);
			// okay; create this item
			Entry *pNewEntry = Entry::CreateEntryForFile(sChildFilename, this);
			if (pNewEntry)
			{
				// ...and load it
				if (!pNewEntry->Load(&Group, &sChildFilename, fLoadEx))
				{
					DebugLogF("Error loading entry \"%s\" in SubFolder \"%s\"!", sChildFilename.getData(), Group.GetFullName().getData());
					delete pNewEntry;
				}
			}
			// mark progress
			if (!pLoader->DoProcessCallback(++iLoadCount, iEntryCount)) return false;
		}
		// next search mask
		if (szSearchMask == szC4CFN_ScenarioFiles)
			szSearchMask = C4CFN_FolderFiles;
		else
			szSearchMask = nullptr;
	}
	// load map folder data
	if (Config.Graphics.ShowFolderMaps && Group.FindEntry(C4CFN_MapFolderData))
	{
		pMapData = new C4MapFolderData();
		if (!pMapData->Load(Group, this))
		{
			// load error :(
			delete pMapData;
			pMapData = nullptr;
		}
	}
	// done, success
	fContentsLoaded = true;
	return true;
}

// RegularFolder

void C4ScenarioListLoader::RegularFolder::Merge(const fs::path &path)
{
	contents.emplace_back(path);
}

bool C4ScenarioListLoader::RegularFolder::LoadCustom(C4Group &rGrp, bool fNameLoaded, bool fIconLoaded)
{
	// default icon fallback
	if (!fIconLoaded)
		fctIcon.Set(C4Startup::Get()->Graphics.fctScenSelIcons.GetSection(C4StartupScenSel_DefaultIcon_WinFolder));
	// folder index
	iFolderIndex = C4F.Head.Index;
	return true;
}

bool C4ScenarioListLoader::RegularFolder::DoLoadContents(C4ScenarioListLoader *pLoader, C4Group *pFromGrp, const StdStrBuf &sFilename, bool fLoadEx)
{
	// clear any previous
	ClearChildren();
	// regular folders must exist and not be within group!
	assert(!pFromGrp);
	if (sFilename.getData() && sFilename[0])
	{
		Merge(sFilename.getData());
	}

	// get number of entries, to estimate progress
	int32_t iCountLoaded = 0, iCountTotal = 0;

	for (const auto &path : contents)
	{
		if (!DirectoryExists(path))
		{
			continue;
		}

		iCountTotal += static_cast<int32_t>(std::count_if(fs::directory_iterator{path}, fs::directory_iterator{}, [](const auto &item)
		{
			return item.path().string()[0] != '.';
		}));
	}
	// initial progress estimate
	if (!pLoader->DoProcessCallback(iCountLoaded, iCountTotal)) return false;

	// do actual loading of files
	std::set<std::string> names;

	for (const auto &path : contents)
	{
		if (!pLoader->DoProcessCallback(iCountLoaded, iCountTotal)) return false;
		for (const auto &item : fs::directory_iterator{path})
		{
			StdStrBuf childPathBuf{item.path().string().c_str()};
			const std::string childFileName{item.path().filename().string()};

			// Ignore directory navigation entries and CVS folders
			if (C4Group_TestIgnore(childFileName.c_str()))
			{
				continue;
			}

			if (names.find(childFileName) != std::cend(names))
			{
				continue;
			}

			names.insert(childFileName);

			// filename okay; create this item
			std::unique_ptr<Entry> newEntry{Entry::CreateEntryForFile(childPathBuf, this)};
			if (newEntry)
			{
				if (!newEntry->Load(nullptr, &childPathBuf, fLoadEx))
				{
					// ...and load it
					DebugLogF("Error loading entry \"%s\" in Folder \"%s\"!", childFileName.c_str(), path.string().c_str());
				}
				else
				{
					newEntry.release();
				}
			}

			if (!pLoader->DoProcessCallback(++iCountLoaded, iCountTotal)) return false;
		}
	}

	// done, success
	fContentsLoaded = true;
	return true;
}

// C4ScenarioListLoader

C4ScenarioListLoader::C4ScenarioListLoader() : pRootFolder(nullptr), pCurrFolder(nullptr),
	iLoading(0), iProgress(0), iMaxProgress(0), fAbortThis(false), fAbortPrevious(false) {}

C4ScenarioListLoader::~C4ScenarioListLoader()
{
	delete pRootFolder;
}

bool C4ScenarioListLoader::BeginActivity(bool fAbortPrevious)
{
	// if previous activities were running, stop them first if desired
	if (iLoading && fAbortPrevious)
		this->fAbortPrevious = true;
	// mark this activity
	++iLoading;
	// progress of activity not yet decided
	iProgress = iMaxProgress = 0;
	// okay; start activity
	return true;
}

void C4ScenarioListLoader::EndActivity()
{
	assert(iLoading);
	if (!--iLoading)
	{
		// last activity done: Reset any flags
		fAbortThis = false;
		fAbortPrevious = false;
		iProgress = iMaxProgress = 0;
	}
	else
	{
		// child activity done: Transfer abort flag for next activity
		fAbortThis = fAbortPrevious;
	}
}

bool C4ScenarioListLoader::DoProcessCallback(int32_t iProgress, int32_t iMaxProgress)
{
	this->iProgress = iProgress;
	this->iMaxProgress = iMaxProgress;
	// callback to dialog
	if (C4StartupScenSelDlg::pInstance) C4StartupScenSelDlg::pInstance->ProcessCallback();
	auto checkTimer = false;
	const auto now = timeGetTime();
	// limit to real 10 FPS, as it may unnecessarily slow down loading a lot otherwise
	if (lastCheckTimer == 0 || now - lastCheckTimer > 100)
	{
		checkTimer = true;
		lastCheckTimer = now;
	}
	// process callback - abort at a few ugly circumstances...
	if (Application.HandleMessage(0, checkTimer) == HR_Failure // WM_QUIT message?
		|| !C4StartupScenSelDlg::pInstance // host dialog removed?
		|| !C4StartupScenSelDlg::pInstance->IsShown() // host dialog closed?
		) return false;
	// and also abort if flagged
	return !fAbortThis;
}

bool C4ScenarioListLoader::Load(const StdStrBuf &sRootFolder)
{
	// (unthreaded) loading of all entries in root folder
	if (!BeginActivity(true)) return false;
	delete pRootFolder; pRootFolder = nullptr;
	pCurrFolder = pRootFolder = new RegularFolder(nullptr);
	// Load regular game data if no explicit path specified
	if(!sRootFolder.getData())
	{
		for (const auto &pathInfo : Reloc)
		{
			static_cast<RegularFolder *>(pRootFolder)->Merge(pathInfo.Path);
		}
	}

	bool fSuccess = pRootFolder->LoadContents(this, nullptr, &sRootFolder, false, false);
	EndActivity();
	return fSuccess;
}

bool C4ScenarioListLoader::Load(Folder *pSpecifiedFolder, bool fReload)
{
	// call safety
	if (!pRootFolder || !pSpecifiedFolder) return false;
	// set new current and load it
	if (!BeginActivity(true)) return false;
	pCurrFolder = pSpecifiedFolder;
	bool fSuccess = pCurrFolder->LoadContents(this, nullptr, nullptr, false, fReload);
	EndActivity();
	return fSuccess;
}

bool C4ScenarioListLoader::LoadExtended(Entry *pEntry)
{
	// call safety
	if (!pRootFolder || !pEntry) return false;
	// load info of selection
	if (!BeginActivity(false)) return false;
	bool fSuccess = pEntry->Load(nullptr, nullptr, true);
	EndActivity();
	return fSuccess;
}

bool C4ScenarioListLoader::FolderBack()
{
	// call safety
	if (!pRootFolder || !pCurrFolder) return false;
	// already in root: Can't go up
	if (pCurrFolder == pRootFolder) return false;
	// otherwise, up one level
	return Load(pCurrFolder->GetParent(), false);
}

bool C4ScenarioListLoader::ReloadCurrent()
{
	// call safety
	if (!pRootFolder || !pCurrFolder) return false;
	// reload current
	return Load(pCurrFolder, true);
}

// Scenario selection GUI

// font clrs
const uint32_t ClrScenarioItem         = 0xff000000,
               ClrScenarioItemXtra     = 0x7f000000,
               ClrScenarioItemDisabled = 0x7f000000;

// C4StartupScenSelDlg::ScenListItem
C4StartupScenSelDlg::ScenListItem::ScenListItem(C4GUI::ListBox *pForListBox, C4ScenarioListLoader::Entry *pForEntry, C4GUI::Element *pInsertBeforeElement)
	: pIcon(nullptr), pNameLabel(nullptr), pScenListEntry(pForEntry)
{
	assert(pScenListEntry);
	CStdFont &rUseFont = C4Startup::Get()->Graphics.BookFont;
	StdStrBuf sIgnore;
	bool fEnabled = pScenListEntry->CanOpen(sIgnore);
	// calc height
	int32_t iHeight = rUseFont.GetLineHeight() + 2 * IconLabelSpacing;
	// create subcomponents
	pIcon = new C4GUI::Picture(C4Rect(0, 0, iHeight, iHeight), true);
	pIcon->SetFacet(pScenListEntry->GetIconFacet());
	StdStrBuf name = pScenListEntry->GetName();
	CMarkup::StripMarkup(&name);
	pNameLabel = new C4GUI::Label(name.getData(), iHeight + IconLabelSpacing, IconLabelSpacing, ALeft, fEnabled ? ClrScenarioItem : ClrScenarioItemDisabled, &rUseFont, false, false);
	// calc own bounds - use icon bounds only, because only the height is used when the item is added
	SetBounds(pIcon->GetBounds());
	// add components
	AddElement(pIcon); AddElement(pNameLabel);
	// tooltip by name, so long names can be read via tooltip
	SetToolTip(pScenListEntry->GetName().getData());
	// add to listbox (will get resized horizontally and moved) - zero indent; no tree structure in this dialog
	pForListBox->InsertElement(this, pInsertBeforeElement, 0);
	// update name label width to reflect new horizontal size
	// name label width must be set so rename edit will take its size
	pNameLabel->SetAutosize(false);
	C4Rect rcNLB = pNameLabel->GetBounds(); rcNLB.Wdt = GetClientRect().Wdt - rcNLB.x - IconLabelSpacing;
	pNameLabel->SetBounds(rcNLB);
}

void C4StartupScenSelDlg::ScenListItem::UpdateOwnPos()
{
	// parent for client rect
	typedef C4GUI::Window ParentClass;
	ParentClass::UpdateOwnPos();
	// reposition items
	C4GUI::ComponentAligner caBounds(GetContainedClientRect(), IconLabelSpacing, IconLabelSpacing);
	// nothing to reposition for now...
}

void C4StartupScenSelDlg::ScenListItem::MouseInput(C4GUI::CMouse &rMouse, int32_t iButton, int32_t iX, int32_t iY, uint32_t dwKeyParam)
{
	// inherited processing
	typedef C4GUI::Window BaseClass;
	BaseClass::MouseInput(rMouse, iButton, iX, iY, dwKeyParam);
}

bool C4StartupScenSelDlg::ScenListItem::CheckNameHotkey(const char *c)
{
	// return whether this item can be selected by entering given char:
	// first char of name must match
	// FIXME: make unicode-ready
	if (!pScenListEntry) return false;
	const char *szName = pScenListEntry->GetName().getData();
	return szName && (toupper(*szName) == toupper(c[0]));
}

bool C4StartupScenSelDlg::ScenListItem::KeyRename()
{
	// rename this entry
	C4StartupScenSelDlg::pInstance->StartRenaming(new C4GUI::CallbackRenameEdit<C4StartupScenSelDlg::ScenListItem, RenameParams>(pNameLabel, this, RenameParams(), &C4StartupScenSelDlg::ScenListItem::DoRenaming, &C4StartupScenSelDlg::ScenListItem::AbortRenaming));
	return true;
}

void C4StartupScenSelDlg::ScenListItem::AbortRenaming(RenameParams par)
{
	// no renaming
	C4StartupScenSelDlg::pInstance->SetRenamingDone();
}

C4GUI::RenameEdit::RenameResult C4StartupScenSelDlg::ScenListItem::DoRenaming(RenameParams par, const char *szNewName)
{
	// check validity for new name
	if (!GetEntry()->RenameTo(szNewName)) return C4GUI::RenameEdit::RR_Invalid;
	// rename label
	pNameLabel->SetText(GetEntry()->GetName().getData());
	// main dlg update
	C4StartupScenSelDlg::pInstance->SetRenamingDone();
	C4StartupScenSelDlg::pInstance->ResortFolder();
	C4StartupScenSelDlg::pInstance->UpdateSelection();
	C4StartupScenSelDlg::pInstance->FocusScenList();
	// done; rename accepted and control deleted by ResortFolder
	return C4GUI::RenameEdit::RR_Deleted;
}

// C4StartupScenSelDlg

C4StartupScenSelDlg::C4StartupScenSelDlg(bool fNetwork) : C4StartupDlg(LoadResStrNoAmp(fNetwork ? "IDS_DLG_NETSTART" : "IDS_DLG_STARTGAME")), pScenLoader(nullptr), fIsInitialLoading(false), fStartNetworkGame(fNetwork), pMapData(nullptr), pRenameEdit(nullptr), pfctBackground(nullptr), btnAllowUserChange{nullptr}
{
	// assign singleton
	pInstance = this;

	// screen calculations
	UpdateSize();
	int32_t iButtonWidth, iCaptionFontHgt;
	int iButtonHeight = C4GUI_ButtonHgt;
	int iBookPageWidth;
	int iExtraHPadding = rcBounds.Wdt >= 700 ? rcBounds.Wdt / 50 : 0;
	int iExtraVPadding = rcBounds.Hgt >= 540 ? rcBounds.Hgt / 20 : 0;
	C4GUI::GetRes()->CaptionFont.GetTextExtent("<< BACK", iButtonWidth, iCaptionFontHgt, true);
	iButtonWidth *= 3;
	C4GUI::ComponentAligner caMain(GetClientRect(), 0, 0, true);
	C4GUI::ComponentAligner caButtonArea(caMain.GetFromBottom(caMain.GetHeight() / 8), rcBounds.Wdt / (rcBounds.Wdt >= 700 ? 128 : 256), 0);
	C4Rect rcMap = caMain.GetCentered(caMain.GetWidth(), caMain.GetHeight());
	int iYOversize = caMain.GetHeight() / 10; // overlap of map to top
	rcMap.y -= iYOversize; rcMap.Hgt += iYOversize;
	C4GUI::ComponentAligner caMap(rcMap, 0, 0, true);
	caMap.ExpandTop(-iYOversize);
	C4GUI::ComponentAligner caBook(caMap.GetCentered(caMap.GetWidth() * 11 / 12 - 4 * iExtraHPadding, caMap.GetHeight()), rcBounds.Wdt / 30, iExtraVPadding, false);
	C4GUI::ComponentAligner caBookLeft(caBook.GetFromLeft(iBookPageWidth = caBook.GetWidth() * 4 / 9 + 4 - iExtraHPadding * 2), 0, 5);

	// tabular for different scenario selection designs
	pScenSelStyleTabular = new C4GUI::Tabular(rcMap, C4GUI::Tabular::tbNone);
	pScenSelStyleTabular->SetSheetMargin(0);
	pScenSelStyleTabular->SetDrawDecoration(false);
	AddElement(pScenSelStyleTabular);
	C4GUI::Tabular::Sheet *pSheetBook = pScenSelStyleTabular->AddSheet(nullptr);
	// map sheet; later queried via GetSheet(ShowStyle_Map)
	pScenSelStyleTabular->AddSheet(nullptr);

	// scenario selection list
	CStdFont &rScenSelCaptionFont = C4Startup::Get()->Graphics.BookFontTitle;
	pScenSelCaption = new C4GUI::Label("", caBookLeft.GetFromTop(rScenSelCaptionFont.GetLineHeight()), ACenter, ClrScenarioItem, &rScenSelCaptionFont, false);
	pSheetBook->AddElement(pScenSelCaption);
	pScenSelCaption->SetToolTip(LoadResStr("IDS_DLGTIP_SELECTSCENARIO"));

	// search bar
	const char *labelText = LoadResStr("IDS_DLG_SEARCH");
	int32_t width = 100, height;
	C4GUI::GetRes()->TextFont.GetTextExtent(labelText, width, height, true);
	C4GUI::ComponentAligner caSearchBar(caBookLeft.GetFromBottom(height), 0, 0);
	auto *searchLabel = new C4GUI::WoodenLabel(labelText, caSearchBar.GetFromLeft(width + 10), C4GUI_Caption2FontClr, &C4GUI::GetRes()->TextFont);
	searchLabel->SetToolTip(LoadResStr("IDS_DLGTIP_SEARCHLIST"));
	pSheetBook->AddElement(searchLabel);

	searchBar = new C4GUI::CallbackEdit<C4StartupScenSelDlg>(caSearchBar.GetAll(), this, &C4StartupScenSelDlg::OnSearchBarEnter);
	searchBar->SetToolTip(LoadResStr("IDS_DLGTIP_SEARCHLIST"));
	pSheetBook->AddElement(searchBar);

	// scenario selection list box
	pScenSelList = new C4GUI::ListBox(caBookLeft.GetAll());
	pScenSelList->SetToolTip(LoadResStr("IDS_DLGTIP_SELECTSCENARIO"));
	pScenSelList->SetDecoration(false, &C4Startup::Get()->Graphics.sfctBookScroll, true);
	pSheetBook->AddElement(pScenSelList);
	pScenSelList->SetSelectionChangeCallbackFn(new C4GUI::CallbackHandler<C4StartupScenSelDlg>(this, &C4StartupScenSelDlg::OnSelChange));
	pScenSelList->SetSelectionDblClickFn(new C4GUI::CallbackHandler<C4StartupScenSelDlg>(this, &C4StartupScenSelDlg::OnSelDblClick));
	// scenario selection list progress label
	pScenSelProgressLabel = new C4GUI::Label("", pScenSelList->GetBounds().GetMiddleX(), pScenSelList->GetBounds().GetMiddleX() - iCaptionFontHgt / 2, ACenter, ClrScenarioItem, &(C4Startup::Get()->Graphics.BookFontCapt), false);
	pSheetBook->AddElement(pScenSelProgressLabel);

	// right side of book: Displaying current selection
	pSelectionInfo = new C4GUI::TextWindow(caBook.GetFromRight(iBookPageWidth), C4StartupScenSel_TitlePictureWdt + 2 * C4StartupScenSel_TitleOverlayMargin, C4StartupScenSel_TitlePictureHgt + 2 * C4StartupScenSel_TitleOverlayMargin,
		C4StartupScenSel_TitlePicturePadding, 100, 4096, nullptr, true, &C4Startup::Get()->Graphics.fctScenSelTitleOverlay, C4StartupScenSel_TitleOverlayMargin);
	pSelectionInfo->SetDecoration(false, false, &C4Startup::Get()->Graphics.sfctBookScroll, true);
	pSheetBook->AddElement(pSelectionInfo);

	// back button
	C4GUI::CallbackButton<C4StartupScenSelDlg> *btn;
	AddElement(btn = new C4GUI::CallbackButton<C4StartupScenSelDlg>(LoadResStr("IDS_BTN_BACK"), caButtonArea.GetFromLeft(iButtonWidth, iButtonHeight), &C4StartupScenSelDlg::OnBackBtn));
	btn->SetToolTip(LoadResStr("IDS_DLGTIP_BACKMAIN"));
	AddElement(btn);
	// next button
	pOpenBtn = new C4GUI::CallbackButton<C4StartupScenSelDlg>(LoadResStr("IDS_BTN_OPEN"), caButtonArea.GetFromRight(iButtonWidth, iButtonHeight), &C4StartupScenSelDlg::OnNextBtn);
	pOpenBtn->SetToolTip(LoadResStr("IDS_DLGTIP_SCENSELNEXT"));

	// allow user change button
	AddElement(btnAllowUserChange = new C4GUI::CheckBox(caButtonArea.GetFromRight(iButtonWidth, iButtonHeight), LoadResStr("IDS_DLG_ALLOWUSERCHANGE"), false));

	// game options boxes
	pGameOptionButtons = new C4GameOptionButtons(caButtonArea.GetAll(), fNetwork, true, false);
	AddElement(pGameOptionButtons);
	// next button adding
	AddElement(pOpenBtn);

	// dlg starts with focus on listbox
	SetFocus(pScenSelList, false);

	// key bindings
	keyBack.reset(new C4KeyBinding(C4KeyCodeEx(K_LEFT), "StartupScenSelFolderUp", KEYSCOPE_Gui,
		new C4GUI::DlgKeyCB<C4StartupScenSelDlg>(*this, &C4StartupScenSelDlg::KeyBack), C4CustomKey::PRIO_CtrlOverride));
	keyRefresh.reset(new C4KeyBinding(C4KeyCodeEx(K_F5), "StartupScenSelReload", KEYSCOPE_Gui,
		new C4GUI::DlgKeyCB<C4StartupScenSelDlg>(*this, &C4StartupScenSelDlg::KeyRefresh), C4CustomKey::PRIO_CtrlOverride));
	keyForward.reset(new C4KeyBinding(C4KeyCodeEx(K_RIGHT), "StartupScenSelNext", KEYSCOPE_Gui,
		new C4GUI::DlgKeyCB<C4StartupScenSelDlg>(*this, &C4StartupScenSelDlg::KeyForward), C4CustomKey::PRIO_CtrlOverride));
	keyRename.reset(new C4KeyBinding(C4KeyCodeEx(K_F2), "StartupScenSelRename", KEYSCOPE_Gui,
		new C4GUI::ControlKeyDlgCB<C4StartupScenSelDlg>(pScenSelList, *this, &C4StartupScenSelDlg::KeyRename), C4CustomKey::PRIO_CtrlOverride));
	keyDelete.reset(new C4KeyBinding(C4KeyCodeEx(K_DELETE), "StartupScenSelDelete", KEYSCOPE_Gui,
		new C4GUI::ControlKeyDlgCB<C4StartupScenSelDlg>(pScenSelList, *this, &C4StartupScenSelDlg::KeyDelete), C4CustomKey::PRIO_CtrlOverride));
	keyCheat.reset(new C4KeyBinding(C4KeyCodeEx(KEY_M, KEYS_Alt), "StartupScenSelCheat", KEYSCOPE_Gui,
		new C4GUI::ControlKeyDlgCB<C4StartupScenSelDlg>(pScenSelList, *this, &C4StartupScenSelDlg::KeyCheat), C4CustomKey::PRIO_CtrlOverride));
	keySearch.reset(new C4KeyBinding(C4KeyCodeEx(KEY_F, KEYS_Control), "StartupScenSelSearch", KEYSCOPE_Gui,
		new C4GUI::ControlKeyDlgCB<C4StartupScenSelDlg>(pScenSelList, *this, &C4StartupScenSelDlg::KeySearch), C4CustomKey::PRIO_CtrlOverride));
	keyEscape.reset(new C4KeyBinding(C4KeyCodeEx(K_ESCAPE), "StartupScenSelEscape", KEYSCOPE_Gui,
		new C4GUI::ControlKeyDlgCB<C4StartupScenSelDlg>(pScenSelList, *this, &C4StartupScenSelDlg::KeyEscape), C4CustomKey::PRIO_CtrlOverride));
}

C4StartupScenSelDlg::~C4StartupScenSelDlg()
{
	delete pScenLoader;
	if (this == pInstance) pInstance = nullptr;
}

void C4StartupScenSelDlg::DrawElement(C4FacetEx &cgo)
{
	// draw background
	if (pfctBackground)
		DrawBackground(cgo, *pfctBackground);
	else
		DrawBackground(cgo, C4Startup::Get()->Graphics.fctScenSelBG);
}

void C4StartupScenSelDlg::HideTitle(bool hide)
{
	if (hide == !pFullscreenTitle)
	{
		return;
	}

	FullscreenDialog::SetTitle(hide ? "" : LoadResStrNoAmp(fStartNetworkGame ? "IDS_DLG_NETSTART" : "IDS_DLG_STARTGAME"));
}

void C4StartupScenSelDlg::OnShown()
{
	C4StartupDlg::OnShown();
	// init file list
	fIsInitialLoading = true;
	if (!pScenLoader) pScenLoader = new C4ScenarioListLoader();
	pScenLoader->Load(StdStrBuf{});
	UpdateList();
	UpdateSelection();
	fIsInitialLoading = false;
	// network activation by dialog type
	Game.NetworkActive = fStartNetworkGame;
}

void C4StartupScenSelDlg::OnClosed(bool fOK)
{
	AbortRenaming();
	// clear laoded scenarios
	if (pScenLoader)
	{
		delete pScenLoader;
		pScenLoader = nullptr;
		UpdateList(); // must clear scenario list, because it points to deleted stuff
		UpdateSelection(); // must clear picture facet of selection!
	}
	// dlg abort: return to main screen
	if (!fOK)
	{
		// clear settings: Password
		Game.Network.SetPassword(nullptr);
		C4Startup::Get()->SwitchDialog(C4Startup::SDID_Back);
	}
}

void C4StartupScenSelDlg::UpdateUseCrewBtn()
{
	C4ScenarioListLoader::Entry *pSel = GetSelectedEntry();
	C4SForceFairCrew eOpt = pSel ? pSel->GetFairCrewAllowed() : C4SFairCrew_Free;
	pGameOptionButtons->SetForceFairCrewState(eOpt);
}

void C4StartupScenSelDlg::UpdateList()
{
	AbortRenaming();
	// default: Show book (also for loading screen)
	pMapData = nullptr;
	pScenSelStyleTabular->SelectSheet(ShowStyle_Book, false);
	// and delete any stuff from map selection
	C4GUI::Tabular::Sheet *pMapSheet = pScenSelStyleTabular->GetSheet(ShowStyle_Map);
	while (pMapSheet->GetFirst()) delete pMapSheet->GetFirst();
	pfctBackground = nullptr;
	// for now, all the list is loaded at once anyway
	// so just clear and add all loaded items
	// remember old selection
	C4ScenarioListLoader::Entry *pOldSelection = GetSelectedEntry();
	C4GUI::Element *pEl;
	while (pEl = pScenSelList->GetFirst()) delete pEl;
	pScenSelCaption->SetText("");
	// scen loader still busy: Nothing to add
	if (!pScenLoader) return;
	if (pScenLoader->IsLoading())
	{
		StdStrBuf sProgressText;
		sProgressText.Format(LoadResStr("IDS_MSG_SCENARIODESC_LOADING"), static_cast<int32_t>(pScenLoader->GetProgressPercent()));
		pScenSelProgressLabel->SetText(sProgressText.getData());
		pScenSelProgressLabel->SetVisibility(true);
		return;
	}
	pScenSelProgressLabel->SetVisibility(false);
	// is this a map folder? Then show the map instead
	C4ScenarioListLoader::Folder *pFolder = pScenLoader->GetCurrFolder();
	if (pMapData = pFolder->GetMapData())
	{
		pMapData->ResetSelection();
		pMapData->CreateGUIElements(this, *pScenSelStyleTabular->GetSheet(ShowStyle_Map));
		pScenSelStyleTabular->SelectSheet(ShowStyle_Map, false);
	}
	else
	{
		// book style selection
		HideTitle(false);

		// add what has been loaded
		for (C4ScenarioListLoader::Entry *pEnt = pScenLoader->GetFirstEntry(); pEnt; pEnt = pEnt->GetNext())
		{
			StdStrBuf name = pEnt->GetName();
			CMarkup::StripMarkup(&name);
			if (!SLen(searchBar->GetText()) || SSearchNoCase(name.getData(), searchBar->GetText()))
			{
				ScenListItem *pEntItem = new ScenListItem(pScenSelList, pEnt);
				if (pEnt == pOldSelection) pScenSelList->SelectEntry(pEntItem, false);
			}
			else if (pEnt == pOldSelection)
			{
				pOldSelection = nullptr;
			}
		}
		// set title of current folder
		// but not root
		if (pFolder && pFolder != pScenLoader->GetRootFolder())
			pScenSelCaption->SetText(pFolder->GetName().getData());
		else
		{
			// special root title
			pScenSelCaption->SetText(LoadResStr("IDS_DLG_SCENARIOS"));
		}
		// new list has been loaded: Select first entry if nothing else had been selected
		if (!pOldSelection) pScenSelList->SelectFirstEntry(false);
	}
}

void C4StartupScenSelDlg::ResortFolder()
{
	// if it's still loading, sorting will be done in the end anyway
	if (!pScenLoader || pScenLoader->IsLoading()) return;
	C4ScenarioListLoader::Folder *pFolder = pScenLoader->GetCurrFolder();
	if (!pFolder) return;
	pFolder->Resort();
	UpdateList();
}

void C4StartupScenSelDlg::UpdateSelection()
{
	AbortRenaming();
	if (!pScenLoader)
	{
		C4Facet fctNoPic;
		pSelectionInfo->SetPicture(fctNoPic);
		pSelectionInfo->ClearText(false);
		SetOpenButtonDefaultText();
		return;
	}
	// determine target text box
	C4GUI::TextWindow *pSelectionInfo = pMapData ? pMapData->GetSelectionInfoBox() : this->pSelectionInfo;
	// get selected entry
	C4ScenarioListLoader::Entry *pSel = GetSelectedEntry();
	if (!pSel)
	{
		// no selection: Display data of current parent folder
		pSel = pScenLoader->GetCurrFolder();
		// but not root
		if (pSel == pScenLoader->GetRootFolder()) pSel = nullptr;
	}
	// get title image and desc of selected entry
	C4Facet fctTitle; StdStrBuf sTitle, sDesc, sVersion, sAuthor;
	if (pSel)
	{
		pScenLoader->LoadExtended(pSel); // 2do: Multithreaded
		fctTitle = pSel->GetTitlePicture();
		sTitle.Ref(pSel->GetName());
		sDesc.Ref(pSel->GetDesc());
		sVersion.Ref(pSel->GetVersion());
		sAuthor.Ref(pSel->GetAuthor());
		// never show a pure title string: There must always be some text or an image
		if (!fctTitle.Surface && (!sDesc || !*sDesc.getData()))
			sTitle.Clear();
		// selection specific open/start button
		pOpenBtn->SetText(pSel->GetOpenText().getData());
		pOpenBtn->SetToolTip(pSel->GetOpenTooltip().getData());

		if (auto *scenario = dynamic_cast<C4ScenarioListLoader::Scenario *>(pSel); scenario)
		{
			btnAllowUserChange->SetEnabled(!scenario->GetC4S().Definitions.LocalOnly);
			btnAllowUserChange->SetChecked(scenario->GetC4S().Definitions.AllowUserChange);
		}
		else
		{
			btnAllowUserChange->SetChecked(false);
			btnAllowUserChange->SetEnabled(false);
		}
	}
	else
	{
		btnAllowUserChange->SetChecked(false);
		btnAllowUserChange->SetEnabled(false);
		SetOpenButtonDefaultText();
	}
	// set data in selection component
	pSelectionInfo->ClearText(false);
	pSelectionInfo->SetPicture(fctTitle);
	if (!!sTitle && (!sDesc || !*sDesc.getData())) pSelectionInfo->AddTextLine(sTitle.getData(), &C4Startup::Get()->Graphics.BookFontCapt, ClrScenarioItem, false, false);
	if (!!sDesc) pSelectionInfo->AddTextLine(sDesc.getData(), &C4Startup::Get()->Graphics.BookFont, ClrScenarioItem, false, false, &C4Startup::Get()->Graphics.BookFontCapt);
	if (!!sAuthor) pSelectionInfo->AddTextLine(FormatString(LoadResStr("IDS_CTL_AUTHOR"), sAuthor.getData()).getData(),
		&C4Startup::Get()->Graphics.BookFont, ClrScenarioItemXtra, false, false);
	if (!!sVersion) pSelectionInfo->AddTextLine(FormatString(LoadResStr("IDS_DLG_VERSION"), sVersion.getData()).getData(),
		&C4Startup::Get()->Graphics.BookFont, ClrScenarioItemXtra, false, false);
	pSelectionInfo->UpdateHeight();
	// usecrew-button
	UpdateUseCrewBtn();
}

C4StartupScenSelDlg::ScenListItem *C4StartupScenSelDlg::GetSelectedItem()
{
	return static_cast<ScenListItem *>(pScenSelList->GetSelectedItem());
}

C4ScenarioListLoader::Entry *C4StartupScenSelDlg::GetSelectedEntry()
{
	// map layout: Get selection from map
	if (pMapData) return pMapData->GetSelectedEntry();
	// get selection in listbox
	ScenListItem *pSel = static_cast<ScenListItem *>(pScenSelList->GetSelectedItem());
	return pSel ? pSel->GetEntry() : nullptr;
}

bool C4StartupScenSelDlg::StartScenario(C4ScenarioListLoader::Scenario *pStartScen)
{
	assert(pStartScen);
	if (!pStartScen) return false;
	// get required object definitions
	if (btnAllowUserChange->GetChecked())
	{
		// get definitions as user selects them
		std::vector<std::string> defs = pStartScen->GetC4S().Definitions.GetModules();
		if (defs.empty())
		{
			defs.push_back("Objects.c4d");
		}
		if (!C4DefinitionSelDlg::SelectDefinitions(GetScreen(), defs))
			// user aborted during definition selection
			return false;

		Game.DefinitionFilenames = defs;
		Game.FixedDefinitions = true;
	}
	else
	{
		// for no user change, just set default objects. Custom settings will override later anyway
		Game.DefinitionFilenames.push_back("Objects.c4d");
	}
	// set other default startup parameters
	SCopy(pStartScen->GetEntryFilename().getData(), Game.ScenarioFilename);
	Game.fLobby = !!Game.NetworkActive; // always lobby in network
	Game.fObserve = false;
	// start with this set!
	C4Startup::Get()->Start();
	return true;
}

bool C4StartupScenSelDlg::OpenFolder(C4ScenarioListLoader::Folder *pNewFolder)
{
	// open it through loader
	if (!pScenLoader) return false;
	searchBar->ClearText();
	bool fSuccess = pScenLoader->Load(pNewFolder, false);
	UpdateList();
	UpdateSelection();
	if (!pMapData) SetFocus(pScenSelList, false);
	return fSuccess;
}

bool C4StartupScenSelDlg::DoOK()
{
	AbortRenaming();
	// get selected entry
	C4ScenarioListLoader::Entry *pSel = GetSelectedEntry();
	if (!pSel) return false;
	// check if open is possible
	StdStrBuf sError;
	if (!pSel->CanOpen(sError))
	{
		GetScreen()->ShowMessage(sError.getData(), LoadResStr("IDS_MSG_CANNOTSTARTSCENARIO"), C4GUI::Ico_Error);
		return false;
	}
	// if CanOpen returned true but set an error message, that means it's a warning. Display it!
	if (sError.getLength())
	{
		if (!GetScreen()->ShowMessageModal(sError.getData(), LoadResStrNoAmp("IDS_DLG_STARTGAME"), C4GUI::MessageDialog::btnOKAbort, C4GUI::Ico_Notify, &Config.Startup.HideMsgStartDedicated))
			// user chose to not start it
			return false;
	}
	// start it!
	return pSel->Start();
}

bool C4StartupScenSelDlg::DoBack(bool fAllowClose)
{
	AbortRenaming();
	// if in a subfolder, try backtrace folders first
	if (pScenLoader && pScenLoader->FolderBack())
	{
		searchBar->ClearText();
		UpdateList();
		UpdateSelection();
		return true;
	}
	// while this isn't multithreaded, the dialog must not be aborted while initial load...
	if (pScenLoader && pScenLoader->IsLoading()) return false;
	// return to main screen
	if (fAllowClose)
	{
		Close(false);
		return true;
	}
	return false;
}

void C4StartupScenSelDlg::DoRefresh()
{
	if (pScenLoader && !pScenLoader->IsLoading())
	{
		pScenSelList->SelectNone(false);
		pScenLoader->ReloadCurrent();
		UpdateList();
		UpdateSelection();
	}
}

void C4StartupScenSelDlg::SetOpenButtonDefaultText()
{
	pOpenBtn->SetText(LoadResStr("IDS_BTN_OPEN"));
	pOpenBtn->SetToolTip(LoadResStr("IDS_DLGTIP_SCENSELNEXT"));
}

bool C4StartupScenSelDlg::KeyRename()
{
	// no rename in map mode
	if (pMapData) return false;
	// not if renaming already
	if (IsRenaming()) return false;
	// forward to selected scenario list item
	ScenListItem *pSel = GetSelectedItem();
	if (!pSel) return false;
	return pSel->KeyRename();
}

bool C4StartupScenSelDlg::KeyDelete()
{
	// do not delete from map folder
	if (pMapData) return false;
	// cancel renaming
	AbortRenaming();
	// delete selected item: Confirmation first
	ScenListItem *pSel = GetSelectedItem();
	if (!pSel) return false;
	C4ScenarioListLoader::Entry *pEnt = pSel->GetEntry();
	StdStrBuf sWarning;
	bool fOriginal = false;
	if (C4Group_IsGroup(pEnt->GetEntryFilename().getData()))
	{
		C4Group Grp;
		if (Grp.Open(pEnt->GetEntryFilename().getData()))
		{
			fOriginal = !!Grp.GetOriginal();
		}
		Grp.Close();
	}
	sWarning.Format(LoadResStr(fOriginal ? "IDS_MSG_DELETEORIGINAL" : "IDS_MSG_PROMPTDELETE"), FormatString("%s %s", pEnt->GetTypeName().getData(), pEnt->GetName().getData()).getData());
	GetScreen()->ShowRemoveDlg(new C4GUI::ConfirmationDialog(sWarning.getData(), LoadResStr("IDS_MNU_DELETE"),
		new C4GUI::CallbackHandlerExPar<C4StartupScenSelDlg, ScenListItem *>(this, &C4StartupScenSelDlg::DeleteConfirm, pSel), C4GUI::MessageDialog::btnYesNo));
	return true;
}

void C4StartupScenSelDlg::DeleteConfirm(ScenListItem *pSel)
{
	// deletion confirmed. Do it.
	C4ScenarioListLoader::Entry *pEnt = pSel->GetEntry();
	if (!C4Group_DeleteItem(pEnt->GetEntryFilename().getData(), true))
	{
		StdStrBuf sMsg; sMsg.Format("%s", LoadResStr("IDS_FAIL_DELETE"));
		Game.pGUI->ShowMessageModal(sMsg.getData(), LoadResStr("IDS_MNU_DELETE"), C4GUI::MessageDialog::btnOK, C4GUI::Ico_Error);
		return;
	}
	// remove from scenario list
	pScenSelList->SelectEntry(pSel->GetNext(), false);
	delete pEnt;
	delete pSel;
}

bool C4StartupScenSelDlg::KeyCheat()
{
	return Game.pGUI->ShowRemoveDlg(new C4GUI::InputDialog(LoadResStr("IDS_TEXT_ENTERMISSIONPASSWORD"), LoadResStr("IDS_DLG_MISSIONACCESS"), C4GUI::Ico_Options,
		new C4GUI::InputCallback<C4StartupScenSelDlg>(this, &C4StartupScenSelDlg::KeyCheat2),
		false));
}

bool C4StartupScenSelDlg::KeySearch()
{
	SetFocus(searchBar, false);
	return true;
}

bool C4StartupScenSelDlg::KeyEscape()
{
	if (GetFocus() == searchBar)
	{
		SetFocus(pScenSelList, false);
		return true;
	}
	return false;
}

void C4StartupScenSelDlg::KeyCheat2(const StdStrBuf &rsCheatCode)
{
	// Special character "-": remove mission password(s)
	if (SEqual2(rsCheatCode.getData(), "-"))
	{
		const char *szPass = rsCheatCode.getPtr(1);
		if (szPass && *szPass)
		{
			SRemoveModules(Config.General.MissionAccess, szPass, false);
			UpdateList();
			UpdateSelection();
			return;
		}
	}

	// No special character: add mission password(s)
	const char *szPass = rsCheatCode.getPtr(0);
	if (szPass && *szPass)
	{
		SAddModules(Config.General.MissionAccess, szPass, false);
		UpdateList();
		UpdateSelection();
		return;
	}
}

void C4StartupScenSelDlg::FocusScenList()
{
	SetFocus(pScenSelList, false);
}

void C4StartupScenSelDlg::OnButtonScenario(C4GUI::Control *pEl)
{
	// map button was clicked: Update selected scenario
	if (!pMapData || !pEl) return;

	// OnButtonScenario may change the current selection
	if (C4ScenarioListLoader::Entry *pSel{GetSelectedEntry()}; pMapData->OnButtonScenario(pEl) || (pSel && pSel == GetSelectedEntry()))
	{
		DoOK();
		return;
	}

	// the first click selects it
	SetFocus(pEl, false);
	UpdateSelection();
}

void C4StartupScenSelDlg::DeselectAll()
{
	// Deselect all so current folder info is displayed
	if (GetFocus()) C4GUI::GUISound("ArrowHit");
	SetFocus(nullptr, true);
	if (pMapData) pMapData->ResetSelection();
	UpdateSelection();
}

void C4StartupScenSelDlg::StartRenaming(C4GUI::RenameEdit *pNewRenameEdit)
{
	pRenameEdit = pNewRenameEdit;
}

void C4StartupScenSelDlg::AbortRenaming()
{
	if (pRenameEdit) pRenameEdit->Abort();
}

// NICHT: 9, 7.2.2, 113-114
