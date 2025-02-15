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

// Startup screen for non-parameterized engine start: Player selection dialog
// Also contains player creation, editing and crew management

#pragma once

#include "C4InfoCore.h"
#include "C4Group.h"
#include "C4Startup.h"

// startup dialog: Player selection
class C4StartupPlrSelDlg : public C4StartupDlg
{
private:
	enum Mode { PSDM_Player = 0, PSDM_Crew }; // player selection list, or crew editing mode
	enum { IconLabelSpacing = 2 }; // space between an icon and its text

private:
	// one item in the player or crew list
	class ListItem : public C4GUI::Control
	{
	private:
		typedef C4GUI::Window BaseClass;
		// subcomponents

	protected:
		C4GUI::CheckBox *pCheck; // check box to mark participation
		C4GUI::Label *pNameLabel; // item caption
		class C4StartupPlrSelDlg *pPlrSelDlg;
		C4GUI::Icon *pIcon; // item icon

	private:
		class C4KeyBinding *pKeyCheck; // space activates/deactivates selected player
		C4FacetExSurface fctPortrait, fctPortraitBase; // big portrait
		StdStrBuf Filename; // file info was loaded from

	public:
		ListItem(C4StartupPlrSelDlg *pForDlg, C4GUI::ListBox *pForListBox, C4GUI::Element *pInsertBeforeElement = nullptr, bool fActivated = false);
		virtual ~ListItem();

	protected:
		virtual C4GUI::ContextMenu *ContextMenu() = 0;

		C4GUI::ContextMenu *ContextMenu(C4GUI::Element *pEl, int32_t iX, int32_t iY)
		{
			return ContextMenu();
		}

		void LoadPortrait(C4Group &rGrp, bool fUseDefault);
		void CreateColoredPortrait();
		void SetDefaultPortrait();

		virtual void UpdateOwnPos() override; // recalculate item positioning
		bool KeyCheck() { pCheck->ToggleCheck(true); return true; }
		virtual bool IsFocusOnClick() override { return false; } // do not focus; keep focus on listbox

		void SetName(const char *szNewName);
		void SetIcon(C4GUI::Icons icoNew);

		void SetFilename(const StdStrBuf &sNewFN);

	public:
		ListItem *GetNext() const { return static_cast<ListItem *>(BaseClass::GetNext()); }
		const C4FacetEx &GetPortrait() const { return fctPortrait; }
		virtual uint32_t GetColorDw() const = 0; // get drawing color for portrait
		bool IsActivated() const { return pCheck->GetChecked(); }
		void SetActivated(bool fToVal) { pCheck->SetChecked(fToVal); }
		const char *GetName() const;
		virtual void SetSelectionInfo(C4GUI::TextWindow *pSelectionInfo) = 0; // clears text field and writes selection info text into it
		const StdStrBuf &GetFilename() const { return Filename; }
		virtual std::string GetDelWarning() = 0;
		void GrabIcon(C4FacetExSurface &rFromFacet);
		void GrabPortrait(C4FacetExSurface *pFromFacet);

		virtual bool CheckNameHotkey(const char *c) override; // return whether this item can be selected by entering given char

		class LoadError : public std::runtime_error
		{
		public:
			using runtime_error::runtime_error;
		}; // class thrown off load function if load failed
	};

public:
	// a list item when in player selection mode
	class PlayerListItem : public ListItem
	{
	private:
		C4PlayerInfoCore Core; // player info core loaded from player file
		bool fHasCustomIcon; // set for players with a BigIcon.png

	public:
		PlayerListItem(C4StartupPlrSelDlg *pForDlg, C4GUI::ListBox *pForListBox, C4GUI::Element *pInsertBeforeElement = nullptr, bool fActivated = false);
		~PlayerListItem() {}

		void Load(const StdStrBuf &rsFilename); // may throw LoadError

	protected:
		virtual C4GUI::ContextMenu *ContextMenu() override;

	public:
		const C4PlayerInfoCore &GetCore() const { return Core; }
		void UpdateCore(C4PlayerInfoCore &NewCore); // Save Core to disk and update this item
		void GrabCustomIcon(C4FacetExSurface &fctGrabFrom);
		virtual void SetSelectionInfo(C4GUI::TextWindow *pSelectionInfo) override;
		virtual uint32_t GetColorDw() const override { return Core.PrefColorDw; }
		virtual std::string GetDelWarning() override;
		bool MoveFilename(const char *szToFilename); // change filename to given
	};

private:
	// a list item when in crew editing mode
	class CrewListItem : public ListItem
	{
	private:
		bool fLoaded;
		C4ObjectInfoCore Core;
		uint32_t dwPlrClr;
		C4Group *pParentGrp;

