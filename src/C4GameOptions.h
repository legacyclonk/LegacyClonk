/* by Sven2, 2005 */
// Custom game options and configuration dialog

#pragma once

#include "C4Gui.h"

class C4GameOptionsList;

// options dialog: created as listbox inside another dialog
// used to configure some standard runtime options, as well as custom game options
class C4GameOptionsList : public C4GUI::ListBox
{
public:
	enum { IconLabelSpacing = 2 }; // space between an icon and its text

private:
	class Option : public C4GUI::Control
	{
	protected:
		typedef C4GUI::Control BaseClass;

		// primary subcomponent: forward focus to this element
		C4GUI::Control *pPrimarySubcomponent;

		virtual bool IsFocused(C4GUI::Control *pCtrl)
		{
			// also forward own focus to primary control
			return BaseClass::IsFocused(pCtrl) || (HasFocus() && pPrimarySubcomponent == pCtrl);
		}

	public:
		Option(class C4GameOptionsList *pForDlg); // adds to list
		void InitOption(C4GameOptionsList *pForDlg); // add to list and do initial update

		virtual void Update() {}; // update data

		Option *GetNext() { return static_cast<Option *>(BaseClass::GetNext()); }
	};

	// dropdown list option
	class OptionDropdown : public Option
	{
	public:
		OptionDropdown(class C4GameOptionsList *pForDlg, const char *szCaption, bool fReadOnly);

	protected:
		C4GUI::Label *pCaption;
		C4GUI::ComboBox *pDropdownList;

		virtual void DoDropdownFill(C4GUI::ComboBox_FillCB *pFiller) = 0;

		void OnDropdownFill(C4GUI::ComboBox_FillCB *pFiller)
		{
			DoDropdownFill(pFiller);
		}

		virtual void DoDropdownSelChange(int32_t idNewSelection) = 0;

		bool OnDropdownSelChange(C4GUI::ComboBox *pForCombo, int32_t idNewSelection)
		{
			DoDropdownSelChange(idNewSelection); Update(); return true;
		}
	};

	// drop down list to specify central/decentral control mode
	class OptionControlMode : public OptionDropdown
	{
	public:
		OptionControlMode(class C4GameOptionsList *pForDlg);

	protected:
		virtual void DoDropdownFill(C4GUI::ComboBox_FillCB *pFiller);
		virtual void DoDropdownSelChange(int32_t idNewSelection);

		virtual void Update(); // update data to current control rate
	};

	// drop down list option to adjust control rate
	class OptionControlRate : public OptionDropdown
	{
	public:
		OptionControlRate(class C4GameOptionsList *pForDlg);

	protected:
		virtual void DoDropdownFill(C4GUI::ComboBox_FillCB *pFiller);
		virtual void DoDropdownSelChange(int32_t idNewSelection);

		virtual void Update(); // update data to current control rate
	};

	// drop down list option to adjust team usage
	class OptionTeamDist : public OptionDropdown
	{
	public:
		OptionTeamDist(class C4GameOptionsList *pForDlg);

	protected:
		virtual void DoDropdownFill(C4GUI::ComboBox_FillCB *pFiller);
		virtual void DoDropdownSelChange(int32_t idNewSelection);

		virtual void Update(); // update data to current team mode
	};

	// drop down list option to adjust team color state
	class OptionTeamColors : public OptionDropdown
	{
	public:
		OptionTeamColors(class C4GameOptionsList *pForDlg);

	protected:
		virtual void DoDropdownFill(C4GUI::ComboBox_FillCB *pFiller);
		virtual void DoDropdownSelChange(int32_t idNewSelection);

		virtual void Update(); // update data to current team color mode
	};

	// drop down list option to adjust control rate
	class OptionRuntimeJoin : public OptionDropdown
	{
	public:
		OptionRuntimeJoin(class C4GameOptionsList *pForDlg);

	protected:
		virtual void DoDropdownFill(C4GUI::ComboBox_FillCB *pFiller);
		virtual void DoDropdownSelChange(int32_t idNewSelection);

		virtual void Update(); // update data to current runtime join state
	};

public:
	C4GameOptionsList(const C4Rect &rcBounds, bool fActive, bool fRuntime);
	~C4GameOptionsList() { Deactivate(); }

private:
	C4Sec1TimerCallback<C4GameOptionsList> *pSec1Timer; // engine timer hook for updates
	bool fRuntime; // set for runtime options dialog - does not provide pre-game options such as team colors

	void InitOptions(); // creates option selection components

public:
	// update all option flags by current game state
	void Update();
	void OnSec1Timer() { Update(); }

	// activate/deactivate periodic updates
	void Activate();
	void Deactivate();

	// config
	bool IsTabular() const { return fRuntime; } // wide runtime dialog allows tabular layout
	bool IsRuntime() const { return fRuntime; }
};
