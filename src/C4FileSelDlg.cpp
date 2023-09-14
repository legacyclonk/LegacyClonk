/*
 * LegacyClonk
 *
 * Copyright (c) RedWolf Design
 * Copyright (c) 2008, Sven2
 * Copyright (c) 2017-2021, The LegacyClonk Team and contributors
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

// file selection dialogs

#include "C4GuiComboBox.h"
#include "C4GuiListBox.h"
#include "C4GuiResource.h"
#include <C4Include.h>
#include <C4FileSelDlg.h>

#include <C4Game.h> // only for single use of Game.GraphicsResource.fctOKCancel below...

#ifdef _WIN32
#include "C4Windows.h"
#include <shlobj.h>
#endif

// C4FileSelDlg::ListItem

C4FileSelDlg::ListItem::ListItem(const char *szFilename) : C4GUI::Control(C4Rect(0, 0, 0, 0))
{
	if (szFilename) sFilename.Copy(szFilename);
}

C4FileSelDlg::ListItem::~ListItem() {}

// C4FileSelDlg::DefaultListItem

C4FileSelDlg::DefaultListItem::DefaultListItem(const char *szFilename, bool fTruncateExtension, bool fCheckbox, bool fGrayed, C4GUI::Icons eIcon)
	: C4FileSelDlg::ListItem(szFilename), pLbl(nullptr), pCheck(nullptr), pKeyCheck(nullptr), fGrayed(fGrayed)
{
	StdStrBuf sLabel; if (szFilename) sLabel.Ref(::GetFilename(szFilename)); else sLabel.Ref(LoadResStr("IDS_CTL_NONE"));
	if (szFilename && fTruncateExtension)
	{
		sLabel.Copy();
		char *szFilename = sLabel.GrabPointer();
		RemoveExtension(szFilename);
		sLabel.Take(szFilename);
	}
	rcBounds.Hgt = C4GUI::GetRes()->TextFont.GetLineHeight();
	UpdateSize();
	C4GUI::ComponentAligner caMain(GetContainedClientRect(), 0, 0);
	int32_t iHeight = caMain.GetInnerHeight();
	if (fCheckbox)
	{
		pCheck = new C4GUI::CheckBox(caMain.GetFromLeft(iHeight), "", false);
		if (fGrayed) pCheck->SetEnabled(false);
		AddElement(pCheck);
		pKeyCheck = new C4KeyBinding(C4KeyCodeEx(K_SPACE), "FileSelToggleFileActive", KEYSCOPE_Gui,
			new C4GUI::ControlKeyCB<ListItem>(*this, &ListItem::UserToggleCheck), C4CustomKey::PRIO_Ctrl);
	}
	C4GUI::Icon *pIco = new C4GUI::Icon(caMain.GetFromLeft(iHeight), eIcon);
	AddElement(pIco);
	pLbl = new C4GUI::Label(sLabel.getData(), caMain.GetAll(), ALeft, fGrayed ? C4GUI_CheckboxDisabledFontClr : C4GUI_CheckboxFontClr);
	AddElement(pLbl);
}

C4FileSelDlg::DefaultListItem::~DefaultListItem()
{
	delete pKeyCheck;
}

void C4FileSelDlg::DefaultListItem::UpdateOwnPos()
{
	BaseClass::UpdateOwnPos();
	if (!pLbl) return;
	C4GUI::ComponentAligner caMain(GetContainedClientRect(), 0, 0);
	caMain.GetFromLeft(caMain.GetInnerHeight() * (1 + !!pCheck));
	pLbl->SetBounds(caMain.GetAll());
}

bool C4FileSelDlg::DefaultListItem::IsChecked() const
{
	return pCheck ? pCheck->GetChecked() : false;
}

void C4FileSelDlg::DefaultListItem::SetChecked(bool fChecked)
{
	// store new state in checkbox
	if (pCheck) pCheck->SetChecked(fChecked);
}

bool C4FileSelDlg::DefaultListItem::UserToggleCheck()
{
	// toggle if possible
	if (pCheck && !IsGrayed())
	{
		pCheck->ToggleCheck(true);
		return true;
	}
	return false;
}

// C4FileSelDlg

C4FileSelDlg::C4FileSelDlg(const char *szRootPath, const char *szTitle, C4FileSel_BaseCB *pSelCallback, bool fInitElements)
	: C4GUI::Dialog(BoundBy(C4GUI::GetScreenWdt() * 2 / 3 + 10, 300, 600), BoundBy(C4GUI::GetScreenHgt() * 2 / 3 + 10, 220, 500), szTitle, false),
	pFileListBox(nullptr), pSelectionInfoBox(nullptr), btnOK(nullptr), pSelection(nullptr), pSelCallback(pSelCallback), pLocations(nullptr), iLocationCount(0), pLocationComboBox(nullptr)
{
	sTitle.Copy(szTitle);
	// key bindings
	pKeyRefresh = new C4KeyBinding(C4KeyCodeEx(K_F5), "FileSelReload", KEYSCOPE_Gui,
		new C4GUI::DlgKeyCB<C4FileSelDlg>(*this, &C4FileSelDlg::KeyRefresh), C4CustomKey::PRIO_CtrlOverride);
	pKeyEnterOverride = new C4KeyBinding(C4KeyCodeEx(K_RETURN), "FileSelConfirm", KEYSCOPE_Gui,
		new C4GUI::DlgKeyCB<C4FileSelDlg>(*this, &C4FileSelDlg::KeyEnter), C4CustomKey::PRIO_CtrlOverride);
	if (fInitElements) InitElements();
	sPath.Copy(szRootPath);
}

void C4FileSelDlg::InitElements()
{
	UpdateSize();
	CStdFont *pUseFont = &(C4GUI::GetRes()->TextFont);
	// main calcs
	bool fHasOptions = HasExtraOptions();
	C4GUI::ComponentAligner caMain(GetClientRect(), 0, 0, true);
	C4Rect rcOptions;
	C4GUI::ComponentAligner caButtonArea(caMain.GetFromBottom(C4GUI_ButtonAreaHgt, 2 * C4GUI_DefButton2Wdt + 4 * C4GUI_DefButton2HSpace), C4GUI_DefButton2HSpace, (C4GUI_ButtonAreaHgt - C4GUI_ButtonHgt) / 2);
	if (fHasOptions) rcOptions = caMain.GetFromBottom(pUseFont->GetLineHeight() + 2 * C4GUI_DefDlgSmallIndent);
	C4GUI::ComponentAligner caUpperArea(caMain.GetAll(), C4GUI_DefDlgIndent, C4GUI_DefDlgIndent, true);
	// create file selection area
	if (iLocationCount)
	{
		C4GUI::ComponentAligner caLocations(caUpperArea.GetFromTop(C4GUI::ComboBox::GetDefaultHeight() + 2 * C4GUI_DefDlgSmallIndent), C4GUI_DefDlgIndent, C4GUI_DefDlgSmallIndent, false);
		StdStrBuf sText(LoadResStr("IDS_TEXT_LOCATION"), false);
		AddElement(new C4GUI::Label(sText.getData(), caLocations.GetFromLeft(pUseFont->GetTextWidth(sText.getData())), ALeft));
		pLocationComboBox = new C4GUI::ComboBox(caLocations.GetAll());
		pLocationComboBox->SetComboCB(new C4GUI::ComboBox_FillCallback<C4FileSelDlg>(this, &C4FileSelDlg::OnLocationComboFill, &C4FileSelDlg::OnLocationComboSelChange));
		pLocationComboBox->SetText(pLocations[0].sName.getData());
	}
	// create file selection area
	bool fHasPreview = HasPreviewArea();
	pFileListBox = new C4GUI::ListBox(fHasPreview ? caUpperArea.GetFromLeft(caUpperArea.GetWidth() / 2) : caUpperArea.GetAll(), GetFileSelColWidth());
	pFileListBox->SetSelectionChangeCallbackFn(new C4GUI::CallbackHandler<C4FileSelDlg>(this, &C4FileSelDlg::OnSelChange));
	pFileListBox->SetSelectionDblClickFn(new C4GUI::CallbackHandler<C4FileSelDlg>(this, &C4FileSelDlg::OnSelDblClick));
	if (fHasPreview)
	{
		caUpperArea.ExpandLeft(C4GUI_DefDlgIndent);
		pSelectionInfoBox = new C4GUI::TextWindow(caUpperArea.GetAll());
		pSelectionInfoBox->SetDecoration(true, true, nullptr, true);
	}
	// create button area
	C4GUI::Button *btnAbort = new C4GUI::CancelButton(caButtonArea.GetFromRight(C4GUI_DefButton2Wdt));
	btnOK = new C4GUI::OKButton(caButtonArea.GetFromRight(C4GUI_DefButton2Wdt));
	// add components in tab order
	if (pLocationComboBox) AddElement(pLocationComboBox);
	AddElement(pFileListBox);
	if (pSelectionInfoBox) AddElement(pSelectionInfoBox);
	if (fHasOptions) AddExtraOptions(rcOptions);
	AddElement(btnOK);
	AddElement(btnAbort);
	SetFocus(pFileListBox, false);
	// no selection yet
	UpdateSelection();
}

C4FileSelDlg::~C4FileSelDlg()
{
	delete[] pLocations;
	delete pSelCallback;
	delete pKeyEnterOverride;
	delete pKeyRefresh;
}

void C4FileSelDlg::OnLocationComboFill(C4GUI::ComboBox_FillCB *pFiller)
{
	// Add all locations
	for (int32_t i = 0; i < iLocationCount; ++i)
		pFiller->AddEntry(pLocations[i].sName.getData(), i);
}

bool C4FileSelDlg::OnLocationComboSelChange(C4GUI::ComboBox *pForCombo, int32_t idNewSelection)
{
	SetCurrentLocation(idNewSelection, true);
	// No text change by caller; text alread changed by SetCurrentLocation
	return true;
}

void C4FileSelDlg::SetPath(const char *szNewPath, bool fRefresh)
{
	sPath.Copy(szNewPath);
	if (fRefresh && IsShown()) UpdateFileList();
}

void C4FileSelDlg::OnShown()
{
	BaseClass::OnShown();
	// load files
	UpdateFileList();
}

void C4FileSelDlg::UserClose(bool fOK)
{
	if (!fOK || pSelection)
	{
		// allow OK only if something is sth is selected
		Close(fOK);
	}
	else
	{
		GetScreen()->ShowErrorMessage(LoadResStr("IDS_ERR_PLEASESELECTAFILEFIRST"));
	}
}

void C4FileSelDlg::OnClosed(bool fOK)
{
	if (fOK && pSelection && pSelCallback)
		pSelCallback->OnFileSelected(pSelection->GetFilename());
	// base call: Might delete dlg
	BaseClass::OnClosed(fOK);
}

void C4FileSelDlg::OnSelDblClick(class C4GUI::Element *pEl)
{
	// item double-click: confirms with this file in single mode; toggles selection in multi mode
	if (IsMultiSelection())
	{
		ListItem *pItem = static_cast<ListItem *>(pEl);
		pItem->UserToggleCheck();
	}
	else
		UserClose(true);
}

C4FileSelDlg::ListItem *C4FileSelDlg::CreateListItem(const char *szFilename)
{
	// Default list item
	if (szFilename)
		return new DefaultListItem(szFilename, !!GetFileMask(), IsMultiSelection(), IsItemGrayed(szFilename), GetFileItemIcon());
	else
		return new DefaultListItem(nullptr, false, IsMultiSelection(), false, GetFileItemIcon());
}

void C4FileSelDlg::UpdateFileList()
{
	BeginFileListUpdate();
	// reload files
	C4GUI::Element *pEl;
	while (pEl = pFileListBox->GetFirst()) delete pEl;
	// file items
	const char *szFileMask = GetFileMask();
	for (DirectoryIterator iter(sPath.getData()); *iter; ++iter)
		if (!szFileMask || WildcardListMatch(szFileMask, *iter))
			pFileListBox->AddElement(CreateListItem(*iter));
	// none-item?
	if (HasNoneItem())
	{
		pFileListBox->AddElement(CreateListItem(nullptr));
	}
	// list now done
	EndFileListUpdate();
	// path into title
	const char *szPath = sPath.getData();
	SetTitle(*szPath ? FormatString("%s [%s]", sTitle.getData(), szPath).getData() : sTitle.getData());
	// initial no-selection
	UpdateSelection();
}

void C4FileSelDlg::UpdateSelection()
{
	// update selection from list
	pSelection = static_cast<ListItem *>(pFileListBox->GetSelectedItem());
	// OK button only available if selection
	// btnOK->SetEnabled would look a lot better here, but it doesn't exist yet :(
	// selection preview, if enabled
	if (pSelectionInfoBox)
	{
		// default empty
		pSelectionInfoBox->ClearText(false);
		if (!pSelection) { pSelectionInfoBox->UpdateHeight(); return; }
		// add selection description
		if (pSelection->GetFilename())
			pSelectionInfoBox->AddTextLine(pSelection->GetFilename(), &C4GUI::GetRes()->TextFont, C4GUI_MessageFontClr, true, false);
	}
}

void C4FileSelDlg::SetSelection(const std::vector<std::string> &newSelection, bool fFilenameOnly)
{
	// check all selected definitions
	for (ListItem *pFileItem = static_cast<ListItem *>(pFileListBox->GetFirst()); pFileItem; pFileItem = static_cast<ListItem *>(pFileItem->GetNext()))
	{
		const char *szFileItemFilename = pFileItem->GetFilename();
		if (fFilenameOnly) szFileItemFilename = GetFilename(szFileItemFilename);
		pFileItem->SetChecked(std::find(newSelection.begin(), newSelection.end(), szFileItemFilename) != newSelection.end());
	}
}

std::vector<std::string> C4FileSelDlg::GetSelection(const std::vector<std::string> &fixedSelection, bool filenameOnly) const
{
	if (!IsMultiSelection())
	{
		// get single selected file for single selection dlg
		if (const char *str{filenameOnly ? GetFilename(pSelection->GetFilename()) : pSelection->GetFilename()}; str)
		{
			return {str};
		}

		return {};
	}
	else
	{
		std::vector<std::string> selection{fixedSelection};

		//  get ';'-separated list for multi selection dlg
		for (ListItem *pFileItem = static_cast<ListItem *>(pFileListBox->GetFirst()); pFileItem; pFileItem = static_cast<ListItem *>(pFileItem->GetNext()))
		{
			if (pFileItem->IsChecked())
			{
				const char *szAppendFilename = pFileItem->GetFilename();
				if (filenameOnly) szAppendFilename = GetFilename(szAppendFilename);

				// prevent adding entries twice (especially those from the fixed selection list)
				if (std::find(selection.cbegin(), selection.cend(), szAppendFilename) == selection.cend())
				{
					selection.push_back(szAppendFilename);
				}
			}
		}

		return selection;
	}
}

void C4FileSelDlg::AddLocation(const char *szName, const char *szPath)
{
	// add to list
	int32_t iNewLocCount = iLocationCount + 1;
	Location *pNewLocations = new Location[iNewLocCount];
	for (int32_t i = 0; i < iLocationCount; ++i) pNewLocations[i] = pLocations[i];
	pNewLocations[iLocationCount].sName.Copy(szName);
	pNewLocations[iLocationCount].sPath.Copy(szPath);
	delete[] pLocations; pLocations = pNewLocations; iLocationCount = iNewLocCount;
	// first location? Then set path to this
	if (iLocationCount == 1) SetPath(szPath, false);
}

void C4FileSelDlg::AddCheckedLocation(const char *szName, const char *szPath)
{
	// check location
	// path must exit
	if (!szPath || !*szPath) return;
	if (!DirectoryExists(szPath)) return;
	// path must not be in list yet
	for (int32_t i = 0; i < iLocationCount; ++i)
		if (ItemIdentical(szPath, pLocations[i].sPath.getData()))
			return;
	// OK; add it!
	AddLocation(szName, szPath);
}

int32_t C4FileSelDlg::GetCurrentLocationIndex() const
{
	return iCurrentLocationIndex;
}

void C4FileSelDlg::SetCurrentLocation(int32_t idx, bool fRefresh)
{
	// safety
	if (!Inside<int32_t>(idx, 0, iLocationCount)) return;
	// update ComboBox-text
	iCurrentLocationIndex = idx;
	if (pLocationComboBox) pLocationComboBox->SetText(pLocations[idx].sName.getData());
	// set new path
	SetPath(pLocations[idx].sPath.getData(), fRefresh);
}

// C4PlayerSelDlg

C4PlayerSelDlg::C4PlayerSelDlg(C4FileSel_BaseCB *pSelCallback)
	: C4FileSelDlg(Config.AtExePath(Config.General.PlayerPath), LoadResStr("IDS_MSG_SELECTPLR"), pSelCallback) {}

// C4DefinitionSelDlg

C4DefinitionSelDlg::C4DefinitionSelDlg(C4FileSel_BaseCB *pSelCallback, const std::vector<std::string> &fixedSelection)
	: C4FileSelDlg(Config.AtExePath(Config.General.DefinitionPath), FormatString(LoadResStr("IDS_MSG_SELECT"), LoadResStr("IDS_DLG_DEFINITIONS")).getData(), pSelCallback), fixedSelection(fixedSelection)
{
}

void C4DefinitionSelDlg::OnShown()
{
	// base call: load file list
	C4FileSelDlg::OnShown();
	// initial selection
	if (fixedSelection.size()) SetSelection(fixedSelection, true);
}

bool C4DefinitionSelDlg::IsItemGrayed(const char *szFilename) const
{
	// cannot change initial selection
	if (fixedSelection.empty()) return false;
	return std::find(fixedSelection.begin(), fixedSelection.end(), GetFilename(szFilename)) != fixedSelection.end();
}

bool C4DefinitionSelDlg::SelectDefinitions(C4GUI::Screen *pOnScreen, std::vector<std::string> &selection)
{
	// let the user select definitions by showing a modal selection dialog
	C4DefinitionSelDlg *pDlg = new C4DefinitionSelDlg(nullptr, selection);
	bool fResult;
	if (fResult = pOnScreen->ShowModalDlg(pDlg, false))
	{
		selection = pDlg->GetSelection(selection, true);
	}
	if (C4GUI::IsGUIValid()) delete pDlg;
	return fResult;
}

// C4PortraitSelDlg::ListItem

C4PortraitSelDlg::ListItem::ListItem(const char *szFilename) : C4FileSelDlg::ListItem(szFilename)
, fError(false), fLoaded(false)
{
	CStdFont *pUseFont = &(C4GUI::GetRes()->MiniFont);
	// determine label text
	StdStrBuf sDisplayLabel;
	if (szFilename)
	{
		sDisplayLabel.Copy(::GetFilename(szFilename));
		::RemoveExtension(&sDisplayLabel);
	}
	else
	{
		sDisplayLabel.Ref(LoadResStr("IDS_MSG_NOPORTRAIT"));
	}
	// insert linebreaks into label text
	int32_t iLineHgt = std::max<int32_t>(pUseFont->BreakMessage(sDisplayLabel.getData(), ImagePreviewSize - 6, &sFilenameLabelText, false), 1);
	// set size
	SetBounds(C4Rect(0, 0, ImagePreviewSize, ImagePreviewSize + iLineHgt));
}

void C4PortraitSelDlg::ListItem::Load()
{
	if (sFilename)
	{
		// safety
		fLoaded = false;
		// load image file
		C4Group SrcGrp;
		StdStrBuf sParentPath;
		GetParentPath(sFilename.getData(), &sParentPath);
		bool fLoadError = true;
		if (SrcGrp.Open(sParentPath.getData()))
			if (fctLoadedImage.Load(SrcGrp, ::GetFilename(sFilename.getData())))
			{
				// image loaded. Can only be put into facet by main thread, because those operations aren't thread safe
				fLoaded = true;
				fLoadError = false;
			}
		SrcGrp.Close();
		fError = fLoadError;
	}
}

void C4PortraitSelDlg::ListItem::DrawElement(C4FacetEx &cgo)
{
	// Scale down newly loaded image?
	if (fLoaded)
	{
		fLoaded = false;
		if (!fctImage.CopyFromSfcMaxSize(fctLoadedImage.GetFace(), ImagePreviewSize))
			fError = true;
		fctLoadedImage.GetFace().Clear();
		fctLoadedImage.Clear();
	}
	// Draw picture
	CStdFont *pUseFont = &(C4GUI::GetRes()->MiniFont);
	C4Facet cgoPicture(cgo.Surface, cgo.TargetX + rcBounds.x, cgo.TargetY + rcBounds.y, ImagePreviewSize, ImagePreviewSize);
	if (fError || !sFilename)
	{
		C4Facet &fctNoneImg = Game.GraphicsResource.fctOKCancel;
		fctNoneImg.Draw(cgoPicture.Surface, cgoPicture.X + (cgoPicture.Wdt - fctNoneImg.Wdt) / 2, cgoPicture.Y + (cgoPicture.Hgt - fctNoneImg.Hgt) / 2, 1, 0);
	}
	else
	{
		if (!fctImage.Surface)
		{
			// not loaded yet
			lpDDraw->TextOut(LoadResStr("IDS_PRC_INITIALIZE"), C4GUI::GetRes()->MiniFont, 1.0f, cgo.Surface, cgoPicture.X + cgoPicture.Wdt / 2, cgoPicture.Y + (cgoPicture.Hgt - C4GUI::GetRes()->MiniFont.GetLineHeight()) / 2, C4GUI_StatusFontClr, ACenter, false);
		}
		else
		{
			fctImage.Draw(cgoPicture);
		}
	}
	// draw filename
	lpDDraw->TextOut(sFilenameLabelText.getData(), *pUseFont, 1.0f, cgo.Surface, cgoPicture.X + rcBounds.Wdt / 2, cgoPicture.Y + cgoPicture.Hgt, C4GUI_MessageFontClr, ACenter, false);
}

// C4PortraitSelDlg::ImageLoader

void C4PortraitSelDlg::ImageLoader::ClearLoadItems()
{
	// clear list
	LoadItems.clear();
}

void C4PortraitSelDlg::ImageLoader::AddLoadItem(ListItem *pItem)
{
	LoadItems.push_back(pItem);
}

void C4PortraitSelDlg::ImageLoader::Execute()
{
	// list empty?
	if (!LoadItems.size())
	{
		// then we're done!
		return;
	}
	// load one item at the time
	ListItem *pLoadItem = LoadItems.front();
	pLoadItem->Load();
	LoadItems.pop_front();
}

// C4PortraitSelDlg

C4PortraitSelDlg::C4PortraitSelDlg(C4FileSel_BaseCB *pSelCallback, bool fSetPicture, bool fSetBigIcon)
	: C4FileSelDlg(Config.General.ExePath, FormatString(LoadResStr("IDS_MSG_SELECT"), LoadResStr("IDS_TYPE_PORTRAIT")).getData(), pSelCallback, false)
	, pCheckSetPicture(nullptr), pCheckSetBigIcon(nullptr), fDefSetPicture(fSetPicture), fDefSetBigIcon(fSetBigIcon)
{
	char path[_MAX_PATH + 1];
	// add common picture locations
	StdStrBuf strLocation;
	SCopy(Config.AtUserPath(""), path, _MAX_PATH); TruncateBackslash(path);
	strLocation.Format("%s %s", C4ENGINECAPTION, LoadResStr("IDS_TEXT_USERPATH"));
	AddLocation(strLocation.getData(), path);
	strLocation.Format("%s %s", C4ENGINECAPTION, LoadResStr("IDS_TEXT_PROGRAMDIRECTORY"));
	AddCheckedLocation(strLocation.getData(), Config.General.ExePath);
#ifdef _WIN32
	if (SHGetSpecialFolderPath(nullptr, path, CSIDL_PERSONAL,         FALSE)) AddCheckedLocation(LoadResStr("IDS_TEXT_MYDOCUMENTS"), path);
	if (SHGetSpecialFolderPath(nullptr, path, CSIDL_MYPICTURES,       FALSE)) AddCheckedLocation(LoadResStr("IDS_TEXT_MYPICTURES"),  path);
	if (SHGetSpecialFolderPath(nullptr, path, CSIDL_DESKTOPDIRECTORY, FALSE)) AddCheckedLocation(LoadResStr("IDS_TEXT_DESKTOP"),     path);
#endif
#ifdef __APPLE__
	AddCheckedLocation(LoadResStr("IDS_TEXT_HOME"),       getenv("HOME"));
#else
	AddCheckedLocation(LoadResStr("IDS_TEXT_HOMEFOLDER"), getenv("HOME"));
#endif
#ifndef _WIN32
	sprintf(path, "%s%cDesktop", getenv("HOME"), DirectorySeparator);
	AddCheckedLocation(LoadResStr("IDS_TEXT_DESKTOP"), path);
#endif
	// build dialog
	InitElements();
	// select last location
	SetCurrentLocation(Config.Startup.LastPortraitFolderIdx, false);
}

void C4PortraitSelDlg::AddExtraOptions(const C4Rect &rcOptionsRect)
{
	C4GUI::ComponentAligner caOptions(rcOptionsRect, C4GUI_DefDlgIndent, C4GUI_DefDlgSmallIndent, false);
	CStdFont *pUseFont = &(C4GUI::GetRes()->TextFont);
	AddElement(new C4GUI::Label(LoadResStr("IDS_CTL_IMPORTIMAGEAS"), caOptions.GetGridCell(0, 3, 0, 1, -1, pUseFont->GetLineHeight(), true), ALeft));
	AddElement(pCheckSetPicture = new C4GUI::CheckBox(caOptions.GetGridCell(1, 3, 0, 1, -1, pUseFont->GetLineHeight(), true), LoadResStr("IDS_TEXT_PLAYERIMAGE"), fDefSetPicture));
	pCheckSetPicture->SetToolTip(LoadResStr("IDS_DESC_CHANGESTHEIMAGEYOUSEEINTH"));
	AddElement(pCheckSetBigIcon = new C4GUI::CheckBox(caOptions.GetGridCell(2, 3, 0, 1, -1, pUseFont->GetLineHeight(), true), LoadResStr("IDS_TEXT_LOBBYICON"), fDefSetPicture));
	pCheckSetBigIcon->SetToolTip(LoadResStr("IDS_DESC_CHANGESTHEIMAGEYOUSEEINTH2"));
}

void C4PortraitSelDlg::OnClosed(bool fOK)
{
	// remember location
	Config.Startup.LastPortraitFolderIdx = GetCurrentLocationIndex();
	// inherited
	C4FileSelDlg::OnClosed(fOK);
}

C4FileSelDlg::ListItem *C4PortraitSelDlg::CreateListItem(const char *szFilename)
{
	// use own list item type
	ListItem *pNew = new ListItem(szFilename);
	// schedule image loading
	ImageLoader.AddLoadItem(pNew);
	return pNew;
}

void C4PortraitSelDlg::BeginFileListUpdate()
{
	// new file list. Stop loading current
	ImageLoader.ClearLoadItems();
}

void C4PortraitSelDlg::OnIdle()
{
	// no multithreading. Workaround for image loading then...
	static int32_t i = 0;
	if (!(i++ % 10)) ImageLoader.Execute();
}

bool C4PortraitSelDlg::SelectPortrait(C4GUI::Screen *pOnScreen, std::string &selection, bool *pfSetPicture, bool *pfSetBigIcon)
{
	// copy some default potraits to UserPath (but only try this once, no real error handling)
	if (!Config.General.UserPortraitsWritten)
	{
		Log("Copying default portraits to user path...");
		C4Group hGroup;
		if (hGroup.Open(Config.AtExePath(C4CFN_Graphics)))
		{
			hGroup.Extract("Portrait1.png", Config.AtUserPath("Clonk.png"));
			hGroup.Extract("PortraitBandit.png", Config.AtUserPath("Bandit.png"));
			hGroup.Extract("PortraitIndianChief.png", Config.AtUserPath("IndianChief.png"));
			hGroup.Extract("PortraitKing.png", Config.AtUserPath("King.png"));
			hGroup.Extract("PortraitKnight.png", Config.AtUserPath("Knight.png"));
			hGroup.Extract("PortraitMage.png", Config.AtUserPath("Mage.png"));
			hGroup.Extract("PortraitPiranha.png", Config.AtUserPath("Piranha.png"));
			hGroup.Extract("PortraitSheriff.png", Config.AtUserPath("Sheriff.png"));
			hGroup.Extract("PortraitWipf.png", Config.AtUserPath("Wipf.png"));
			hGroup.Close();
		}
		Config.General.UserPortraitsWritten = true;
	}
	// let the user select a portrait by showing a modal selection dialog
	C4PortraitSelDlg *pDlg = new C4PortraitSelDlg(nullptr, *pfSetPicture, *pfSetBigIcon);
	bool fResult = pOnScreen->ShowModalDlg(pDlg, false);
	if (fResult)
	{
		std::vector<std::string> s{pDlg->GetSelection({}, false)};
		fResult = !s.empty();
		if (fResult)
		{
			selection = *s.begin();
		}
		*pfSetPicture = pDlg->IsSetPicture();
		*pfSetBigIcon = pDlg->IsSetBigIcon();
	}
	if (C4GUI::IsGUIValid()) delete pDlg;
	return fResult;
}