	public:
		CrewListItem(C4StartupPlrSelDlg *pForDlg, C4GUI::ListBox *pForListBox, uint32_t dwPlrClr);
		~CrewListItem() {}

		void Load(C4Group &rGrp, const StdStrBuf &rsFilename); // may throw LoadError

	protected:
		virtual C4GUI::ContextMenu *ContextMenu() override;

		void RewriteCore();

		struct RenameParams {};
		void AbortRenaming(RenameParams par);
		C4GUI::RenameResult DoRenaming(RenameParams par, const char *szNewName);

	private:
		std::string GetPhysicalTextLine(int32_t iPhysValue, C4ResStrTableKeyFormat<> idsName); // get string for physical info bar

	public:
		void UpdateClonkEnabled();

		virtual uint32_t GetColorDw() const override { return dwPlrClr; } // get drawing color for portrait
		virtual void SetSelectionInfo(C4GUI::TextWindow *pSelectionInfo) override; // clears text field and writes selection info text into it
		virtual std::string GetDelWarning() override;
		const C4ObjectInfoCore &GetCore() const { return Core; }

		CrewListItem *GetNext() const { return static_cast<CrewListItem *>(ListItem::GetNext()); }

		void CrewRename(); // shows the edit-field to rename a crew member
		bool SetName(const char *szNewName); // update clonk name and core
		void OnDeathMessageCtx(C4GUI::Element *el);
		void OnDeathMessageSet(const StdStrBuf &rsNewMessage);
	};

public:
	C4StartupPlrSelDlg();
	~C4StartupPlrSelDlg();

private:
	class C4KeyBinding *pKeyBack, *pKeyProperties, *pKeyCrew, *pKeyDelete, *pKeyRename, *pKeyNew;
	class C4GUI::ListBox *pPlrListBox;
	C4GUI::TextWindow *pSelectionInfo;
	class C4GUI::Picture *pPortraitPict;
	Mode eMode;

	// in crew mode:
	struct CurrPlayer_t
	{
		C4PlayerInfoCore Core; // loaded player main core
		C4Group Grp; // group to player file; opened when in crew mode
	}
	CurrPlayer;

private:
	C4Rect rcBottomButtons; int32_t iBottomButtonWidth;
	class C4GUI::Button *btnActivatePlr, *btnCrew, *btnProperties, *btnDelete, *btnBack, *btnNew;

	void UpdateBottomButtons(); // update command button texts and positions
	void UpdatePlayerList(); // refill pPlrListBox with players in player folder, or with crew in selected player
	void UpdateSelection();
	void OnSelChange(class C4GUI::Element *pEl) { UpdateSelection(); }
	void OnSelDblClick(class C4GUI::Element *pEl) { C4GUI::GUISound("Click"); OnPropertyBtn(nullptr); }
	void UpdateActivatedPlayers(); // update Config.General.Participants by currently activated players
	void SelectItem(const std::string &filename, bool fActivate); // find item by filename and select (and activate it, if desired)

	void SetPlayerMode(); // change view to listing players
	void SetCrewMode(PlayerListItem *pForPlayer); // change view to listing crew of a player

	static int32_t CrewSortFunc(const C4GUI::Element *pEl1, const C4GUI::Element *pEl2, void *par);
	void ResortCrew();

protected:
	void OnItemCheckChange(C4GUI::Element *pCheckBox);
	static bool CheckPlayerName(const StdStrBuf &Playername, std::string &filename, const StdStrBuf *pPrevFilename, bool fWarnEmpty);
	ListItem *GetSelection();
	void SetSelection(ListItem *pNewItem);

	C4GUI::RenameEdit *pRenameEdit; // hack: set by crew list item renaming. Must be cleared when something is done in the dlg
	void AbortRenaming();

	friend class ListItem; friend class PlayerListItem; friend class CrewListItem;
	friend class C4StartupPlrPropertiesDlg;

protected:
	virtual int32_t GetMarginTop() override { return (rcBounds.Hgt / 7); }
	virtual bool HasBackground() override { return true; }
	virtual void DrawElement(C4FacetEx &cgo) override;

