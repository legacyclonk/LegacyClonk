// About/credits screen

#ifndef INC_C4StartupAboutDlg
#define INC_C4StartupAboutDlg

#include "C4Startup.h"

// webcode show time
const int32_t C4AboutWebCodeShowTime = 25; // seconds

// startup dialog: About
class C4StartupAboutDlg : public C4StartupDlg
	{
	public:
		C4StartupAboutDlg(); // ctor
		~C4StartupAboutDlg(); // dtor

	private:
		class C4KeyBinding *pKeyBack;
		C4GUI::Label *pWebCodeLbl;
		int32_t iWebCodeTimer;
		C4Sec1TimerCallback<C4StartupAboutDlg> *pSec1Timer; // engine timer hook for display of webcode

	protected:
		virtual int32_t GetMarginTop() { return iDlgMarginY + C4GUI_FullscreenDlg_TitleHeight/2; } // less top margin

		virtual void MouseInput(C4GUI::CMouse &rMouse, int32_t iButton, int32_t iX, int32_t iY, DWORD dwKeyParam); // input: back on any button
		virtual bool OnEnter() { DoBack(); return true; }
		virtual bool OnEscape() { DoBack(); return true; }
		virtual void DrawElement(C4FacetEx &cgo);
		bool KeyBack() { DoBack(); return true; }
		void OnBackBtn(C4GUI::Control *btn) { DoBack(); }
		void OnRegisterBtn(C4GUI::Control *btn);
		void OnUpdateBtn(C4GUI::Control *btn);

	public:
		void OnSec1Timer();

		void DoBack(); // back to main menu	

	//public:
	//	void RecreateDialog(bool fFade);

	};


#endif // INC_C4StartupAboutDlg
