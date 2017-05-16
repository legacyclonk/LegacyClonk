/* by Sven2, 2005 */
// main game dialogs (abort game dlg, observer dlg)

#include <C4Include.h>
#include <C4GameDialogs.h>

#ifndef BIG_C4INCLUDE
#include <C4Viewport.h>
#include <C4Network2Dialogs.h>
#include <C4Game.h>
#include <C4Player.h>
#endif

bool C4AbortGameDialog::is_shown = false;

// ---------------------------------------------------
// C4GameAbortDlg

C4AbortGameDialog::C4AbortGameDialog()
: fGameHalted(false),
  C4GUI::ConfirmationDialog(LoadResStr("IDS_HOLD_ABORT"), 
														LoadResStr("IDS_DLG_ABORT"),
														NULL,
														MessageDialog::btnYesNo, 
														true, 
														C4GUI::Ico_Exit)
	{
	is_shown = true; // assume dlg will be shown, soon
	}

C4AbortGameDialog::~C4AbortGameDialog()
	{
	is_shown = false;
	}

void C4AbortGameDialog::OnShown()
	{
	if(!Game.Network.isEnabled())
		{
		fGameHalted = true;
		Game.HaltCount++;
		}
	}

void C4AbortGameDialog::OnClosed(bool fOK)
	{
	if(fGameHalted)
		Game.HaltCount--;
	// inherited
	typedef C4GUI::ConfirmationDialog Base;
	Base::OnClosed(fOK);
	// abort
	if(fOK)
		Game.Abort();
	}
