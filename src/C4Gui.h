/*
 * LegacyClonk
 *
 * Copyright (c) RedWolf Design
 * Copyright (c) 2001, Sven2
 * Copyright (c) 2017-2023, The LegacyClonk Team and contributors
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

// generic user interface
// defines user controls

// 2do:
//   mouse wheel processing
//   disabled buttons

#pragma once

#define ConsoleDlgClassName "C4GUIdlg"
#define ConsoleDlgWindowStyle (WS_VISIBLE | WS_POPUP | WS_SYSMENU | WS_CAPTION | WS_MINIMIZEBOX)

#include "C4FacetEx.h"
#include "C4ForwardDeclarations.h"
#include "C4GamePadCon.h"
#include "C4Id.h"
#include "C4KeyboardInput.h"
#include "C4LogBuf.h"
#include "C4Rect.h"

#include "StdFont.h"
#include "StdResStr2.h"
#include "StdWindow.h"

#include <cmath>
#include <cstdint>

class C4GroupSet;
class C4Viewport;

template<typename T>
class C4Sec1TimerCallback;

// consts (load those from a def file some time)
// font colors - alpha is font alpha, which is inversed opaque
#define C4GUI_CaptionFontClr          0xffffffff
#define C4GUI_Caption2FontClr         0xffffff00
#define C4GUI_InactCaptionFontClr     0xffafafaf
#define C4GUI_ButtonFontClr           0xffffff00
#define C4GUI_StatusFontClr           0xffffffff
#define C4GUI_MessageFontClr          0xffffffff
#define C4GUI_MessageFontAlpha        0xff000000
#define C4GUI_InactMessageFontClr     0xffafafaf
#define C4GUI_NotifyFontClr           0xffff0000
#define C4GUI_ComboFontClr            0xffffffff
#define C4GUI_CheckboxFontClr         0xffffffff
#define C4GUI_CheckboxDisabledFontClr 0xffafafaf
#define C4GUI_LogFontClr              0xffafafaf
#define C4GUI_LogFontClr2             0xffff1f1f
#define C4GUI_ErrorFontClr            0xffff1f1f
#define C4GUI_ProgressBarFontClr      0xffffffff
#define C4GUI_ContextFontClr          0xffffffff
#define C4GUI_GfxTabCaptActiveClr     0xff000000
#define C4GUI_GfxTabCaptInactiveClr   0xff000000
#define C4GUI_HyperlinkFontClr        0xff8080ff

// other colors
#define C4GUI_ImportantBGColor      0xcf00007f
#define C4GUI_ListBoxSelColor       0xafaf0000
#define C4GUI_ListBoxInactSelColor  0xaf7f7f7f
#define C4GUI_ContextSelColor       0xafaf0000
#define C4GUI_ContextBGColor        0x4f3f1a00
#define C4GUI_StandardBGColor       0x5f000000
#define C4GUI_ActiveTabBGColor      C4GUI_StandardBGColor
#define C4GUI_ListBoxBarColor       0x7f772200
#define C4GUI_EditBGColor           0x7f000000
#define C4GUI_EditFontColor         0xffffffff

#define C4GUI_ToolTipBGColor    0x00F1EA78
#define C4GUI_ToolTipFrameColor 0x7f000000
#define C4GUI_ToolTipColor      0xFF483222

// winner/loser color marking
#define C4GUI_WinningTextColor       0xffffdf00
#define C4GUI_WinningBackgroundColor 0x4faf7a00
#define C4GUI_LosingTextColor        0xffffffff
#define C4GUI_LosingBackgroundColor  0x7fafafaf

// border colors for 3D-frames
#define C4GUI_BorderAlpha   0xaf
#define C4GUI_BorderColor1  0x772200
#define C4GUI_BorderColor2  0x331100
#define C4GUI_BorderColor3  0xaa4400
#define C4GUI_BorderColorA1 (C4GUI_BorderAlpha << 24 | C4GUI_BorderColor1)
#define C4GUI_BorderColorA2 (C4GUI_BorderAlpha << 24 | C4GUI_BorderColor2)

// GUI icon sizes
#define C4GUI_IconWdt   40
#define C4GUI_IconHgt   40
#define C4GUI_IconExWdt 64
#define C4GUI_IconExHgt 64

// scroll bar size
#define C4GUI_ScrollBarWdt   16
#define C4GUI_ScrollBarHgt   16
#define C4GUI_ScrollArrowHgt 16
#define C4GUI_ScrollArrowWdt 16
#define C4GUI_ScrollThumbHgt 16 // only for non-dynamic scroll thumbs
#define C4GUI_ScrollThumbWdt 16 // only for non-dynamic scroll thumbs

// button size
#define C4GUI_ButtonHgt         32 // height of buttons
#define C4GUI_BigButtonHgt      40 // height of bigger buttons (main menu)
#define C4GUI_ButtonAreaHgt     40 // height of button areas
#define C4GUI_DefButtonWdt     140 // width of default buttons
#define C4GUI_DefButton2Wdt    120 // width of default buttons if there are two of them
#define C4GUI_DefButton2HSpace  10 // horzontal space between two def dlg buttons

#define C4GUI_CheckBoxLabelSpacing 4 // pixels between checkbox box and label

// list box item spacing
#define C4GUI_DefaultListSpacing  1 // 1 px of free space between two list items
#define C4GUI_ListBoxBarIndent   10

// default dialog box sizes
#define C4GUI_MessageDlgWdt       500 // width of message dialog
#define C4GUI_MessageDlgWdtMedium 360 // width of message dialog w/o much text
#define C4GUI_MessageDlgWdtSmall  300 // width of message dialog w/o much text
#define C4GUI_ProgressDlgWdt      500 // width of progress dialog
#define C4GUI_InputDlgWdt         300
#define C4GUI_DefDlgIndent         10 // indent for default dlg items
#define C4GUI_DefDlgSmallIndent     4 // indent for dlg items that are grouped
#define C4GUI_ProgressDlgVRoom    150 // height added to text height in progress dialog
#define C4GUI_InputDlgVRoom       150
#define C4GUI_ProgressDlgPBHgt     30 // height of progress bar in progress dlg
#define C4GUI_InfoDlgWdt          620 // width of info dialog
#define C4GUI_InfoDlgVRoom        100 // height added to text height in info dialog
#define C4GUI_MaxToolTipWdt       500 // maximum width for tooltip boxes

// time for tooltips to appear (msecs) -evaluated while drawing
#define C4GUI_ToolTipShowTime 500 // 0.5 seconds

// time for title bars to start scrolling to make longer text visible -evaluated while drawing
#define C4GUI_TitleAutoScrollTime 3000 // 3 seconds

// time interval for tab caption scrolling
#define C4GUI_TabCaptionScrollTime 500 // 0.5 seconds

// Z-ordering of dialogs
#define C4GUI_Z_CHAT    +2 // chat input dialog more important than other input dialogs
#define C4GUI_Z_INPUT   +1 // input dialogs on top of others
#define C4GUI_Z_DEFAULT  0 // normal placement on top of viewports

#define C4GUI_MinWoodBarHgt 23

#define C4GUI_FullscreenDlg_TitleHeight C4UpperBoardHeight // pixels reserved for top of fullscreen dialog title
#define C4GUI_FullscreenCaptionFontClr 0xffffff00

namespace C4GUI
{

// some class predefs

// C4Gui.cpp
class Element; class Screen; class CMouse;
class Resource; class ComponentAligner;

// C4GuiLabels.cpp
class Label; class WoodenLabel; class MultilineLabel;
class HorizontalLine; class ProgressBar;
class Picture; class Icon;
class TextWindow;

// C4GuiContainers.cpp
class Container; class Window; class GroupBox; class Control;
class ScrollBar; class ScrollWindow;

// C4GuiButton.cpp
class Button; template <class CallbackDlg, class Base> class CallbackButton;
class IconButton;
class CloseButton; class OKButton; class CancelButton;
class CloseIconButton; class OKIconButton; class CancelIconButton;

// C4GuiEdit.cpp
class Edit;
class LabeledEdit;
class RenameEdit;

enum InputResult // action to be taken when text is confirmed with enter
{
	IR_None = 0,  // do nothing and continue pasting
	IR_CloseDlg,  // stop any pastes and close parent dialog successfully
	IR_CloseEdit, // stop any pastes and remove this control
	IR_Abort,     // do nothing and stop any pastes
};

enum RenameResult
{
	RR_Invalid = 0, // rename not accepted; continue editing
	RR_Accepted,    // rename accepted; delete control
	RR_Deleted,     // control deleted - leave everything
};

// C4GuiCheckBox.cpp
class CheckBox;

// C4GuiListBox.cpp
class ListBox;

// C4GuiTabular.cpp
class Tabular;

// C4GuiMenu.cpp
class ContextMenu;

// C4GUIComboBox.cpp
class ComboBox;

// C4GuiDialog.cpp
class Dialog; class MessageDialog; class ProgressDialog;
class InputDialog; class InfoDialog; class TimedDialog;

// inline
class MenuHandler; class ContextHandler;

// expand text like "Te&xt" to "Te<c ffff00>x</c>t"
bool ExpandHotkeyMarkup(StdStrBuf &sText, char &rcHotkey);

// make color readable on black: max alpha to 0x1f, max color hues
uint32_t MakeColorReadableOnBlack(uint32_t &rdwClr);

struct FLOAT_RECT
{
	float left, right, top, bottom;

	// Surround floating point rectangle
	operator C4Rect() const
	{
		return
		{
			static_cast<std::int32_t>(left),
			static_cast<std::int32_t>(top),
			static_cast<std::int32_t>(ceilf(right)  - floorf(left)),
			static_cast<std::int32_t>(ceilf(bottom) - floorf(top))
		};
	}
};

// menu handler: generic context menu callback
class MenuHandler
{
public:
	MenuHandler() {}
	virtual ~MenuHandler() {}

	virtual void OnOK(Element *pTarget) = 0; // menu item selected
};

// context handler: opens context menu on right-click or menu key
class ContextHandler
{
private:
	int32_t iRefs;

public:
	ContextHandler() : iRefs(0) {}
	virtual ~ContextHandler() {}

	virtual bool OnContext(Element *pOnElement, int32_t iX, int32_t iY) = 0; // context queried - ret true if handled
	virtual ContextMenu *OnSubcontext(Element *pOnElement) = 0; // subcontext queried

	inline void Ref() { ++iRefs; }
	inline void DeRef() { if (!--iRefs) delete this; }
};

// generic callback handler
class BaseCallbackHandler
{
private:
	int32_t iRefs;

public:
	BaseCallbackHandler() : iRefs(0) {}
	virtual ~BaseCallbackHandler() {}

	inline void Ref() { ++iRefs; }
	inline void DeRef() { if (!--iRefs) delete this; }

	virtual void DoCall(class Element *pElement) = 0;
};

template <class CB> class CallbackHandler : public BaseCallbackHandler
{
public:
	typedef void (CB::*Func)(class Element *pElement);

private:
	CB *pCBClass;
	Func CBFunc;

public:
	virtual void DoCall(class Element *pElement) override
	{
		((pCBClass)->*CBFunc)(pElement);
	}

	CallbackHandler(CB *pTarget, Func rFunc) : pCBClass(pTarget), CBFunc(rFunc) {}
};

template <class CB> class CallbackHandlerNoPar : public BaseCallbackHandler
{
public:
	typedef void (CB::*Func)();

private:
	CB *pCBClass;
	Func CBFunc;

public:
	virtual void DoCall(class Element *pElement) override
	{
		((pCBClass)->*CBFunc)();
	}

	CallbackHandlerNoPar(CB *pTarget, Func rFunc) : pCBClass(pTarget), CBFunc(rFunc) {}
};

template <class CB, class ParType> class CallbackHandlerExPar : public BaseCallbackHandler
{
public:
	typedef void (CB::*Func)(ParType);

private:
	CB *pCBClass;
	ParType par;
	Func CBFunc;

public:
	virtual void DoCall(class Element *pElement) override
	{
		((pCBClass)->*CBFunc)(par);
	}

	CallbackHandlerExPar(CB *pTarget, Func rFunc, ParType par) : pCBClass(pTarget), CBFunc(rFunc), par(par) {}
};

// callback with parameter coming from calling class
template <class ParType> class BaseParCallbackHandler : public BaseCallbackHandler
{
private:
	virtual void DoCall(class Element *pElement) override { assert(false); } // no-par: Not to be called

public:
	BaseParCallbackHandler() {}

	virtual void DoCall(ParType par) = 0;
};

template <class CB, class ParType> class ParCallbackHandler : public BaseParCallbackHandler<ParType>
{
public:
	typedef void (CB::*Func)(ParType par);

private:
	CB *pCBClass;
	Func CBFunc;

public:
	virtual void DoCall(ParType par) override { ((pCBClass)->*CBFunc)(par); }

	ParCallbackHandler(CB *pTarget, Func rFunc) : pCBClass(pTarget), CBFunc(rFunc) {}
};

// three facets for left/top, middle and right/bottom of an auto-sized bar
struct DynBarFacet
{
	C4Facet fctBegin, fctMiddle, fctEnd;

	void SetHorizontal(C4Surface &rBySfc, int iHeight = 0, int iBorderWidth = 0);
	void SetHorizontal(C4Facet &rByFct, int32_t iBorderWidth = 0);
	void Clear() { fctBegin.Default(); fctMiddle.Default(); fctEnd.Default(); }
};

// facets used to draw a scroll bar
struct ScrollBarFacets
{
	DynBarFacet barScroll;
	C4Facet fctScrollDTop, fctScrollPin, fctScrollDBottom;

	void Set(const C4Facet &rByFct, int32_t iPinIndex = 0);
	void Clear() { barScroll.Clear(); fctScrollDTop.Default(); fctScrollPin.Default(); fctScrollDBottom.Default(); }
};

// a generic gui-element
class Element
{
private:
	StdStrBuf ToolTip; // MouseOver - status text

protected:
	Container *pParent; // owning container
	Element *pPrev, *pNext; // previous and next element of same container
	Window *pDragTarget; // target that is dragged around when the user drags this element
	int32_t iDragX, iDragY; // drag start pos
	bool fDragging; // if set, mouse is down on component and dragging enabled
	ContextHandler *pContextHandler; // context handler to be called upon context menu request

public:
	bool fVisible; // if false, component (and subcomponents) are not drawn

protected:
	C4Rect rcBounds; // element bounds

	virtual void Draw(C4FacetEx &cgo) { DrawElement(cgo); } // draw this class (this + any contents)
	virtual void DrawElement(C4FacetEx &cgo) {} // draw element itself

	virtual void RemoveElement(Element *pChild); // clear ptrs

	virtual void UpdateSize(); // called when own size changed
	virtual void UpdatePos();  // called when own position changed

	void Draw3DFrame(C4FacetEx &cgo, bool fUp = false, int32_t iIndent = 1, uint8_t byAlpha = C4GUI_BorderAlpha, bool fDrawTop = true, int32_t iTopOff = 0, bool fDrawLeft = true, int32_t iLeftOff = 0); // draw frame around element
	void DrawBar(C4FacetEx &cgo, DynBarFacet &rFacets); // draw gfx bar within element bounds
	void DrawVBar(C4FacetEx &cgo, DynBarFacet &rFacets); // draw gfx bar within element bounds
	void DrawHBarByVGfx(C4FacetEx &cgo, DynBarFacet &rFacets); // draw horizontal gfx bar within element bounds, using gfx of vertical one

	virtual bool IsOwnPtrElement() { return false; } // if true is returned, item will not be deleted when container is cleared
	virtual bool IsExternalDrawDialog() { return false; }
	virtual bool IsMenu() { return false; }

	// for listbox-selection by character input
	virtual bool CheckNameHotkey(const char *c) { return false; }

public:
	virtual Container *GetContainer() { return pParent; } // returns parent for elements; this for containers

	virtual void MouseInput(CMouse &rMouse, int32_t iButton, int32_t iX, int32_t iY, uint32_t dwKeyParam); // input: mouse movement or buttons
	virtual void MouseEnter(CMouse &rMouse) {} // called when mouse cursor enters element region
	virtual void MouseLeave(CMouse &rMouse) {} // called when mouse cursor leaves element region

	virtual void StartDragging(CMouse &rMouse, int32_t iX, int32_t iY, uint32_t dwKeyParam); // called by element in MouseInput: start dragging
	virtual void DoDragging(CMouse &rMouse, int32_t iX, int32_t iY, uint32_t dwKeyParam);    // called by mouse: dragging process
	virtual void StopDragging(CMouse &rMouse, int32_t iX, int32_t iY, uint32_t dwKeyParam);  // called by mouse: mouse released after dragging process

	virtual bool OnHotkey(char cHotkey) { return false; } // return true when hotkey has been processed

public:
	bool DoContext(); // open context menu if assigned

public:
	Element();
	virtual ~Element();

	Container *GetParent() { return pParent; } // get owning container
	virtual class Dialog *GetDlg(); // return contained dialog
	virtual Screen *GetScreen(); // return contained screen
	virtual Control *IsFocusElement() { return nullptr; } // return control to gain focus in search-cycle

	virtual void UpdateOwnPos() {} // called when element bounds were changed externally
	void ScreenPos2ClientPos(int32_t &riX, int32_t &riY); // transform screen coordinates to element coordinates
	void ClientPos2ScreenPos(int32_t &riX, int32_t &riY); // transform element coordinates to screen coordinates

	void SetToolTip(const char *szNewTooltip); // update used tooltip
	const char *GetToolTip(); // return tooltip const char* (own or fallback to parent)

	int32_t GetWidth() { return rcBounds.Wdt; }
	int32_t GetHeight() { return rcBounds.Hgt; }
	C4Rect &GetBounds() { return rcBounds; }
	void SetBounds(const C4Rect &rcNewBound) { rcBounds = rcNewBound; UpdatePos(); UpdateSize(); }
	virtual C4Rect &GetClientRect() { return rcBounds; }
	C4Rect GetContainedClientRect() { C4Rect rc = GetClientRect(); rc.x = rc.y = 0; return rc; }
	Element *GetNext() const { return pNext; }
	Element *GetPrev() const { return pPrev; }
	virtual Element *GetFirstNestedElement(bool fBackwards) { return this; }
	virtual Element *GetFirstContained() { return nullptr; }
	bool IsInActiveDlg(bool fForKeyboard);
	virtual bool IsParentOf(Element *pEl) { return false; } // whether this is the parent container (directly or recursively) of the passed element

	C4Rect GetToprightCornerRect(int32_t iWidth = 16, int32_t iHeight = 16, int32_t iHIndent = 4, int32_t iVIndent = 4, int32_t iIndexX = 0); // get rectangle to be used for context buttons and stuff

	bool IsVisible();
	virtual void SetVisibility(bool fToValue);

	virtual int32_t GetListItemTopSpacing() { return C4GUI_DefaultListSpacing; }
	virtual bool GetListItemTopSpacingBar() { return false; }

	void SetDragTarget(Window *pToWindow) { pDragTarget = pToWindow; }

	void SetContextHandler(ContextHandler *pNewHd) // takes over ptr
	{
		if (pContextHandler) pContextHandler->DeRef();
		if ((pContextHandler = pNewHd)) pNewHd->Ref();
	}

	virtual ContextHandler *GetContextHandler(); // get context handler to be used (own or parent)

	friend class Container; friend class TextWindow; friend class ListBox;
};

// a simple text label on the screen
class Label : public Element
{
protected:
	StdStrBuf sText; // label text
	StdStrBuf sHyperlink; // URL to be visited when clicked
	uint32_t dwFgClr; // text color
	int32_t x0, iAlign; // x-textstart-pos; horizontal alignment
	CStdFont *pFont;
	char cHotkey; // hotkey for this label
	bool fAutosize;
	bool fMarkup;

	Control *pClickFocusControl; // control that gets focus if the label is clicked or hotkey is pressed

	virtual void MouseInput(CMouse &rMouse, int32_t iButton, int32_t iX, int32_t iY, uint32_t dwKeyParam) override; // input: mouse

	virtual void DrawElement(C4FacetEx &cgo) override; // output label
	virtual void UpdateOwnPos() override;

	virtual bool OnHotkey(char cHotkey) override; // focus control on correct hotkey

	virtual int32_t GetLeftIndent() { return 0; }

public:
	Label(std::string_view lblText, int32_t iX0, int32_t iTop, int32_t iAlign = ALeft, uint32_t dwFClr = 0xffffffff, CStdFont *pFont = nullptr, bool fMakeReadableOnBlack = true, bool fMarkup = true);
	Label(std::string_view lblText, const C4Rect &rcBounds, int32_t iAlign = ALeft, uint32_t dwFClr = 0xffffffff, CStdFont *pFont = nullptr, bool fMakeReadableOnBlack = true, bool fAutosize = true, bool fMarkup = true);

	void SetText(const char *szText, bool fAllowHotkey = true); // update text
	void SetText(std::string_view toText, bool fAllowHotkey = true); // update text
	const char *GetText() { return sText.getData(); } // retrieve current text
	void SetClickFocusControl(Control *pToCtrl) { pClickFocusControl = pToCtrl; }
	void SetColor(uint32_t dwToClr, bool fMakeReadableOnBlack = true) { dwFgClr = fMakeReadableOnBlack ? MakeColorReadableOnBlack(dwToClr) : dwToClr; } // update label color
	void SetX0(int32_t iToX0);
	void SetAutosize(bool fToVal) { fAutosize = fToVal; }
	void SetHyperlink(const char *szURL); // make blue and underlined and open this URL when clicked
};

// a label with some wood behind
// used for captions
class WoodenLabel : public Label
{
private:
	time_t tAutoScrollDelay; // if set and text is longer than would fit, the label will automatically start moving if not changed and displayed for a while
	time_t tLastChangeTime; // time when the label text was changed last. 0 if not initialized; set upon first drawing
	int32_t iScrollPos, iScrollDir;
	int32_t iRightIndent;

protected:
	virtual void DrawElement(C4FacetEx &cgo) override; // output label

	C4Facet fctIcon; // icon shown at left-side of label; if set, text is aligned to left

	virtual int32_t GetLeftIndent() override { return fctIcon.Surface ? rcBounds.Hgt : 0; }
	int32_t GetRightIndent() const { return iRightIndent; }

public:
	WoodenLabel(const char *szLblText, const C4Rect &rcBounds, uint32_t dwFClr = 0xffffffff, CStdFont *pFont = nullptr, int32_t iAlign = ACenter, bool fMarkup = true)
		: Label(szLblText, rcBounds, iAlign, dwFClr, pFont, true, true, fMarkup), tAutoScrollDelay(0), tLastChangeTime(0), iScrollPos(0), iScrollDir(0), iRightIndent(0)
	{
		SetAutosize(false); this->rcBounds = rcBounds;
	} // re-sets bounds after SetText

	static int32_t GetDefaultHeight(CStdFont *pUseFont = nullptr);

	void SetIcon(const C4Facet &rfctIcon);
	void SetAutoScrollTime(time_t tDelay) { tAutoScrollDelay = tDelay; ResetAutoScroll(); }
	void ResetAutoScroll() { tLastChangeTime = 0; iScrollPos = iScrollDir = 0; }
	void SetRightIndent(int32_t iNewIndent) { iRightIndent = iNewIndent; }
};

// a multiline label with automated text clipping
// used for display of log buffers
class MultilineLabel : public Element
{
protected:
	C4LogBuffer Lines;
	bool fMarkup;

protected:
	virtual void DrawElement(C4FacetEx &cgo) override; // output label
	void UpdateHeight();
	virtual void UpdateSize() override; // update label height

public:
	MultilineLabel(const C4Rect &rcBounds, int32_t iMaxLines, int32_t iMaxBuf, const char *szIndentChars, bool fAutoGrow, bool fMarkup);

	void AddLine(const char *szLine, CStdFont *pFont, uint32_t dwClr, bool fDoUpdate, bool fMakeReadableOnBlack, CStdFont *pCaptionFont); // add line of text
	void Clear(bool fDoUpdate); // clear lines

	friend class TextWindow;
};

// a bar that show progress
class ProgressBar : public Element
{
protected:
	int32_t iProgress, iMax;

	virtual void DrawElement(C4FacetEx &cgo) override; // draw progress bar

public:
	ProgressBar(C4Rect &rrcBounds, int32_t iMaxProgress = 100)
		: Element(), iProgress(0), iMax(iMaxProgress)
	{
		rcBounds = rrcBounds; UpdatePos();
	}

	void SetProgress(int32_t iToProgress) { iProgress = iToProgress; }
};

// Auxiliary design gfx: a horizontal line
class HorizontalLine : public Element
{
protected:
	uint32_t dwClr, dwShadowClr;

	virtual void DrawElement(C4FacetEx &cgo) override; // draw horizontal line

public:
	HorizontalLine(C4Rect &rrcBounds, uint32_t dwClr = 0x000000, uint32_t dwShadowClr = 0xaf7f7f7f)
		: Element(), dwClr(dwClr), dwShadowClr(dwShadowClr)
	{
		SetBounds(rrcBounds);
	}
};

// picture displaying a FacetEx
class Picture : public Element
{
protected:
	C4FacetExSurface Facet; // picture facet
	bool fAspect; // preserve width/height-ratio when drawing
	bool fCustomDrawClr; // custom drawing color for clrbyowner-surfaces
	uint32_t dwDrawClr;
	bool fAnimate; // if true, the picture is animated. Whoaa!
	int32_t iPhaseTime, iAnimationPhase, iDelay; // current animation phase - undefined if not animated

	virtual void DrawElement(C4FacetEx &cgo) override; // draw the image

public:
	Picture(const C4Rect &rcBounds, bool fAspect); // does not load image

	const C4FacetExSurface &GetFacet() const { return Facet; } // get picture facet
	C4FacetExSurface &GetMFacet() { return Facet; } // get picture facet
	void SetFacet(const C4Facet &fct) { Facet.Clear(); Facet.Set(fct); }
	bool EnsureOwnSurface(); // create an own surface, if it's just a link
	void SetDrawColor(uint32_t dwToClr) { dwDrawClr = dwToClr; fCustomDrawClr = true; }
	void SetAnimated(bool fEnabled, int iDelay); // starts/stops cycling through all phases of the specified facet
};

// picture displaying two facets
class OverlayPicture : public Picture
{
protected:
	int iBorderSize; // border of overlay image if not zoomed
	C4Facet OverlayImage; // image to be displayed on top of the actual picture

	virtual void DrawElement(C4FacetEx &cgo) override; // draw the image

public:
	OverlayPicture(const C4Rect &rcBounds, bool fAspect, const C4Facet &rOverlayImage, int iBorderSize); // does not load image
};

// icon indices
enum Icons
{
	Ico_Empty = -2, // for context menus only
	Ico_None = -1,
	Ico_Clonk = 0,
	Ico_Notify = 1,
	Ico_Wait = 2,
	Ico_NetWait = 3,
	Ico_Host = 4,
	Ico_Client = 5,
	Ico_UnknownClient = 6,
	Ico_UnknownPlayer = 7,
	Ico_ObserverClient = 8,
	Ico_Player = 9,
	Ico_Resource = 10,
	Ico_Error = 11,
	Ico_SavegamePlayer = 12,
	Ico_Save = 13,
	Ico_Active = 14,
	Ico_Options = 14,
	Ico_Inactive = 15,
	Ico_Kick = 16,
	Ico_Loading = 17,
	Ico_Confirm = 18,
	Ico_Team = 19,
	Ico_AddPlr = 20,
	Ico_Record = 21,
	Ico_Chart = 21,
	Ico_Gfx = 22,
	Ico_Sound = 23,
	Ico_Keyboard = 24,
	Ico_Gamepad = 25,
	Ico_MouseOff = 26,
	Ico_MouseOn = 27,
	Ico_Help = 28,
	Ico_Definition = 29,
	Ico_GameRunning = 30,
	Ico_Lobby = 31,
	Ico_RuntimeJoin = 32,
	Ico_Exit = 33,
	Ico_Close = 34,
	Ico_Rank1 = 35,
	Ico_Rank2 = 36,
	Ico_Rank3 = 37,
	Ico_Rank4 = 38,
	Ico_Rank5 = 39,
	Ico_Rank6 = 40,
	Ico_Rank7 = 41,
	Ico_Rank8 = 42,
	Ico_Rank9 = 43,
	Ico_OfficialServer = 44,
	Ico_Surrender = 45,
	Ico_MeleeLeague = 46,
	Ico_Ready = 47,
	Ico_Star = 48,
	Ico_Disconnect = 49,
	Ico_View = 50,
	Ico_RegJoinOnly = 51,
	Ico_NoSound = 52,

	Ico_Extended = 0x100, // icon index offset for extended icons

	Ico_Ex_RecordOff      = Ico_Extended + 0,
	Ico_Ex_RecordOn       = Ico_Extended + 1,
	Ico_Ex_FairCrew       = Ico_Extended + 2,
	Ico_Ex_NormalCrew     = Ico_Extended + 3,
	Ico_Ex_LeagueOff      = Ico_Extended + 4,
	Ico_Ex_LeagueOn       = Ico_Extended + 5,
	Ico_Ex_InternetOff    = Ico_Extended + 6,
	Ico_Ex_InternetOn     = Ico_Extended + 7,
	Ico_Ex_League         = Ico_Extended + 8,
	Ico_Ex_FairCrewGray   = Ico_Extended + 9,
	Ico_Ex_NormalCrewGray = Ico_Extended + 10,
	Ico_Ex_Locked         = Ico_Extended + 11,
	Ico_Ex_Unlocked       = Ico_Extended + 12,
	Ico_Ex_LockedFrontal  = Ico_Extended + 13,
	Ico_Ex_Update         = Ico_Extended + 14,
	Ico_Ex_Chat           = Ico_Extended + 15,
	Ico_Ex_GameList       = Ico_Extended + 16,
	Ico_Ex_Comment        = Ico_Extended + 17,
};

// cute, litte, useless thingy
class Icon : public Picture
{
public:
	Icon(const C4Rect &rcBounds, Icons icoIconIndex);

	void SetIcon(Icons icoNewIconIndex);
	static C4FacetEx GetIconFacet(Icons icoIconIndex);
};

// a collection of gui-elements
class Container : public Element
{
protected:
	Element *pFirst, *pLast; // contained elements

	virtual void Draw(C4FacetEx &cgo) override; // draw all elements
	virtual void ElementSizeChanged(Element *pOfElement) {} // called when an element size is changed
	virtual void ElementPosChanged(Element *pOfElement) {}  // called when an element position is changed

	virtual void AfterElementRemoval()
	{
		if (pParent) pParent->AfterElementRemoval();
	} // called by ScrollWindow to parent after an element has been removed

	virtual bool OnHotkey(char cHotkey) override; // check all contained elements for hotkey

public:
	Container();
	~Container();

	void Clear(); // delete all child elements
	void ClearChildren(); // delete all child elements
	virtual void RemoveElement(Element *pChild) override; // remove child element from container
	void MakeLastElement(Element *pChild); // resort to the end of the list
	void AddElement(Element *pChild); // add child element to container
	void ReaddElement(Element *pChild); // resort child element to end of list
	void InsertElement(Element *pChild, Element *pInsertBefore); // add child element to container, ordered directly before given, other element
	Element *GetNextNestedElement(Element *pPrevElement, bool fBackwards); // get next element after given, applying recursion
	virtual Element *GetFirstContained() override { return pFirst; }
	virtual Element *GetLastContained() { return pLast; }
	virtual Element *GetFirstNestedElement(bool fBackwards) override;
	Element *GetFirst() { return pFirst; }
	Element *GetLast() { return pLast; }
	virtual Container *GetContainer() override { return this; } // returns parent for elements; this for containers
	Element *GetElementByIndex(int32_t i); // get indexed child element
	int32_t GetElementCount();
	virtual void SetVisibility(bool fToValue) override;

	virtual bool IsFocused(Control *pCtrl) { return pParent ? pParent->IsFocused(pCtrl) : false; }
	virtual bool IsSelectedChild(Element *pChild) { return pParent ? pParent->IsSelectedChild(pChild) : true; } // whether the child element is selected - only false for list-box-containers which can have unselected children
	virtual bool IsParentOf(Element *pEl) override; // whether this is the parent container (directly or recursively) of the passed element

	virtual void ApplyElementOffset(int32_t &riX, int32_t &riY) {} // apply child drawing offset
	virtual void ApplyInvElementOffset(int32_t &riX, int32_t &riY) {} // subtract child drawing offset

	friend class Element; friend class ScrollWindow;
};

// a rectangled control that contains other elements
class Window : public Container
{
protected:
	C4Rect rcClientRect; // area for contained elements

	virtual void MouseInput(CMouse &rMouse, int32_t iButton, int32_t iX, int32_t iY, uint32_t dwKeyParam) override; // input: mouse movement or buttons
	virtual void Draw(C4FacetEx &cgo) override; // draw this window

public:
	Window();

	void SetPos(int32_t iXPos, int32_t iYPos)
	{
		rcBounds.x = iXPos; rcBounds.y = iYPos; UpdatePos();
	}

	virtual void UpdateOwnPos() override; // update client rect
	virtual C4Rect &GetClientRect() override { return rcClientRect; }

	virtual void ApplyElementOffset(int32_t &riX, int32_t &riY) override
	{
		riX -= rcClientRect.x; riY -= rcClientRect.y;
	}

	virtual void ApplyInvElementOffset(int32_t &riX, int32_t &riY) override
	{
		riX += rcClientRect.x; riY += rcClientRect.y;
	}

	virtual bool IsComponentOutsideClientArea() { return false; } // if set, drawing routine of subcomponents will clip to Bounds rather than to ClientRect

	// get margins from bounds to client rect
	virtual int32_t GetMarginTop()    { return 0; }
	virtual int32_t GetMarginLeft()   { return 0; }
	virtual int32_t GetMarginRight()  { return 0; }
	virtual int32_t GetMarginBottom() { return 0; }
};

// a scroll bar
class ScrollBar : public Element
{
protected:
	bool fScrolling; // if set, scrolling is currently enabled
	bool fAutoHide; // if set, bar is made invisible if scrolling is not possible anyway
	int32_t iScrollThumbSize; // height(/width) of scroll thumb
	int32_t iScrollPos; // y(/x) offset of scroll thumb
	bool fTopDown, fBottomDown; // whether scrolling buttons are pressed
	bool fHorizontal; // if set, the scroll bar is horizontal instead of vertical
	int32_t iCBMaxRange; // range for callback class

	ScrollWindow *pScrollWindow; // associated scrolled window - may be 0 for callback scrollbars
	BaseParCallbackHandler<int32_t> *pScrollCallback; // callback called when scroll pos changes

	ScrollBarFacets *pCustomGfx;

	void Update();       // update scroll bar according to window
	void OnPosChanged(); // update window according to scroll bar, and/or do callbacks

	// mouse handling
	virtual void MouseInput(CMouse &rMouse, int32_t iButton, int32_t iX, int32_t iY, uint32_t dwKeyParam) override; // input: mouse movement or buttons
	virtual void DoDragging(CMouse &rMouse, int32_t iX, int32_t iY, uint32_t dwKeyParam) override; // dragging: allow dragging of thumb
	virtual void MouseLeave(CMouse &rMouse) override; // mouse leaves with button down: reset down state of buttons

	virtual void DrawElement(C4FacetEx &cgo) override; // draw scroll bar

	// suppress scrolling pin for very narrow menus
	bool HasPin()
	{
		if (fHorizontal) return rcBounds.Wdt > (2 * C4GUI_ScrollArrowWdt + C4GUI_ScrollThumbWdt);
		else             return rcBounds.Hgt > (2 * C4GUI_ScrollArrowHgt + C4GUI_ScrollThumbHgt);
	}

	int32_t GetMaxScroll()
	{
		if (fHorizontal) return HasPin() ? GetBounds().Wdt - 2 * C4GUI_ScrollArrowWdt - iScrollThumbSize : 100;
		else             return HasPin() ? GetBounds().Hgt - 2 * C4GUI_ScrollArrowHgt - iScrollThumbSize : 100;
	}

	int32_t GetScrollByPos(int32_t iX, int32_t iY)
	{
		return BoundBy<int32_t>((fHorizontal ? iX - C4GUI_ScrollArrowWdt : iY - C4GUI_ScrollArrowHgt) - iScrollThumbSize / 2, 0, GetMaxScroll());
	}

	bool IsScrolling() { return fScrolling; }

public:
	ScrollBar(C4Rect &rcBounds, ScrollWindow *pWin); // ctor for scroll window
	ScrollBar(C4Rect &rcBounds, bool fHorizontal, BaseParCallbackHandler<int32_t> *pCB, int32_t iCBMaxRange = 256); // ctor for callback
	~ScrollBar();

	// change style
	void SetDecoration(ScrollBarFacets *pToGfx, bool fAutoHide)
	{
		pCustomGfx = pToGfx; this->fAutoHide = fAutoHide;
	}

	// change scroll pos in a [0, iCBMaxRange-1] scale
	void SetScrollPos(int32_t iToPos) { iScrollPos = iToPos * GetMaxScroll() / (iCBMaxRange - 1); }

	friend class ScrollWindow;
};

// a window that can be scrolled
class ScrollWindow : public Window
{
protected:
	ScrollBar *pScrollBar; // vertical scroll bar associated with the window
	int32_t iScrollY; // vertical scroll pos
	int32_t iClientHeight; // client rect height
	bool fHasBar;
	int32_t iFrozen; // if >0, no scrolling updates are done (used during window refill)

	// pass element updates through to parent window
	virtual void ElementSizeChanged(Element *pOfElement) override // called when an element size is changed
	{
		Window::ElementSizeChanged(pOfElement);
		if (pParent) pParent->ElementSizeChanged(pOfElement);
	}

	virtual void ElementPosChanged(Element *pOfElement) override // called when an element position is changed
	{
		Window::ElementPosChanged(pOfElement);
		if (pParent) pParent->ElementPosChanged(pOfElement);
	}

	virtual void MouseInput(CMouse &rMouse, int32_t iButton, int32_t iX, int32_t iY, uint32_t dwKeyParam) override;

public:
	ScrollWindow(Window *pParentWindow); // create scroll window in client area of window
	~ScrollWindow() { if (pScrollBar) pScrollBar->pScrollWindow = nullptr; }

	virtual bool IsComponentOutsideClientArea() override { return true; } // always clip drawing routine of subcomponents to Bounds
	void Update(); // update client rect and scroll bar according to window
	virtual void UpdateOwnPos() override;
	void Freeze() { ++iFrozen; }
	void UnFreeze() { if (!--iFrozen) Update(); }
	bool IsFrozen() const { return !!iFrozen; }

	void SetClientHeight(int32_t iToHgt) // set new client height
	{
		iClientHeight = iToHgt; Update();
	}

	// change style
	void SetDecoration(ScrollBarFacets *pToGfx, bool fAutoScroll)
	{
		if (pScrollBar) pScrollBar->SetDecoration(pToGfx, fAutoScroll);
	}

	void SetScroll(int32_t iToScroll); // sets new scrolling; does not update scroll bar
	void ScrollToBottom(); // set scrolling to bottom range; updates scroll bar
	void ScrollPages(int iPageCount); // scroll down by multiples of visible height; updates scroll bar
	void ScrollBy(int iAmount); // scroll down by vertical pixel amount; updates scroll bar
	void ScrollRangeInView(int32_t iY, int32_t iHgt); // sets scrolling so range is in view; updates scroll bar
	bool IsRangeInView(int32_t iY, int32_t iHgt); // returns whether scrolling range is in view

	int32_t GetScrollY() { return iScrollY; }

	void SetScrollBarEnabled(bool fToVal);
	bool IsScrollBarEnabled() { return fHasBar; }

	bool IsScrollingActive() { return fHasBar && pScrollBar && pScrollBar->IsScrolling(); }
	bool IsScrollingNecessary() { return iClientHeight > rcBounds.Hgt; }

	friend class ScrollBar;
};

// a collection of components
class GroupBox : public Window
{
private:
	StdStrBuf sTitle;
	CStdFont *pFont;
	uint32_t dwFrameClr, dwTitleClr, dwBackClr;
	int32_t iMargin;

	CStdFont *GetTitleFont() const;

public:
	GroupBox(const C4Rect &rtBounds) : Window(), pFont(nullptr), dwFrameClr(0u), dwTitleClr(C4GUI_CaptionFontClr), dwBackClr(0xffffffff), iMargin(4)
	{
		// init client rect
		SetBounds(rtBounds);
	}

	void SetFont(CStdFont *pToFont) { pFont = pToFont; }
	void SetColors(uint32_t dwFrameClr, uint32_t dwTitleClr, uint32_t dwBackClr = 0xffffffff) { this->dwFrameClr = dwFrameClr; this->dwTitleClr = dwTitleClr; this->dwBackClr = dwBackClr; }
	void SetTitle(const char *szToTitle) { if (szToTitle && *szToTitle) sTitle.Copy(szToTitle); else sTitle.Clear(); UpdateOwnPos(); }
	void SetMargin(int32_t iNewMargin) { iMargin = iNewMargin; UpdateOwnPos(); }

	bool HasTitle() const { return !!sTitle.getLength(); }

	virtual void DrawElement(C4FacetEx &cgo) override; // draw frame

	virtual int32_t GetMarginTop() override    { return HasTitle() ? iMargin + GetTitleFont()->GetLineHeight() : iMargin; }
	virtual int32_t GetMarginLeft() override   { return iMargin; }
	virtual int32_t GetMarginRight() override  { return iMargin; }
	virtual int32_t GetMarginBottom() override { return iMargin; }
};

// a control that may have focus
class Control : public Window
{
private:
	class C4KeyBinding *pKeyContext;

protected:
	virtual bool CharIn(const char *c) { return false; } // input: character key pressed - should return false for none-character-inputs
	virtual void MouseInput(CMouse &rMouse, int32_t iButton, int32_t iX, int32_t iY, uint32_t dwKeyParam) override; // input: mouse. left-click sets focus

	void DisableFocus(); // called when control gets disabled: Make sure it loses focus
	virtual bool IsFocusOnClick() { return true; } // defaultly, controls get focused on left-down
	virtual Control *IsFocusElement() override { return this; } // this control can gain focus
	virtual void OnGetFocus(bool fByMouse) {} // callback when control gains focus
	virtual void OnLooseFocus() {} // callback when control looses focus

	bool KeyContext() { return DoContext(); }

public:
	Control(const C4Rect &rtBounds);
	~Control();

	bool HasFocus() { return pParent && pParent->IsFocused(this); }
	bool HasDrawFocus();

	friend class Dialog; friend class ListBox;
};

// generic callback-functions to the dialog
template <class CallbackDlg> class DlgCallback
{
public:
	typedef void (CallbackDlg::*Func)(Control *pFromControl);
	typedef ContextMenu *(CallbackDlg::*ContextFunc)(Element *pFromElement, int32_t iX, int32_t iY);
	typedef void (CallbackDlg::*ContextClickFunc)(Element *pTargetElement);
};

// multi-param callback-functions to the dialog
template <class CallbackDlg, class TEx> class DlgCallbackEx
{
public:
	typedef void (CallbackDlg::*ContextClickFunc)(Element *pTargetElement, TEx Extra);
};

// a button. may be pressed.
class Button : public Control
{
private:
	class C4KeyBinding *pKeyButton;
	DynBarFacet *pCustomGfx, *pCustomGfxDown;

protected:
	StdStrBuf sText; // button label
	bool fDown; // if set, button is currently held down
	bool fMouseOver; // if set, the mouse hovers over the button
	char cHotkey; // hotkey for this button
	bool fEnabled;

	virtual void MouseInput(CMouse &rMouse, int32_t iButton, int32_t iX, int32_t iY, uint32_t dwKeyParam) override; // input: mouse movement or buttons
	virtual void MouseEnter(CMouse &rMouse) override; // mouse re-enters with button down: set button down
	virtual void MouseLeave(CMouse &rMouse) override; // mouse leaves with button down: reset down state
	virtual bool IsFocusOnClick() override { return false; } // buttons don't get focus on click (for easier input, e.g. in chatbox)

	virtual void DrawElement(C4FacetEx &cgo) override; // draw dlg bg

	virtual void OnPress(); // called when button is pressed

	bool KeyButtonDown();
	bool KeyButtonUp();
	void SetDown(); // mark down and play sound
	void SetUp(bool fPress); // mark up and play sound

	virtual bool OnHotkey(char cHotkey) override; // press btn on correct hotkey

public:
	Button(const char *szBtnText, const C4Rect &rtBounds);
	~Button();

	void SetText(const char *szToText); // update button text (and hotkey)

	void SetCustomGraphics(DynBarFacet *pCustomGfx, DynBarFacet *pCustomGfxDown)
	{
		this->pCustomGfx = pCustomGfx; this->pCustomGfxDown = pCustomGfxDown;
	}

	void SetEnabled(bool fToVal) { fEnabled = fToVal; if (!fEnabled) fDown = false; }
};

// button using icon image
class IconButton : public Button
{
protected:
	C4Facet fctIcon;
	uint32_t dwClr;
	bool fHasClr;
	bool fHighlight; // if true, the button is highlighted permanently

	virtual void DrawElement(C4FacetEx &cgo) override; // draw icon and highlight if necessary

public:
	IconButton(Icons eUseIcon, const C4Rect &rtBounds, char cHotkey);
	void SetIcon(Icons eUseIcon);
	void SetFacet(const C4Facet &rCpy, uint32_t dwClr = 0u) { fctIcon = rCpy; }
	void SetColor(uint32_t dwClr) { fHasClr = true; this->dwClr = dwClr; }
	void SetHighlight(bool fToVal) { fHighlight = fToVal; }
};

// button using arrow image
class ArrowButton : public Button
{
public:
	enum ArrowFct { Left = 0, Right = 1, Down = 2 };

protected:
	ArrowFct eDir;

	virtual void DrawElement(C4FacetEx &cgo) override; // draw arrow; highlight and down if necessary

public:
	ArrowButton(ArrowFct eDir, const C4Rect &rtBounds, char cHotkey = 0);

	static int32_t GetDefaultWidth();
	static int32_t GetDefaultHeight();
};

// button using facets for highlight
class FacetButton : public Button
{
protected:
	C4Facet fctBase, fctHighlight;
	uint32_t dwTextClrInact, dwTextClrAct; // text colors for inactive/active button
	FLOAT_RECT rcfDrawBounds; // drawing bounds

	// title drawing parameters
	int32_t iTxtOffX, iTxtOffY;
	uint8_t byTxtAlign; // ALeft, ACenter or ARight
	CStdFont *pFont; float fFontZoom;

	virtual void DrawElement(C4FacetEx &cgo) override; // draw base facet or highlight facet if necessary

public:
	FacetButton(const C4Facet &rBaseFct, const C4Facet &rHighlightFct, const FLOAT_RECT &rtfBounds, char cHotkey);

	void SetTextColors(uint32_t dwClrInact, uint32_t dwClrAct)
	{
		dwTextClrInact = dwClrInact; dwTextClrAct = dwClrAct;
	}

	void SetTextPos(int32_t iOffX, int32_t iOffY, uint8_t byAlign = ACenter)
	{
		iTxtOffX = iOffX; iTxtOffY = iOffY; byTxtAlign = byAlign;
	}

	void SetTextFont(CStdFont *pFont, float fFontZoom = 1.0f)
	{
		this->pFont = pFont; this->fFontZoom = fFontZoom;
	}
};

// a button doing some callback...
template <class CallbackDlg, class Base = Button> class CallbackButton : public Base
{
protected:
	CallbackDlg *pCB;

	typename DlgCallback<CallbackDlg>::Func pCallbackFn; // callback function

	virtual void OnPress() override
	{
		if (pCallbackFn)
		{
			CallbackDlg *pC = pCB;
			if (!pC) if (!(pC = reinterpret_cast<CallbackDlg *>(Base::GetDlg()))) return;
			(pC->*pCallbackFn)(this);
		}
	}

public:
	CallbackButton(ArrowButton::ArrowFct eDir, const C4Rect &rtBounds, typename DlgCallback<CallbackDlg>::Func pFn, CallbackDlg *pCB = nullptr)
		: Base(eDir, rtBounds, 0), pCallbackFn(pFn), pCB(pCB) {}
	CallbackButton(const char *szBtnText, const C4Rect &rtBounds, typename DlgCallback<CallbackDlg>::Func pFn, CallbackDlg *pCB = nullptr)
		: Base(szBtnText, rtBounds), pCallbackFn(pFn), pCB(pCB) {}
	CallbackButton(Icons eUseIcon, const C4Rect &rtBounds, char cHotkey, typename DlgCallback<CallbackDlg>::Func pFn, CallbackDlg *pCB = nullptr)
		: Base(eUseIcon, rtBounds, cHotkey), pCallbackFn(pFn), pCB(pCB) {}
	CallbackButton(int32_t iID, const C4Rect &rtBounds, char cHotkey, typename DlgCallback<CallbackDlg>::Func pFn, CallbackDlg *pCB = nullptr)
		: Base(iID, rtBounds, cHotkey), pCallbackFn(pFn), pCB(pCB) {}
};

// a button doing some callback to any class
template <class CallbackDlg, class Base = Button> class CallbackButtonEx : public Base
{
protected:
	typedef CallbackDlg *CallbackDlgPointer;
	CallbackDlgPointer pCBTarget; // callback target
	typename DlgCallback<CallbackDlg>::Func pCallbackFn; // callback function

	virtual void OnPress() override
	{
		(pCBTarget->*pCallbackFn)(this);
	}

public:
	CallbackButtonEx(const char *szBtnText, const C4Rect &rtBounds, CallbackDlgPointer pCBTarget, typename DlgCallback<CallbackDlg>::Func pFn)
		: Base(szBtnText, rtBounds), pCBTarget(pCBTarget), pCallbackFn(pFn) {}
	CallbackButtonEx(Icons eUseIcon, const C4Rect &rtBounds, char cHotkey, CallbackDlgPointer pCBTarget, typename DlgCallback<CallbackDlg>::Func pFn)
		: Base(eUseIcon, rtBounds, cHotkey), pCBTarget(pCBTarget), pCallbackFn(pFn) {}
	CallbackButtonEx(const C4Facet &fctBase, const C4Facet &fctHighlight, const FLOAT_RECT &rtfBounds, char cHotkey, CallbackDlgPointer pCBTarget, typename DlgCallback<CallbackDlg>::Func pFn)
		: Base(fctBase, fctHighlight, rtfBounds, cHotkey), pCBTarget(pCBTarget), pCallbackFn(pFn) {}
};

// checkbox with a text label right of it
class CheckBox : public Control
{
private:
	bool fChecked;
	class C4KeyBinding *pKeyCheck;
	StdStrBuf sCaption;
	bool fMouseOn;
	BaseCallbackHandler *pCBHandler; // callback handler called if check state changes
	bool fEnabled;
	CStdFont *pFont;
	uint32_t dwEnabledClr, dwDisabledClr;
	char cHotkey;

public:
	CheckBox(const C4Rect &rtBounds, const std::string &caption, bool fChecked);
	~CheckBox();

private:
	bool KeyCheck() { ToggleCheck(true); return true; }

public:
	void ToggleCheck(bool fByUser); // check on/off; do callback

protected:
	virtual void UpdateOwnPos() override;
	virtual void MouseInput(CMouse &rMouse, int32_t iButton, int32_t iX, int32_t iY, uint32_t dwKeyParam) override; // input: mouse movement or buttons
	virtual void MouseEnter(CMouse &rMouse) override;
	virtual void MouseLeave(CMouse &rMouse) override;
	virtual bool IsFocusOnClick() override { return false; } // just check/uncheck on click; do not gain keyboard focus as well
	virtual Control *IsFocusElement() override { return fEnabled ? Control::IsFocusElement() : nullptr; } // this control can gain focus if enabled
	virtual void DrawElement(C4FacetEx &cgo) override; // draw checkbox
	virtual bool OnHotkey(char cHotkey) override; // return true when hotkey has been processed

public:
	void SetChecked(bool fToVal) { fChecked = fToVal; } // set w/o callback
	bool GetChecked() const { return fChecked; }
	void SetOnChecked(BaseCallbackHandler *pCB);
	void SetEnabled(bool fToVal) { if (!(fEnabled = fToVal)) DisableFocus(); }
	void SetCaption(const std::string &caption);

	const char *GetText() { return sCaption.getData(); }

	void SetFont(CStdFont *pFont, uint32_t dwEnabledClr, uint32_t dwDisabledClr)
	{
		this->pFont = pFont; this->dwEnabledClr = dwEnabledClr; this->dwDisabledClr = dwDisabledClr;
	}

	static bool GetStandardCheckBoxSize(int *piWdt, int *piHgt, const char *szForCaptionText, CStdFont *pUseFont); // get needed size to construct a checkbox
};

// scrollable text box
class TextWindow : public Control
{
protected:
	ScrollWindow *pClientWindow; // client scrolling window
	Picture *pTitlePicture; // [optional]: Picture shown atop the text
	MultilineLabel *pLogBuffer; // buffer holding text data
	bool fDrawBackground, fDrawFrame; // whether dark background should be drawn (default true)
	size_t iPicPadding;

	virtual void DrawElement(C4FacetEx &cgo) override; // draw text window

	virtual void ElementSizeChanged(Element *pOfElement) override; // called when an element size is changed
	virtual void ElementPosChanged(Element *pOfElement) override;  // called when an element position is changed
	virtual void UpdateSize() override;

	virtual Control *IsFocusElement() override { return nullptr; } // no focus element for now, because there's nothing to do (2do: scroll?)

public:
	TextWindow(const C4Rect &rtBounds, size_t iPicWdt = 0, size_t iPicHgt = 0, size_t iPicPadding = 0, size_t iMaxLines = 100, size_t iMaxTextLen = 4096, const char *szIndentChars = "    ", bool fAutoGrow = false, const C4Facet *pOverlayPic = nullptr, int iOverlayBorder = 0, bool fMarkup = false);

	void AddTextLine(const char *szText, CStdFont *pFont, uint32_t dwClr, bool fDoUpdate, bool fMakeReadableOnBlack, CStdFont *pCaptionFont = nullptr) // add text in a new line
	{
		if (pLogBuffer) pLogBuffer->AddLine(szText, pFont, dwClr, fDoUpdate, fMakeReadableOnBlack, pCaptionFont);
	}

	void ScrollToBottom() // set scrolling to bottom range
	{
		if (pClientWindow) pClientWindow->ScrollToBottom();
	}

	void ClearText(bool fDoUpdate)
	{
		if (pLogBuffer) pLogBuffer->Clear(fDoUpdate);
	}

	int32_t GetScrollPos()
	{
		return pClientWindow ? pClientWindow->GetScrollY() : 0;
	}

	void SetScrollPos(int32_t iPos)
	{
		if (pClientWindow) pClientWindow->ScrollRangeInView(iPos, 0);
	}

	void UpdateHeight()
	{
		if (pLogBuffer) pLogBuffer->UpdateHeight();
	}

	void SetDecoration(bool fDrawBG, bool fDrawFrame, ScrollBarFacets *pToGfx, bool fAutoScroll)
	{
		fDrawBackground = fDrawBG; this->fDrawFrame = fDrawFrame; if (pClientWindow) pClientWindow->SetDecoration(pToGfx, fAutoScroll);
	}

	void SetPicture(const C4Facet &rNewPic);

	// get margins from bounds to client rect
	virtual int32_t GetMarginTop() override    { return 8; }
	virtual int32_t GetMarginLeft() override   { return 10; }
	virtual int32_t GetMarginRight() override  { return 5; }
	virtual int32_t GetMarginBottom() override { return 8; }
};

// context menu opened on right-click
class ContextMenu : public Window
{
private:
	// key bindings
	class C4KeyBinding *pKeySelUp, *pKeySelDown, *pKeySubmenu, *pKeyBack, *pKeyAbort, *pKeyConfirm, *pKeyHotkey;

	bool KeySelUp();
	bool KeySelDown();
	bool KeySubmenu();
	bool KeyBack();
	bool KeyAbort();
	bool KeyConfirm();
	bool KeyHotkey(C4KeyCodeEx key);

private:
	static int32_t iGlobalMenuIndex;
	int32_t iMenuIndex;

	void UpdateElementPositions(); // stack all menu elements

	// element adding made private: May only add entries
	bool AddElement(Element *pChild);

public:
	// one text entry (icon+text+eventually right-arrow)
	class Entry : public Element
	{
	private:
		int32_t GetIconIndent() { return (icoIcon == -1) ? 0 : rcBounds.Hgt + 2; }

	protected:
		StdStrBuf sText; // entry text
		char cHotkey; // entry hotkey
		Icons icoIcon; // icon to be drawn left of text
		MenuHandler *pMenuHandler; // callback when item is selected
		ContextHandler *pSubmenuHandler; // callback when submenu is opened

	protected:
		virtual void DrawElement(C4FacetEx &cgo) override; // draw element

		MenuHandler *GetAndZeroCallback()
		{
			MenuHandler *pMH = pMenuHandler; pMenuHandler = nullptr; return pMH;
		}

		virtual void MouseLeave(CMouse &rMouse) override
		{
			if (GetParent()) static_cast<ContextMenu *>(GetParent())->MouseLeaveEntry(rMouse, this);
		}

		virtual bool OnHotkey(char cKey) override { return cKey == cHotkey; }

		virtual bool IsMenu() override { return true; }

	public:
		Entry(const char *szText, Icons icoIcon = Ico_None, MenuHandler *pMenuHandler = nullptr, ContextHandler *pSubmenuHandler = nullptr);
		~Entry()
		{
			delete pMenuHandler;
			delete pSubmenuHandler;
		}

		const char *GetText() { return sText.getData(); }

		friend class ContextMenu;
	};

protected:
	Element *pTarget; // target element; close menu if this is lost
	Element *pSelectedItem; // currently highlighted menu item

	ContextMenu *pSubmenu; // currently open submenu

	// mouse movement or buttons forwarded from screen:
	// Check bounds and forward as MouseInput to context menu or submenus
	// return whether inside context menu bounds
	bool CtxMouseInput(CMouse &rMouse, int32_t iButton, int32_t iScreenX, int32_t iScreenY, uint32_t dwKeyParam);
	virtual void RemoveElement(Element *pChild) override; // clear ptr - abort menu if target is destroyed

	virtual bool CharIn(const char *c); // input: character key pressed - should return false for none-character-inputs
	virtual void MouseInput(CMouse &rMouse, int32_t iButton, int32_t iX, int32_t iY, uint32_t dwKeyParam) override; // input: mouse movement or buttons
	void MouseLeaveEntry(CMouse &rMouse, Entry *pOldEntry); // callback: mouse leaves - deselect menu item if no submenu is open (callback done by menu item)

	virtual void DrawElement(C4FacetEx &cgo) override; // draw BG
	virtual void Draw(C4FacetEx &cgo) override; // draw inherited (element+contents) plus submenu

	virtual bool IsMenu() override { return true; }

	virtual Screen *GetScreen() override; // return screen by static var

	virtual int32_t GetMarginTop() override    { return 5; }
	virtual int32_t GetMarginLeft() override   { return 5; }
	virtual int32_t GetMarginRight() override  { return 5; }
	virtual int32_t GetMarginBottom() override { return 5; }

	void SelectionChanged(bool fByUser); // other item selected: Close any submenu; do callbacks

	virtual void ElementSizeChanged(Element *pOfElement) override; // called when an element size is changed
	virtual void ElementPosChanged(Element *pOfElement) override;  // called when an element position is changed

	void CheckOpenSubmenu(); // open submenu if present for selected item
	bool IsSubmenu(); // return whether it's a submenu
	void DoOK(); // do OK for selected item

public:
	ContextMenu();
	~ContextMenu();

	void Open(Element *pTarget, int32_t iScreenX, int32_t iScreenY);
	void Abort(bool fByUser);

	void AddItem(const char *szText, const char *szToolTip = nullptr, Icons icoIcon = Ico_None, MenuHandler *pMenuHandler = nullptr, ContextHandler *pSubmenuHandler = nullptr)
	{
		Element *pNew = new ContextMenu::Entry(szText, icoIcon, pMenuHandler, pSubmenuHandler);
		AddElement(pNew); pNew->SetToolTip(szToolTip);
	}

	Entry *GetIndexedEntry(int32_t iIndex) { return static_cast<Entry *>(GetElementByIndex(iIndex)); }

	int32_t GetMenuIndex() { return iMenuIndex; }
	static int32_t GetLastMenuIndex() { return iGlobalMenuIndex; }

	friend class Screen; friend class Entry;
};

// combo box filler
// should be ComboBox::FillCB; but nested class will cause internal compiler faults
class ComboBox_FillCB
{
private:
	ComboBox *pCombo;
	ContextMenu *pDrop;

public:
	virtual ~ComboBox_FillCB() {}

	void FillDropDown(ComboBox *pComboBox, ContextMenu *pDropdownList)
	{
		pCombo = pComboBox; pDrop = pDropdownList; FillDropDownCB();
	}

	virtual void FillDropDownCB() = 0;
	virtual bool OnComboSelChange(ComboBox *pForCombo, int32_t idNewSelection) = 0;

	// to be used in fill-callback only (crash otherwise!)
	void AddEntry(const char *szText, int32_t id, const char *desc = nullptr);
	bool FindEntry(const char *szText);
	void ClearEntries();
};

template <class CB> class ComboBox_FillCallback : public ComboBox_FillCB
{
public:
	typedef void (CB::*ComboFillFunc)(ComboBox_FillCB *pFiller);
	typedef bool (CB::*ComboSelFunc)(ComboBox *pForCombo, int32_t idNewSelection);

private:
	CB *pCBClass;
	ComboFillFunc FillFunc;
	ComboSelFunc SelFunc;

protected:
	virtual void FillDropDownCB() override
	{
		if (pCBClass && FillFunc)(pCBClass->*FillFunc)(this);
	}

	virtual bool OnComboSelChange(ComboBox *pForCombo, int32_t idNewSelection) override
	{
		if (pCBClass && SelFunc) return (pCBClass->*SelFunc)(pForCombo, idNewSelection); else return false;
	}

public:
	ComboBox_FillCallback(CB *pCBClass, ComboFillFunc FillFunc, ComboSelFunc SelFunc) :
		pCBClass(pCBClass), FillFunc(FillFunc), SelFunc(SelFunc) {}
};

// information on how to draw dialog borders and face
class FrameDecoration
{
private:
	int iRefCount;

	bool SetFacetByAction(C4Def *pOfDef, class C4FacetEx &rfctTarget, const char *szFacetName);

public:
	C4ID idSourceDef;
	uint32_t dwBackClr; // background face color
	C4FacetEx fctTop, fctTopRight, fctRight, fctBottomRight, fctBottom, fctBottomLeft, fctLeft, fctTopLeft;
	int iBorderTop, iBorderLeft, iBorderRight, iBorderBottom;
	bool fHasGfxOutsideClientArea;

	FrameDecoration() : iRefCount(0) { Clear(); }
	void Clear(); // zero data

	// create from ActMap and graphics of a definition (does some script callbacks to get parameters)
	bool SetByDef(C4ID idSourceDef);
	bool UpdateGfx(); // update Surface, e.g. after def reload

	void Ref() { ++iRefCount; }
	void Deref() { if (!--iRefCount) delete this; }

	void Draw(C4FacetEx &cgo, C4Rect &rcDrawArea); // draw deco for given rect (rect includes border)
};

// a button closing the Dlg
class CloseButton : public Button
{
protected:
	bool fCloseResult;

	virtual void OnPress() override;

public:
	CloseButton(const char *szBtnText, const C4Rect &rtBounds, bool fResult)
		: Button(szBtnText, rtBounds), fCloseResult(fResult) {}
};

class CloseIconButton : public IconButton
{
protected:
	bool fCloseResult;

	virtual void OnPress() override;

public:
	CloseIconButton(const C4Rect &rtBounds, Icons eIcon, bool fResult)
		: IconButton(eIcon, rtBounds, 0), fCloseResult(fResult) {}
};

// OK button
class OKButton : public CloseButton
{
public: OKButton(const C4Rect &rtBounds)
	: CloseButton(LoadResStr("IDS_DLG_OK"), rtBounds, true) {}
};

class OKIconButton : public CloseIconButton
{
public: OKIconButton(const C4Rect &rtBounds, Icons eIcon)
	: CloseIconButton(rtBounds, eIcon, true) {}
};

// cancel button
class CancelButton : public CloseButton
{
public: CancelButton(const C4Rect &rtBounds)
	: CloseButton(LoadResStr("IDS_DLG_CANCEL"), rtBounds, false) {}
};

class CancelIconButton : public CloseIconButton
{
public: CancelIconButton(const C4Rect &rtBounds, Icons eIcon)
	: CloseIconButton(rtBounds, eIcon, false) {}
};

// Yes button
class YesButton : public CloseButton
{
	public: YesButton(const C4Rect &rtBounds)
		: CloseButton(LoadResStr("IDS_DLG_YES"), rtBounds, true) {}
};

// No button
class NoButton : public CloseButton
{
	public: NoButton(const C4Rect &rtBounds)
		: CloseButton(LoadResStr("IDS_DLG_NO"), rtBounds, false) {}
};

class BaseInputCallback
{
public:
	virtual void OnOK(const StdStrBuf &sText) = 0;
	virtual ~BaseInputCallback() {}
};

template <class T> class InputCallback : public BaseInputCallback
{
public:
	typedef void (T::*CBFunc)(const StdStrBuf &);

private:
	T *pTarget;
	CBFunc pCBFunc;

public:
	InputCallback(T *pTarget, CBFunc pCBFunc) : pTarget(pTarget), pCBFunc(pCBFunc) {}

	virtual void OnOK(const StdStrBuf &sText) override { (pTarget->*pCBFunc)(sText); }
};

// a keyboard event that's only executed if the dialog is activated
template <class TargetClass, template <typename Class, typename...> typename BaseTemplate = C4KeyCB, typename... BasePars>
class DlgKeyCB : public BaseTemplate<TargetClass, BasePars...>
{
private:
	using Base = BaseTemplate<TargetClass, BasePars...>;
	using CallbackFunc = Base::CallbackFunc;

public:
	DlgKeyCB(TargetClass &rTarget, const BasePars &... pars, CallbackFunc pFuncDown, CallbackFunc pFuncUp = nullptr, CallbackFunc pFuncPressed = nullptr)
		: Base(rTarget, pars..., pFuncDown, pFuncUp, pFuncPressed) {}

	virtual bool CheckCondition() override { return Base::rTarget.IsInActiveDlg(true) && Base::rTarget.IsVisible(); }
};

template <class TargetClass>
using DlgKeyCBPassKey = DlgKeyCB<TargetClass, C4KeyCBPassKey>;

template <class TargetClass, class ParameterType>
using DlgKeyCBEx = DlgKeyCB<TargetClass, C4KeyCBEx, ParameterType>;

// a keyboard event that's only executed if the control has focus
template <class TargetClass, template <typename Class, typename...> typename BaseTemplate = C4KeyCB, typename... BasePars>
class ControlKeyCB : public BaseTemplate<TargetClass, BasePars...>
{
private:
	using Base = BaseTemplate<TargetClass, BasePars...>;
	using CallbackFunc = Base::CallbackFunc;

public:
	ControlKeyCB(TargetClass &rTarget, const BasePars &... pars, CallbackFunc pFuncDown, CallbackFunc pFuncUp = nullptr, CallbackFunc pFuncPressed = nullptr)
		: Base(rTarget, pars..., pFuncDown, pFuncUp, pFuncPressed) {}

	virtual bool CheckCondition() override { return Base::rTarget.HasDrawFocus(); }
};

template <class TargetClass, class ParameterType>
using ControlKeyCBExPassKey = ControlKeyCB<TargetClass, C4KeyCBExPassKey, ParameterType>;

// key event that checks whether a control is active, but forwards to the dialog
template <class TargetClass> class ControlKeyDlgCB : public C4KeyCB<TargetClass>
{
private:
	class Control *pCtrl;
	typedef C4KeyCB<TargetClass> Base;
	typedef bool(TargetClass::*CallbackFunc)();

public:
	ControlKeyDlgCB(Control *pCtrl, TargetClass &rTarget, CallbackFunc pFuncDown, CallbackFunc pFuncUp = nullptr, CallbackFunc pFuncPressed = nullptr)
		: Base(rTarget, pFuncDown, pFuncUp, pFuncPressed), pCtrl(pCtrl) {}

	virtual bool CheckCondition() override { return pCtrl && pCtrl->IsInActiveDlg(true) && pCtrl->IsVisible(); }
};

// mouse cursor control
class CMouse
{
public:
	int32_t x, y; // cursor position
	bool LDown, MDown, RDown; // mouse button states
	uint32_t dwKeys; // shift, ctrl, etc.
	bool fActive;
	time_t tLastMovementTime; // timeGetTime() when the mouse pos changed last

	// whether last input was done by mouse
	// set to true whenever mouse pos changes or buttons are pressed
	// reset by keyboard actions to avoid tooltips where the user isn't even doing anything
	bool fActiveInput;

public:
	Element *pMouseOverElement, *pPrevMouseOverElement; // elements at mouse position (after and before processing of mouse event)
	Element *pDragElement;                              // element at pos where left mouse button was clicked last

public:
	CMouse(int32_t iX, int32_t iY);
	~CMouse();

	void Draw(C4FacetEx &cgo, bool fDrawToolTip); // draw cursor

	void Input(int32_t iButton, int32_t iX, int32_t iY, uint32_t dwKeyParam); // process mouse input
	bool IsLDown() { return LDown; }

	void GetLastXY(int32_t &rX, int32_t &rY, uint32_t &rdwKeys) { rX = x; rY = y; rdwKeys = dwKeys; }

	void ResetElements() // reset MouseOver/etc.-controls
	{
		pMouseOverElement = pPrevMouseOverElement = pDragElement = nullptr;
	}

	void ReleaseElements(); // reset MouseOver/etc.-controls, doing the appropriate callbacks
	void OnElementGetsInvisible(Element *pChild); // clear ptr

	void RemoveElement(Element *pChild); // clear ptr

	void SetOwnedMouse(bool fToVal) { fActive = fToVal; }

	void ResetToolTipTime() { tLastMovementTime = timeGetTime(); }
	bool IsMouseStill() { return timeGetTime() - tLastMovementTime >= C4GUI_ToolTipShowTime; }
	void ResetActiveInput() { fActiveInput = false; }
	bool IsActiveInput() { return fActiveInput; }

	void ReleaseButtons() { LDown = RDown = MDown = false; }
};

// the dialog client area
class Screen : public Window
{
public:
	CMouse Mouse;

protected:
	Dialog *pActiveDlg; // current, active dialog
	ContextMenu *pContext; // currently opened context menu (lowest submenu)
	bool fExclusive; // default true. if false, input is shared with the game
	C4Rect PreferredDlgRect; // rectangle in which dialogs should be placed
	C4GamePadOpener *pGamePadOpener;

	static Screen *pScreen; // static singleton var

public:
	virtual void RemoveElement(Element *pChild) override; // clear ptr
	const C4Rect &GetPreferredDlgRect() { return PreferredDlgRect; }

protected:
	virtual void Draw(C4FacetEx &cgo, bool fDoBG); // draw screen contents

	virtual void ElementPosChanged(Element *pOfElement) override; // called when a dialog is moved

public:
	void ShowDialog(Dialog *pDlg, bool fFade); // present dialog to user
	void CloseDialog(Dialog *pDlg, bool fFade); // hide dialog from user
	void ActivateDialog(Dialog *pDlg); // set dlg as active
	void RecheckActiveDialog();

	Dialog *GetTopDialog(); // get topmost dlg

public:
	Screen(int32_t tx, int32_t ty, int32_t twdt, int32_t thgt);
	~Screen();

	void Render(bool fDoBG); // render to lpDDraw
	void RenderMouse(C4FacetEx &cgo); // draw mouse only

	virtual Screen *GetScreen() override { return this; } // return contained screen
	static Screen *GetScreenS() { return pScreen; } // get global screen

	bool IsActive() { return !!GetTopDialog(); } // return whether GUI is active

	bool KeyAny(); // to be called on keystrokes; resets some tooltip-times
	virtual bool CharIn(const char *c); // input: character key pressed - should return false for none-character-inputs
	bool MouseInput(int32_t iButton, int32_t iX, int32_t iY, uint32_t dwKeyParam, Dialog *pDlg, C4Viewport *pVP); // input: mouse movement or buttons; sends MouseEnter/Leave; return whether inside dialog

	bool ShowMessage(const char *szMessage, const char *szCaption, Icons icoIcon, bool *pbConfigDontShowAgainSetting = nullptr); // show message
	bool ShowErrorMessage(const char *szMessage); // show message: Error caption and icon
	bool ShowMessageModal(const char *szMessage, const char *szCaption, uint32_t dwButtons, Icons icoIcon, bool *pbConfigDontShowAgainSetting = nullptr); // show modal message dlg
	bool ShowModalDlg(Dialog *pDlg, bool fDestruct = true); // show any dialog modal and destruct it afterwards
	bool ShowRemoveDlg(Dialog *pDlg); // show dialog, and flag it to delete itself when closed; return immediately

	void CloseAllDialogs(bool fWithOK); // close all dialogs on the screen; top dlgs first
	void SetPreferredDlgRect(const C4Rect &rtNewPref) { PreferredDlgRect = rtNewPref; }
	void DoContext(ContextMenu *pNewCtx, Element *pAtElement, int32_t iX, int32_t iY); // open context menu (closes any other contextmenu)
	void AbortContext(bool fByUser) { if (pContext) pContext->Abort(fByUser); } // close context menu
	int32_t GetContextMenuIndex() { return pContext ? pContext->GetMenuIndex() : 0; } // get current context-menu (lowest level)
	int32_t GetLastContextMenuIndex() { return ContextMenu::GetLastMenuIndex(); } // get last opened context-menu (lowest level)
	bool HasContext() { return !!pContext; }

	bool HasFullscreenDialog(bool fIncludeFading); // true if on fullscreen dialog is visible
	Dialog *GetFullscreenDialog(bool fIncludeFading); // return dlg pointer to first found fullscreen dialog

	// exclusive
	void SetExclusive(bool fToState) { fExclusive = fToState; Mouse.SetOwnedMouse(fExclusive); UpdateMouseFocus(); }
	bool IsExclusive() { return fExclusive; }

	bool HasKeyboardFocus();

	bool HasMouseFocus(); // return whether mouse controls GUI only

	int32_t GetMouseControlledDialogCount(); // count dlgs with that flag set
	void UpdateMouseFocus(); // when exclusive mode has changed: Make sure mouse clip is correct

	// draw a tooltip in a box
	static void DrawToolTip(const char *szTip, C4FacetEx &cgo, int32_t x, int32_t y);

	// gamepad
	void UpdateGamepadGUIControlEnabled(); // callback when gamepad gui option changed

	friend class Dialog; friend class ContextMenu;
};

// menu handler bound to member functions of another class
template <class CBClass> class CBMenuHandler : public MenuHandler
{
private:
	CBClass *pCBTarget; // callback target
	typename DlgCallback<CBClass>::ContextClickFunc pCallbackFn; // callback function

public:
	CBMenuHandler(CBClass *pCBTarget, typename DlgCallback<CBClass>::ContextClickFunc pCallbackFn, int32_t iaExtra = 0)
		: MenuHandler(), pCBTarget(pCBTarget), pCallbackFn(pCallbackFn) {}

	virtual void OnOK(Element *pTargetElement) override
	{
		if (!pCBTarget || !pCallbackFn) return;
		// do callback
		(pCBTarget->*pCallbackFn)(pTargetElement);
	}
};

// menu handler bound to member functions of another class, providing extra storage for parameters
template <class CBClass, class TEx> class CBMenuHandlerEx : public MenuHandler
{
private:
	CBClass *pCBTarget; // callback target
	typename DlgCallbackEx<CBClass, const TEx &>::ContextClickFunc pCallbackFn; // callback function
	TEx Extra; // extra data

public:
	CBMenuHandlerEx(CBClass *pCBTarget, typename DlgCallbackEx<CBClass, const TEx &>::ContextClickFunc pCallbackFn, TEx aExtra)
		: MenuHandler(), pCBTarget(pCBTarget), pCallbackFn(pCallbackFn), Extra(aExtra) {}
	CBMenuHandlerEx(CBClass *pCBTarget, typename DlgCallbackEx<CBClass, const TEx &>::ContextClickFunc pCallbackFn)
		: MenuHandler(), pCBTarget(pCBTarget), pCallbackFn(pCallbackFn), Extra() {}

	void SetExtra(const TEx &e) { Extra = e; }

	virtual void OnOK(Element *pTargetElement) override
	{
		if (!pCBTarget || !pCallbackFn) return;
		// do callback
		(pCBTarget->*pCallbackFn)(pTargetElement, Extra);
	}
};

// context handler bound to member functions of another class
template <class CBClass> class CBContextHandler : public ContextHandler
{
private:
	CBClass *pCBTarget; // callback target
	typename DlgCallback<CBClass>::ContextFunc pCallbackFn; // callback function

public:
	CBContextHandler(CBClass *pCBTarget, typename DlgCallback<CBClass>::ContextFunc pCallbackFn)
		: ContextHandler(), pCBTarget(pCBTarget), pCallbackFn(pCallbackFn) {}

		virtual bool OnContext(Element *pOnElement, int32_t iX, int32_t iY) override
	{
		if (!pCBTarget || !pCallbackFn) return false;
		Screen *pScreen = pOnElement->GetScreen();
		if (!pScreen) return false;
		// let CB func create context menu
		ContextMenu *pNewMenu = (pCBTarget->*pCallbackFn)(pOnElement, iX, iY);
		if (!pNewMenu) return false;
		// open it on screen
		pScreen->DoContext(pNewMenu, pOnElement, iX, iY);
		// done, success
		return true;
	}

	virtual C4GUI::ContextMenu *OnSubcontext(Element *pOnElement) override
	{
		// query directly
		return (pCBTarget->*pCallbackFn)(pOnElement, 0, 0);
	}

	friend class Dialog;
};

// a helper used to align elements
class ComponentAligner
{
protected:
	C4Rect rcClientArea; // free client area
	int32_t iMarginX, iMarginY; // horizontal and vertical margins of components

	static C4Rect rcTemp; // temp rect

public:
	ComponentAligner(const C4Rect &rcArea, int32_t iMarginX, int32_t iMarginY, bool fZeroAreaXY = false)
		: rcClientArea(rcArea), iMarginX(iMarginX), iMarginY(iMarginY)
	{
		if (fZeroAreaXY) rcClientArea.x = rcClientArea.y = 0;
	}

	bool GetFromTop   (int32_t iHgt, int32_t iWdt, C4Rect &rcOut); // get a portion from the top
	bool GetFromLeft  (int32_t iWdt, int32_t iHgt, C4Rect &rcOut); // get a portion from the left
	bool GetFromRight (int32_t iWdt, int32_t iHgt, C4Rect &rcOut); // get a portion from the right
	bool GetFromBottom(int32_t iHgt, int32_t iWdt, C4Rect &rcOut); // get a portion from the bottom
	void GetAll(C4Rect &rcOut); // get remaining client area
	bool GetCentered(int32_t iWdt, int32_t iHgt, C4Rect &rcOut); // get centered subregion within area

	C4Rect &GetFromTop   (int32_t iHgt, int32_t iWdt = -1) { GetFromTop   (iHgt, iWdt, rcTemp); return rcTemp; } // get a portion from the top
	C4Rect &GetFromLeft  (int32_t iWdt, int32_t iHgt = -1) { GetFromLeft  (iWdt, iHgt, rcTemp); return rcTemp; } // get a portion from the left
	C4Rect &GetFromRight (int32_t iWdt, int32_t iHgt = -1) { GetFromRight (iWdt, iHgt, rcTemp); return rcTemp; } // get a portion from the right
	C4Rect &GetFromBottom(int32_t iHgt, int32_t iWdt = -1) { GetFromBottom(iHgt, iWdt, rcTemp); return rcTemp; } // get a portion from the bottom
	C4Rect &GetAll() { GetAll(rcTemp); return rcTemp; } // get remaining client are
	C4Rect &GetCentered(int32_t iWdt, int32_t iHgt) { GetCentered(iWdt, iHgt, rcTemp); return rcTemp; } // get centered subregion within area (w/o size checks)

	C4Rect &GetGridCell(int32_t iSectX, int32_t iSectXMax, int32_t iSectY, int32_t iSectYMax, int32_t iSectSizeX = -1, int32_t iSectSizeY = -1, bool fCenterPos = false, int32_t iSectNumX = 1, int32_t iSectNumY = 1);

	int32_t GetWidth()   const { return rcClientArea.Wdt; }
	int32_t GetHeight()  const { return rcClientArea.Hgt; }
	int32_t GetHMargin() const { return iMarginX; }
	int32_t GetVMargin() const { return iMarginY; }

	int32_t GetInnerWidth()  const { return rcClientArea.Wdt - 2 * iMarginX; }
	int32_t GetInnerHeight() const { return rcClientArea.Hgt - 2 * iMarginY; }

	void ExpandTop   (int32_t iByHgt) { rcClientArea.y -= iByHgt; rcClientArea.Hgt += iByHgt; } // enlarge client rect
	void ExpandLeft  (int32_t iByWdt) { rcClientArea.x -= iByWdt; rcClientArea.Wdt += iByWdt; } // enlarge client rect
	void ExpandRight (int32_t iByWdt) { rcClientArea.Wdt += iByWdt; } // enlarge client rect
	void ExpandBottom(int32_t iByHgt) { rcClientArea.Hgt += iByHgt; } // enlarge client rect
};

// shortcut for check whether GUI is active
inline bool IsGUIValid() { return !!Screen::GetScreenS(); }
inline bool IsActive() { return Screen::GetScreenS() && Screen::GetScreenS()->IsActive(); }
inline bool IsExclusive() { return Screen::GetScreenS() && Screen::GetScreenS()->IsExclusive(); }

// shortcut for GUI screen size
int32_t GetScreenWdt();
int32_t GetScreenHgt();

// sound effect in GUI: Only if enabled
void GUISound(const char *szSound);

} // end of namespace