	virtual bool OnEnter() override { return false; } // Enter ignored
	virtual bool OnEscape() override { DoBack(); return true; }
	bool KeyBack()          { DoBack(); return true; }
	bool KeyProperties() { OnPropertyBtn(nullptr); return true; }
	bool KeyCrew()       { OnCrewBtn    (nullptr); return true; }
	bool KeyDelete()     { OnDelBtn     (nullptr); return true; }
	bool KeyNew()        { OnNewBtn     (nullptr); return true; }

	void OnNewBtn(C4GUI::Control *btn);
	void OnActivateBtn(C4GUI::Control *btn);
	void OnPropertyBtn(C4GUI::Control *btn);
	void OnPropertyCtx(C4GUI::Element *el) { OnPropertyBtn(nullptr); }
	void OnCrewBtn(C4GUI::Control *btn);
	void OnDelBtn(C4GUI::Control *btn);
	void OnDelCtx(C4GUI::Element *el) { OnDelBtn(nullptr); }
	void OnDelBtnConfirm(ListItem *pSel);
	void OnBackBtn(C4GUI::Control *btn) { DoBack(); }

public:
	void DoBack(); // back to main menu
};

// player creation or property editing dialog
class C4StartupPlrPropertiesDlg : public C4GUI::Dialog
{
protected:
	C4StartupPlrSelDlg *pMainDlg; // may be nullptr if shown as creation dialog in main menu!
	C4StartupPlrSelDlg::PlayerListItem *pForPlayer;
	C4GUI::Edit *pNameEdit; // player name edit box
	C4GUI::Picture *pClrPreview;
	C4GUI::ScrollBar *pClrSliderR, *pClrSliderG, *pClrSliderB;
	C4GUI::Picture *pCtrlImg;
	C4GUI::IconButton *pMouseBtn, *pJumpNRunBtn, *pClassicBtn, *pPictureBtn;
	C4PlayerInfoCore C4P; // player info core copy currently being edited
	C4FacetExSurface fctOldBigIcon;
	C4FacetExSurface fctNewPicture, fctNewBigIcon; // if assigned, save new picture/bigicon
	bool fClearPicture, fClearBigIcon; // if true, delete current picture/bigicon
	virtual const char *GetID() override { return "PlrPropertiesDlg"; }

	void DrawElement(C4FacetEx &cgo) override;
	virtual int32_t GetMarginTop() override    { return 16; }
	virtual int32_t GetMarginLeft() override   { return 45; }
	virtual int32_t GetMarginRight() override  { return 55; }
	virtual int32_t GetMarginBottom() override { return 30; }

	virtual void UserClose(bool fOK) override; // OK only with a valid name
	virtual bool IsComponentOutsideClientArea() override { return true; } // OK and close btn

	void OnClrChangeLeft(C4GUI::Control *pBtn);
	void OnClrChangeRight(C4GUI::Control *pBtn);
	void OnClrSliderRChange(int32_t iNewVal);
	void OnClrSliderGChange(int32_t iNewVal);
	void OnClrSliderBChange(int32_t iNewVal);
	void OnCtrlChangeLeft(C4GUI::Control *pBtn);
	void OnCtrlChangeRight(C4GUI::Control *pBtn);
	void OnCtrlChangeMouse(C4GUI::Control *pBtn);
	void OnMovementBtn(C4GUI::Control *pBtn);
	void OnPictureBtn(C4GUI::Control *pBtn);

private:
	void UpdatePlayerColor(bool fUpdateSliders);
	void UpdatePlayerControl();
	void UpdatePlayerMovement(); // updates Jump'n'Run vs Classic
	void UpdateBigIcon();

	bool SetNewPicture(C4Surface &srcSfc, C4FacetExSurface *trgFct, int32_t iMaxSize, bool fColorize);
	void SetNewPicture(const char *szFromFilename, bool fSetPicture, bool fSetBigIcon); // set new picture/bigicon by loading and scaling if necessary. If szFromFilename==nullptr, clear picture/bigicon

public:
	C4StartupPlrPropertiesDlg(C4StartupPlrSelDlg::PlayerListItem *pForPlayer, C4StartupPlrSelDlg *pMainDlg);
	~C4StartupPlrPropertiesDlg() {}

	virtual void OnClosed(bool fOK) override; // close CB
};
