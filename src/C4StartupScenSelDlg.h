/*
 * LegacyClonk
 *
 * Copyright (c) RedWolf Design
 * Copyright (c) 2005, Sven2
 * Copyright (c) 2017-2020, The LegacyClonk Team and contributors
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

#pragma once

#include "C4Startup.h"
#include "C4Scenario.h"
#include "C4Folder.h"
#include "StdFile.h"

#include <memory>
#include <string_view>

class C4StartupScenSelDlg;

const int32_t C4StartupScenSel_DefaultIcon_Scenario  = 14,
              C4StartupScenSel_DefaultIcon_Folder    = 0,
              C4StartupScenSel_DefaultIcon_WinFolder = 44,
              C4StartupScenSel_DefaultIcon_OldIconBG = 18,
              C4StartupScenSel_IconCount             = 52,
              C4StartupScenSel_TitlePictureWdt       = 200,
              C4StartupScenSel_TitlePictureHgt       = 150,
              C4StartupScenSel_TitlePicturePadding   = 10,
              C4StartupScenSel_TitleOverlayMargin    = 10; // number of pixels to each side of title overlay picture

// a list of loaded scenarios
class C4ScenarioListLoader
{
public:
	class Folder;
	// either a scenario or scenario folder; manages singly linked tree
	class Entry
	{
	protected:
		Entry *pNext;
		class Folder *pParent;

		friend class Folder;

	protected:
		StdStrBuf sName, sFilename, sDesc, sVersion, sAuthor, sMaker;
		C4FacetExSurface fctIcon, fctTitle;
		bool fBaseLoaded, fExLoaded;
		int iIconIndex;
		int iDifficulty;
		int iFolderIndex;

	public:
		Entry(class Folder *pParent);
		virtual ~Entry(); // dtor: unlink from tree

		bool Load(C4Group *pFromGrp, const StdStrBuf *psFilename, bool fLoadEx); // load as child if pFromGrp, else directly from filename
		virtual bool LoadCustom(C4Group &rGrp, bool fNameLoaded, bool fIconLoaded) { return true; } // load custom data for entry type (e.g. scenario title fallback in Scenario.txt)
		virtual bool LoadCustomPre(C4Group &rGrp) { return true; } // preload stuff that's early in the group (Scenario.txt)
		virtual bool Start() = 0; // start/open entry
		virtual Folder *GetIsFolder() { return nullptr; } // return this if this is a folder

		const StdStrBuf &GetName() const { return sName; }
		const StdStrBuf &GetEntryFilename() const { return sFilename; }
		const StdStrBuf &GetVersion() const { return sVersion; }
		const StdStrBuf &GetAuthor() const { return sAuthor; }
		const C4Facet &GetIconFacet() const { return fctIcon; }
		const C4Facet &GetTitlePicture() const { return fctTitle; }
		const StdStrBuf &GetDesc() const { return sDesc; }
		const int GetIconIndex() { return iIconIndex; }
		const int GetDifficulty() { return iDifficulty; }
		const int GetFolderIndex() { return iFolderIndex; }
		Entry *GetNext() const { return pNext; }
		class Folder *GetParent() const { return pParent; }
		virtual StdStrBuf GetTypeName() = 0;

		static Entry *CreateEntryForFile(const StdStrBuf &sFilename, Folder *pParent); // create correct entry type based on file extension

		virtual bool CanOpen(StdStrBuf &sError) { return true; } // whether item can be started/opened (e.g. mission access)
		virtual bool HasMissionAccess() const { return true; }
		virtual StdStrBuf GetOpenText() = 0; // get open button text
		virtual StdStrBuf GetOpenTooltip() = 0;

		virtual C4SForceFairCrew GetFairCrewAllowed() const { return C4SFairCrew_Free; }

		virtual const char *GetDefaultExtension() { return nullptr; } // extension to be added when item is renamed
		virtual bool SetTitleInGroup(C4Group &rGrp, const char *szNewTitle);
		bool RenameTo(const char *szNewName); // change name+filename
		virtual bool IsScenario() { return false; }
	};

	// a loaded scenario to be started
	class Scenario : public Entry
	{
	private:
		C4Scenario C4S;
		bool fNoMissionAccess;
		int32_t iMinPlrCount;

	public:
		Scenario(class Folder *pParent) : Entry(pParent), fNoMissionAccess(false), iMinPlrCount(0) {}
		virtual ~Scenario() {}

		virtual bool LoadCustom(C4Group &rGrp, bool fNameLoaded, bool fIconLoaded) override; // do fallbacks for title and icon; check whether scenario is valid
		virtual bool LoadCustomPre(C4Group &rGrp) override; // load scenario core
		virtual bool Start() override; // launch scenario!

		virtual bool CanOpen(StdStrBuf &sError) override; // check mission access, player count, etc.
		virtual bool HasMissionAccess() const override { return !fNoMissionAccess; } // check mission access only
		virtual StdStrBuf GetOpenText() override; // get open button text
		virtual StdStrBuf GetOpenTooltip() override;
		const C4Scenario &GetC4S() const { return C4S; } // get scenario core

		virtual C4SForceFairCrew GetFairCrewAllowed() const override { return static_cast<C4SForceFairCrew>(C4S.Head.ForcedFairCrew); }

		virtual StdStrBuf GetTypeName() override { return StdStrBuf(LoadResStr("IDS_TYPE_SCENARIO")); }

		virtual const char *GetDefaultExtension() override { return "c4s"; }

		virtual bool IsScenario() override { return true; }
	};

	// scenario folder
	class Folder : public Entry
	{
	protected:
		C4Folder C4F;
		bool fContentsLoaded; // if set, directory contents are already loaded
		Entry *pFirst; // tree structure
		class C4MapFolderData *pMapData; // if set, contains gfx and data for special map-style folders

		friend class Entry;

	public:
		Folder(Folder *pParent) : Entry(pParent), fContentsLoaded(false), pFirst(nullptr), pMapData(nullptr) {}
		virtual ~Folder();

		virtual bool LoadCustomPre(C4Group &rGrp) override; // load folder core

		bool LoadContents(C4ScenarioListLoader *pLoader, C4Group *pFromGrp, const StdStrBuf *psFilename, bool fLoadEx, bool fReload); // load folder contents as child if pFromGrp, else directly from filename
		uint32_t GetEntryCount() const;

	protected:
		void ClearChildren();
		void Sort();
		virtual bool DoLoadContents(C4ScenarioListLoader *pLoader, C4Group *pFromGrp, const StdStrBuf &sFilename, bool fLoadEx) = 0; // load folder contents as child if pFromGrp, else directly from filename

	public:
		virtual bool Start() override; // open as subfolder
		virtual Folder *GetIsFolder() override { return this; } // this is a folder
		Entry *GetFirstEntry() const { return pFirst; }
		void Resort() { Sort(); }
		Entry *FindEntryByName(const char *szFilename) const; // find entry by filename comparison

		virtual bool CanOpen(StdStrBuf &sError) override { return true; } // can always open folders
		virtual StdStrBuf GetOpenText() override; // get open button text
		virtual StdStrBuf GetOpenTooltip() override;
		C4MapFolderData *GetMapData() const { return pMapData; }
	};

	// .c4f subfolder: Read through by group
	class SubFolder : public Folder
	{
	public:
		SubFolder(Folder *pParent) : Folder(pParent) {}
		virtual ~SubFolder() {}

		virtual const char *GetDefaultExtension() override { return "c4f"; }

		virtual StdStrBuf GetTypeName() override { return StdStrBuf(LoadResStr("IDS_TYPE_FOLDER")); }

	protected:
		virtual bool LoadCustom(C4Group &rGrp, bool fNameLoaded, bool fIconLoaded) override; // load custom data for entry type - icon fallback to folder icon
		virtual bool DoLoadContents(C4ScenarioListLoader *pLoader, C4Group *pFromGrp, const StdStrBuf &sFilename, bool fLoadEx) override; // load folder contents as child if pFromGrp, else directly from filename
	};

	// regular, open folder: Read through by directory iterator
	class RegularFolder : public Folder
	{
	public:
		RegularFolder(Folder *pParent) : Folder(pParent) {}
		virtual ~RegularFolder() {}

		virtual StdStrBuf GetTypeName() override { return StdStrBuf(LoadResStr("IDS_TYPE_DIRECTORY")); }

		void Merge(const fs::path &path);

	protected:
		virtual bool LoadCustom(C4Group &rGrp, bool fNameLoaded, bool fIconLoaded) override; // load custom data for entry type - icon fallback to folder icon
		virtual bool DoLoadContents(C4ScenarioListLoader *pLoader, C4Group *pFromGrp, const StdStrBuf &sFilename, bool fLoadEx) override; // load folder contents as child if pFromGrp, else directly from filename

	private:
		std::vector<fs::path> contents;
	};

private:
	Folder *pRootFolder, *pCurrFolder; // scenario list in working directory
	int32_t iLoading, iProgress, iMaxProgress;
	bool fAbortThis, fAbortPrevious; // activity state
	unsigned long lastCheckTimer{0};

public:
	C4ScenarioListLoader();
	~C4ScenarioListLoader();

private:
	// activity control (to be replaced by true multithreading)
	bool BeginActivity(bool fAbortPrevious);
	void EndActivity();

public:
	bool DoProcessCallback(int32_t iProgress, int32_t iMaxProgress); // returns false if the activity was aborted

public:
	bool Load(const StdStrBuf &sRootFolder); // (unthreaded) loading of all entries in root folder
	bool Load(Folder *pSpecifiedFolder, bool fReload); // (unthreaded) loading of all entries in subfolder
	bool LoadExtended(Entry *pEntry); // (unthreaded) loading of desc and title image of specified entry
	bool FolderBack(); // go upwards by one folder
	bool ReloadCurrent(); // reload file list
	bool IsLoading() const { return !!iLoading; }
	Entry *GetFirstEntry() const { return pCurrFolder ? pCurrFolder->GetFirstEntry() : nullptr; }

	Folder *GetCurrFolder() const { return pCurrFolder; }
	Folder *GetRootFolder() const { return pRootFolder; }

	int32_t GetProgressPercent() const { return iProgress * 100 / std::max<int32_t>(iMaxProgress, 1); }
};

// for map-style folders: Data for map display
class C4MapFolderData
{
private:
	struct Scenario
	{
		// compiled data
		StdStrBuf sFilename;
		StdStrBuf sBaseImage, sOverlayImage;
		bool singleClick{false}; // selected by single click instead of double click

		// parameters for title as drawn on the map (if desired; otherwise sTitle empty)
		StdStrBuf sTitle;
		int32_t iTitleFontSize;
		uint32_t dwTitleInactClr, dwTitleActClr;
		int32_t iTitleOffX, iTitleOffY;
		uint8_t byTitleAlign;
		bool fTitleBookFont;
		bool fImgDump; // developer help: Dump background image part

		C4Rect rcOverlayPos;
		FLOAT_RECT rcfOverlayPos;

		// set during initialization
		C4FacetExSurface fctBase, fctOverlay;
		C4ScenarioListLoader::Entry *pScenEntry;
		C4GUI::Button *pBtn; // used to resolve button events to scenario

		void CompileFunc(StdCompiler *pComp);
	};

	struct AccessGfx
	{
		// compiled data
		StdStrBuf sPassword;
		StdStrBuf sOverlayImage;
		C4Rect rcOverlayPos;
		FLOAT_RECT rcfOverlayPos;

		// set during initialization
		C4FacetExSurface fctOverlay;

		void CompileFunc(StdCompiler *pComp);
	};

	// non-interactive map item
	class MapPic : public C4GUI::Picture
	{
	private:
		FLOAT_RECT rcfBounds; // drawing bounds

	public:
		MapPic(const FLOAT_RECT &rcfBounds, const C4Facet &rfct);

	protected:
		virtual void MouseInput(C4GUI::CMouse &rMouse, int32_t iButton, int32_t iX, int32_t iY, uint32_t dwKeyParam) override; // input: mouse movement or buttons - deselect everything if clicked
		virtual void DrawElement(C4FacetEx &cgo) override; // draw the image
	};

private:
	C4FacetExSurface fctBackgroundPicture; FLOAT_RECT rcfBG;
	bool fCoordinatesAdjusted;
	C4Rect rcScenInfoArea; // area in which scenario info is displayed
	class C4ScenarioListLoader::Folder *pScenarioFolder;
	class C4ScenarioListLoader::Entry *pSelectedEntry;
	C4GUI::TextWindow *pSelectionInfoBox;
	int32_t MinResX, MinResY; // minimum resolution for display of the map
	bool fUseFullscreenMap;
	bool hideTitle;
	Scenario **ppScenList; int32_t iScenCount;
	AccessGfx **ppAccessGfxList; int32_t iAccessGfxCount;
	class C4StartupScenSelDlg *pMainDlg;

public:
	C4MapFolderData() : fCoordinatesAdjusted(false), iScenCount(0), ppScenList(nullptr), iAccessGfxCount(0), ppAccessGfxList(nullptr), pMainDlg(nullptr) {}
	~C4MapFolderData() { Clear(); }

private:
	void ConvertFacet2ScreenCoord(const C4Rect &rc, FLOAT_RECT *pfrc, float fBGZoomX, float fBGZoomY, int iOffX, int iOffY);
	void ConvertFacet2ScreenCoord(int32_t *piValue, float fBGZoom, int iOff);
	void ConvertFacet2ScreenCoord(C4Rect &rcMapArea, bool fAspect); // adjust coordinates of loaded facets so they match given area

protected:
	bool OnButtonScenario(C4GUI::Control *pEl);

	friend class C4StartupScenSelDlg;

public:
	void Clear();
	bool Load(C4Group &hGroup, C4ScenarioListLoader::Folder *pScenLoaderFolder);
	void CompileFunc(StdCompiler *pComp);
	void CreateGUIElements(C4StartupScenSelDlg *pMainDlg, C4GUI::Window &rContainer);
	void ResetSelection();

	C4GUI::TextWindow *GetSelectionInfoBox() const { return pSelectionInfoBox; }
	C4ScenarioListLoader::Entry *GetSelectedEntry() const { return pSelectedEntry; }
};

// startup dialog: Scenario selection
class C4StartupScenSelDlg : public C4StartupDlg
{
public:
	// one item in the scenario list
	class ScenListItem : public C4GUI::Window
	{
	private:
		typedef C4GUI::Window BaseClass;
		// subcomponents
		C4GUI::Picture *pIcon; // item icon
		C4GUI::Label *pNameLabel; // item caption
		C4ScenarioListLoader::Entry *pScenListEntry; // associated, loaded item info

	public:
		ScenListItem(C4GUI::ListBox *pForListBox, C4ScenarioListLoader::Entry *pForEntry, C4GUI::Element *pInsertBeforeElement = nullptr);

	protected:
		struct RenameParams {};
		void AbortRenaming(RenameParams par);
		C4GUI::RenameEdit::RenameResult DoRenaming(RenameParams par, const char *szNewName);

	public:
		bool KeyRename();

	protected:
		virtual void UpdateOwnPos() override; // recalculate item positioning
		virtual void MouseInput(C4GUI::CMouse &rMouse, int32_t iButton, int32_t iX, int32_t iY, uint32_t dwKeyParam) override;

		void Update() {}

	public:
		C4ScenarioListLoader::Entry *GetEntry() const { return pScenListEntry; }
		ScenListItem *GetNext() { return static_cast<ScenListItem *>(BaseClass::GetNext()); }

		virtual bool CheckNameHotkey(const char *c) override; // return whether this item can be selected by entering given char
	};

public:
	C4StartupScenSelDlg(bool fNetwork);
	~C4StartupScenSelDlg();

private:
	enum { ShowStyle_Book = 0, ShowStyle_Map = 1, };
	enum { IconLabelSpacing = 2 }; // space between an icon and its text

	// book style scenario selection
	C4GUI::Label *pScenSelCaption; // caption label atop scenario list; indicating current folder
	C4GUI::ListBox *pScenSelList; // left page of book: Scenario selection
	C4GUI::Label *pScenSelProgressLabel; // progress label shown while scenario list is being generated
	C4GUI::TextWindow *pSelectionInfo; // used to display the description of the current selection

	std::unique_ptr<C4KeyBinding> keyRefresh, keyBack, keyForward, keyRename, keyDelete, keyCheat, keySearch, keyEscape;
	class C4GameOptionButtons *pGameOptionButtons;
	C4GUI::Button *pOpenBtn;
	C4GUI::Tabular *pScenSelStyleTabular;
	C4GUI::Edit *searchBar;

	C4ScenarioListLoader *pScenLoader;

	// map style scenario selection
	C4MapFolderData *pMapData;
	C4FacetEx *pfctBackground;

	bool fIsInitialLoading;
	bool fStartNetworkGame;

	C4GUI::RenameEdit *pRenameEdit;
	C4GUI::CheckBox *btnAllowUserChange;

public:
	static C4StartupScenSelDlg *pInstance; // singleton

protected:
	virtual int32_t GetMarginTop() override { return (rcBounds.Hgt / 7); }
	virtual bool HasBackground() override { return true; }
	virtual void DrawElement(C4FacetEx &cgo) override;
	void HideTitle(bool hide = false);

	virtual bool OnEnter() override { DoOK(); return true; }
	virtual bool OnEscape() override { DoBack(true); return true; }
	bool KeyBack() { return DoBack(true); }
	bool KeyRefresh() { DoRefresh(); return true; }
	bool KeyForward() { DoOK(); return true; }
	bool KeyRename();
	bool KeyDelete();
	bool KeyCheat();
	bool KeySearch();
	bool KeyEscape();
	void KeyCheat2(const StdStrBuf &rsCheatCode);

	void DeleteConfirm(ScenListItem *pSel);

	virtual void OnShown() override; // callback when shown: Init file list
	virtual void OnClosed(bool fOK) override; // callback when dlg got closed: Return to main screen
	void OnBackBtn(C4GUI::Control *btn) { DoBack(true); }
	void OnNextBtn(C4GUI::Control *btn) { DoOK(); }
	void OnSelChange(class C4GUI::Element *pEl) { UpdateSelection(); }
	void OnSelDblClick(class C4GUI::Element *pEl) { DoOK(); }
	void UpdateUseCrewBtn();
	void OnButtonScenario(C4GUI::Control *pEl);

	C4GUI::Edit::InputResult OnSearchBarEnter(C4GUI::Edit *edt, bool fPasting, bool fPastingMore)
	{
		UpdateList(); return C4GUI::Edit::IR_Abort;
	}

	friend class C4MapFolderData;

private:
	void UpdateList();
	void UpdateSelection();
	void ResortFolder();
	ScenListItem *GetSelectedItem();
	C4ScenarioListLoader::Entry *GetSelectedEntry();
	void SetOpenButtonDefaultText();
	void FocusScenList();

public:
	bool StartScenario(C4ScenarioListLoader::Scenario *pStartScen);
	bool OpenFolder(C4ScenarioListLoader::Folder *pNewFolder);
	void ProcessCallback() { UpdateList(); } // process callback by loader
	bool DoOK(); // open/start currently selected item
	bool DoBack(bool fAllowClose); // back folder, or abort dialog
	void DoRefresh(); // refresh file list
	void DeselectAll(); // reset focus and update selection info

	void StartRenaming(C4GUI::RenameEdit *pNewRenameEdit);
	void AbortRenaming();
	bool IsRenaming() const { return !!pRenameEdit; }
	void SetRenamingDone() { pRenameEdit = nullptr; }

	void SetBackground(C4FacetEx *pNewBG) { pfctBackground = pNewBG; }

	bool IsNetworkStart() const { return fStartNetworkGame; }

	friend class ScenListItem;
};
