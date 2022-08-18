/*
 * LegacyClonk
 *
 * Copyright (c) RedWolf Design
 * Copyright (c) 2005, Sven2
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

// Startup screen for non-parameterized engine start: Options dialog

#pragma once

#include "C4InputValidation.h"
#include "C4Startup.h"

// startup dialog: Options
class C4StartupOptionsDlg : public C4StartupDlg
{
	// main dlg stuff

public:
	C4StartupOptionsDlg();
	~C4StartupOptionsDlg();

private:
	class C4KeyBinding *pKeyBack;
	bool fConfigSaved; // set when config has been saved because dlg is closed; prevents double save
	bool fCanGoBack; // set if dlg has not been recreated yet, in which case going back to a previous dialog is not possible
	C4GUI::Tabular *pOptionsTabular;

protected:
	virtual bool OnEnter() override { return false; } // Enter ignored
	virtual bool OnEscape() override { DoBack(); return true; }
	bool KeyBack() { DoBack(); return true; }
	virtual void OnClosed(bool fOK) override; // callback when dlg got closed - save config

	virtual void UserClose(bool fOK) override // callback when the user tried to close the dialog (e.g., by pressing Enter in an edit) - just ignore this and save config instead
	{
		if (fOK) SaveConfig(false, true);
	}

	void OnBackBtn(C4GUI::Control *btn) { DoBack(); }

	bool SaveConfig(bool fForce, bool fKeepOpen); // save any config fields that are not stored directly; return whether all values are OK

public:
	void DoBack(); // back to main menu

public:
	void RecreateDialog(bool fFade);

	// program tab
private:
	// button without fancy background
	class SmallButton : public C4GUI::Button
	{
	protected:
		virtual void DrawElement(C4FacetEx &cgo) override; // draw the button

	public:
		SmallButton(const char *szText, const C4Rect &rtBounds)
			: C4GUI::Button(szText, rtBounds) {}
		static int32_t GetDefaultButtonHeight();
	};

	class C4GUI::ComboBox *pLangCombo;
	class C4GUI::Label *pLangInfoLabel;
	class C4GUI::ComboBox *pFontFaceCombo, *pFontSizeCombo;
	class C4GUI::ComboBox *pDisplayModeCombo;

	void OnLangComboFill(C4GUI::ComboBox_FillCB *pFiller);
	bool OnLangComboSelChange(C4GUI::ComboBox *pForCombo, int32_t idNewSelection);
	void UpdateLanguage();
	void OnFontFaceComboFill(C4GUI::ComboBox_FillCB *pFiller);
	void OnFontSizeComboFill(C4GUI::ComboBox_FillCB *pFiller);
	bool OnFontComboSelChange(C4GUI::ComboBox *pForCombo, int32_t idNewSelection);
	void UpdateFontControls();
	int32_t FairCrewSlider2Strength(int32_t iSliderVal);
	int32_t FairCrewStrength2Slider(int32_t iStrengthVal);
	void OnFairCrewStrengthSliderChange(int32_t iNewVal);
	void OnResetConfigBtn(C4GUI::Control *btn);
	void OnDisplayModeComboFill(C4GUI::ComboBox_FillCB *pFiller);
	bool OnDisplayModeComboSelChange(C4GUI::ComboBox *pForCombo, int32_t idNewSelection);

	// graphics tab
private:
	// checkbox that changes a config boolean directly
	class BoolConfig : public C4GUI::CheckBox
	{
	private:
		bool *pbVal, fInvert; bool *pbRestartChangeCfgVal;

	protected:
		void OnCheckChange(C4GUI::Element *pCheckBox);

	public:
		BoolConfig(const C4Rect &rcBounds, const char *szName, bool *pbVal, bool fInvert = false, bool *pbRestartChangeCfgVal = nullptr);
	};
	// editbox below descriptive label sharing one window for common tooltip
	class EditConfig : public C4GUI::LabeledEdit
	{
	public:
		EditConfig(const C4Rect &rcBounds, const char *szName, ValidatedStdStrBufBase *psConfigVal, int32_t *piConfigVal, bool fMultiline);

	private:
		ValidatedStdStrBufBase *psConfigVal;
		int32_t *piConfigVal;

	public:
		void Save2Config(); // control to config
		static bool GetControlSize(int *piWdt, int *piHgt, const char *szForText, bool fMultiline);
		int32_t GetIntVal() { return atoi(GetEdit()->GetText()); }
		void SetIntVal(int32_t iToVal) { GetEdit()->SetText(FormatString("%d", static_cast<int>(iToVal)).getData(), false); }
	} *pNetworkNameEdit, *pNetworkNickEdit;

	// message dialog with a timer; used to restore the resolution if the user didn't press anything for a while
	class ResChangeConfirmDlg : public C4GUI::TimedDialog
	{
	public:
		ResChangeConfirmDlg();
		~ResChangeConfirmDlg() = default;

	protected:
		virtual bool OnEnter() override { return true; } // Pressing Enter does not confirm this dialog, so "blind" users are less likely to accept their change
		virtual const char *GetID() override { return "ResChangeConfirmDialog"; }
		void UpdateText() override;
	};

	void OnGfxTroubleCheck(C4GUI::Element *pCheckBox)
	{
		SaveGfxTroubleshoot();
	} // immediate save and test
	void OnEffectsSliderChange(int32_t iNewVal);

	C4GUI::GroupBox *pGroupTrouble;
	C4GUI::CheckBox *pCheckGfxNoAlphaAdd, *pCheckGfxPointFilter, *pCheckGfxNoAddBlit, *pCheckGfxNoBoxFades;
	BoolConfig *pCheckGfxDisableGamma;
	EditConfig *pEdtGfxTexIndent, *pEdtGfxBlitOff;
	C4GUI::ScrollBar *pEffectLevelSlider;

	void LoadGfxTroubleshoot(); void SaveGfxTroubleshoot();

	class ScaleEdit : public C4GUI::Edit
	{
		C4StartupOptionsDlg *pDlg;

	public:
		ScaleEdit(C4StartupOptionsDlg *pDlg, const C4Rect &rtBounds, bool fFocusEdit = false);

	protected:
		virtual bool CharIn(const char *c) override;
		virtual InputResult OnFinishInput(bool fPasting, bool fPastingMore) override;
	};

	C4GUI::ScrollBar *pScaleSlider;
	ScaleEdit *pScaleEdit;
	int32_t iNewScale;

	void OnScaleSliderChanged(int32_t val);
	void OnTestScaleBtn(C4GUI::Control *);

	// sound tab
private:
	C4KeyBinding *pKeyToggleMusic; // extra key binding for music: Must also update checkbox in real-time
	C4GUI::CheckBox *pFEMusicCheck;

	void OnFEMusicCheck(C4GUI::Element *pCheckBox); // toggling frontend music is reflected immediately
	void OnMusicVolumeSliderChange(int32_t iNewVal);
	void OnSoundVolumeSliderChange(int32_t iNewVal);

protected:
	bool KeyMusicToggle();

	// keyboard and gamepad control tabs
private:
	// dialog shown to user so he can select a key
	class KeySelDialog : public C4GUI::MessageDialog
	{
	private:
		class C4KeyBinding *pKeyListener;
		C4KeyCode key;
		bool fGamepad;
		int32_t iCtrlSet;

	protected:
		bool KeyDown(C4KeyCodeEx key);

	public:
		KeySelDialog(int32_t iKeyID, int32_t iCtrlSet, bool fGamepad);
		virtual ~KeySelDialog();

		C4KeyCode GetKeyCode() { return key; }
	};

	// Clonk-key-button with a label showing its key name beside it
	class KeySelButton : public C4GUI::IconButton
	{
	private:
		int32_t iKeyID;
		C4KeyCode key;

	protected:
		virtual void DrawElement(C4FacetEx &cgo) override;
		virtual bool IsComponentOutsideClientArea() override { return true; }

	public:
		KeySelButton(int32_t iKeyID, const C4Rect &rcBounds, char cHotkey);
		void SetKey(C4KeyCode keyTo) { key = keyTo; }
	};

	// config area to define a keyboard set
	class ControlConfigArea : public C4GUI::Window
	{
	private:
		bool fGamepad; // if set, gamepad control sets are being configured
		int32_t iMaxControlSets; // number of control sets configured in this area - must be smaller or equal to C4MaxControlSet
		int32_t iSelectedCtrlSet; // keyboard or gamepad set that is currently being configured
		class C4GUI::IconButton **ppKeyControlSetBtns; // buttons to select configured control set - array in length of iMaxControlSets
		class KeySelButton *KeyControlBtns[C4MaxKey]; // buttons to configure individual kbd set buttons
		C4GamePadOpener *pGamepadOpener; // opened gamepad for configuration
		C4StartupOptionsDlg *pOptionsDlg;
		class C4GUI::CheckBox *pGUICtrl;

	public:
		ControlConfigArea(const C4Rect &rcArea, int32_t iHMargin, int32_t iVMargin, bool fGamepad, C4StartupOptionsDlg *pOptionsDlg);
		virtual ~ControlConfigArea();

	protected:
		void UpdateCtrlSet();

		void OnCtrlSetBtn(C4GUI::Control *btn);
		void OnCtrlKeyBtn(C4GUI::Control *btn);
		void OnResetKeysBtn(C4GUI::Control *btn);
		void OnGUIGamepadCheckChange(C4GUI::Element *pCheckBox);
	};

	// network tab
private:
	// checkbox to enable protocol and editbox to input port number
	class NetworkPortConfig : public C4GUI::Window
	{
	public:
		NetworkPortConfig(const C4Rect &rcBounds, const char *szName, int32_t *pConfigValue, int32_t iDefault);

	private:
		int32_t *pConfigValue; // pointer into config set
		C4GUI::CheckBox *pEnableCheck; // check box for whether port is enabled
		C4GUI::Edit *pPortEdit; // edit field for port number

	public:
		void OnEnabledCheckChange(C4GUI::Element *pCheckBox); // callback when checkbox is ticked
		void SavePort(); // controls to config
		int32_t GetPort(); // get port as currently configured by control (or 0 for deactivated)

		static bool GetControlSize(int *piWdt, int *piHgt);
	} *pPortCfgTCP, *pPortCfgUDP, *pPortCfgRef, *pPortCfgDsc;

	class NetworkServerAddressConfig : public C4GUI::Window
	{
	public:
		NetworkServerAddressConfig(const C4Rect &rcBounds, const char *szName, bool *pbConfigEnableValue, char *szConfigAddressValue, int iTabWidth);

	private:
		bool *pbConfigEnableValue; char *szConfigAddressValue;
		C4GUI::CheckBox *pEnableCheck;
		C4GUI::Edit *pAddressEdit;

	public:
		void OnEnabledCheckChange(C4GUI::Element *pCheckBox); // callback when checkbox is ticked
		void Save2Config(); // controls to config

		static bool GetControlSize(int *piWdt, int *piHgt, int *piTabPos, const char *szForText);
	} *pLeagueServerCfg;
};
