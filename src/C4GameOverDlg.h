/* by Sven2, 2008 */
// game over dialog showing winners and losers

#pragma once

#include <C4Gui.h>
#include <C4RoundResults.h>

// horizontal display of goal symbols; filfilled goals marked
// maybe to be reused for a game goal dialog?
class C4GoalDisplay : public C4GUI::Window
{
private:
	// element that draws one goal
	class GoalPicture : public C4GUI::Window
	{
	private:
		C4ID idGoal;
		bool fFulfilled;
		C4FacetExSurface Picture;

	public:
		GoalPicture(const C4Rect &rcBounds, C4ID idGoal, bool fFulfilled);

	protected:
		virtual void DrawElement(C4FacetEx &cgo);
	};

public:
	C4GoalDisplay(const C4Rect &rcBounds) : C4GUI::Window() { SetBounds(rcBounds); }
	virtual ~C4GoalDisplay() {}

	void SetGoals(const C4IDList &rAllGoals, const C4IDList &rFulfilledGoals, int32_t iGoalSymbolHeight);
};

class C4GameOverDlg : public C4GUI::Dialog
{
private:
	static bool is_shown;
	C4Sec1TimerCallback<C4GameOverDlg> *pSec1Timer; // engine timer hook for player list update
	int32_t iPlrListCount;
	class C4PlayerInfoListBox **ppPlayerLists;
	C4GoalDisplay *pGoalDisplay;
	C4GUI::Label *pNetResultLabel; // label showing league result, disconnect, etc.
	C4GUI::Button *pBtnExit, *pBtnContinue, *pBtnRestart, *pBtnNextMission;
	bool fIsNetDone; // set if league is evaluated and round results arrived
	bool fIsQuitBtnVisible; // quit button available? set if not host or when fIsNetDone
	enum NextMissionMode {
		None,
		Restart,
		NextMission
	};
	NextMissionMode nextMissionMode = None;

private:
	void OnExitBtn(C4GUI::Control *btn); // callback: exit button pressed
	void OnContinueBtn(C4GUI::Control *btn); // callback: continue button pressed
	void OnRestartBtn(C4GUI::Control *btn); // callback: restart button pressed
	void OnNextMissionBtn(C4GUI::Control *btn); // callback: next mission button pressed

	void Update();
	void SetNetResult(const char *szResultString, C4RoundResults::NetResult eResultType, size_t iPendingStreamingData, bool fIsStreaming);

protected:
	virtual void OnShown();
	virtual void OnClosed(bool fOK);

	virtual bool OnEnter() { if (fIsQuitBtnVisible) OnExitBtn(nullptr); return true; } // enter on non-button: Always quit
	virtual bool OnEscape() { if (fIsQuitBtnVisible) UserClose(false); return true; } // escape ignored if still streaming

	// true for dialogs that should span the whole screen
	// not just the mouse-viewport
	virtual bool IsFreePlaceDialog() { return true; }

	// true for dialogs that receive full keyboard and mouse input even in shared mode
	virtual bool IsExclusiveDialog() { return true; }

public:
	C4GameOverDlg();
	~C4GameOverDlg();

	static bool IsShown() { return is_shown; }
	void OnSec1Timer() { Update(); }
};
