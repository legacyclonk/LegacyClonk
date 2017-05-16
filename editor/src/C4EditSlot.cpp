/* Copyright (C) 1998-2000  Matthes Bender  RedWolf Design */

/* Edit group file components by extracting them to a temp directory */

#include "C4Explorer.h"

C4EditSlots::C4EditSlots()
	{
	Default();
	}

C4EditSlots::~C4EditSlots()
	{
	Clear();
	}

void C4EditSlots::Default()
	{
	First=NULL;
	}

void C4EditSlots::Clear()
	{
	C4EditSlot *pNext;
	while (First)
		{	pNext = First->Next; delete First; First = pNext; }
	First = NULL;
	}

bool C4EditSlots::EditItem(CString &rItemPath, bool fOriginal)
	{
	C4EditSlot *pSlot;
	
	// Item already edited: use relaunch existing slot
	if (pSlot = GetItemSlot(rItemPath) )
		{
		return pSlot->LaunchEditor();
		}

	// Create edit slot
	pSlot = new C4EditSlot;
	if (!pSlot->Init(rItemPath,fOriginal)) 
		{ 
		C4MessageBox("IDS_FAIL_EDIT");
		delete pSlot; 
		return false; 
		}

	// Add slot to list
	pSlot->Next = First;
	First = pSlot;

	// Success
	return true;

	}

bool C4EditSlots::IsItemEdited(CString &rItemPath)
	{
	for (C4EditSlot *pSlot=First; pSlot; pSlot=pSlot->Next)
		if (ItemIdentical(pSlot->ItemPath,rItemPath))
			return true;
	return false;
	}	

bool C4EditSlots::UpdateModifiedItems(bool fBackground)
	{
	C4EditSlot *pSlot,*pNext,*pPrev=NULL;
	
	// Cannot modify original items, remove modified original slots.
	// Ignore if called by background update
	if (!fBackground) 		
		// Safety disabled
		if (!GetCfg()->Explorer.Kindersicherung)
			// Check slots
			for (pSlot=First; pSlot; pSlot=pNext)
				{
				pNext=pSlot->Next;
				// Item modified but no modification access
				if ( pSlot->IsModified() && pSlot->Original && GetCfg()->Explorer.Kindersicherung )
					{
					CString sMsg; sMsg.Format(LoadResStr("IDS_MSG_NOMODIFYORIGINAL"),GetCfg()->AtExeRelativePath(pSlot->ItemPath));
					C4MessageBox(sMsg);
					if (pPrev) pPrev->Next = pNext; else First = pNext;
					delete pSlot;
					}
				else
					pPrev = pSlot;
				}

	// Compose modified list for prompt
	bool fModified = false;
	CString sPrompt = LoadResStr("IDS_MSG_ITEMSMODIFIED");
	for (pSlot=First; pSlot; pSlot=pSlot->Next)
		if (pSlot->IsModified() && (!pSlot->Original || !GetCfg()->Explorer.Kindersicherung))
			{
			fModified = true;
			sPrompt += pSlot->ItemPath + "\n";
			}

	// None modified
	if (!fModified) return false;

	// Update items
	CWaitCursor WaitCursor;
	SetStatus("IDS_STATUS_UPDATINGITEMS");
	for (pSlot=First; pSlot; pSlot=pSlot->Next)
		if (pSlot->IsModified())
			if (!pSlot->Original || !GetCfg()->Explorer.Kindersicherung)
				if (!pSlot->UpdateItem())
					if (!fBackground) C4MessageBox("IDS_FAIL_UPDATE");

	// Done
	return true;
	}

bool C4EditSlots::CloseFinishedItems()
	{
	C4EditSlot *pSlot,*pNext,*pPrev=NULL;
	for (pSlot=First; pSlot; pSlot=pNext)
		{
		pNext=pSlot->Next;
		// Item edit process finished, remove slot
		if (pSlot->IsFinished())
			{
			TRACE("Item %s finished\n\r",pSlot->ItemPath);
			if (pPrev) pPrev->Next = pNext; else First = pNext;
			delete pSlot;
			}
		else
			pPrev = pSlot;
		}
	return true;
	}

bool C4EditSlots::ItemsStillEdited()
	{
	// Compose edited list for prompt
	bool fEdited = false;
	CString sPrompt = LoadResStr("IDS_MSG_ITEMSEDITED");
	for (C4EditSlot *pSlot=First; pSlot; pSlot=pSlot->Next)
		{
		fEdited = true;
		sPrompt += pSlot->ItemPath + "\n";
		}
	// None modified
	if (!fEdited) return false;
	// Prompt
	C4MessageBox(sPrompt);
	return true;
	}

C4EditSlot::C4EditSlot()
	{
	Default();
	}

C4EditSlot::~C4EditSlot()
	{
	Clear();
	}

void C4EditSlot::Default()
	{
	ItemPath.Empty();
	EditPath.Empty();
	EditFolder.Empty();
	EditorPath.Empty();
	ItemTime=0;
	Next=NULL;
	hProcess=NULL;
	Original=false;
	Directory=false;
	}

void C4EditSlot::Clear()
	{
	// Remove edit folder
	EraseItem(EditFolder);
	}

bool C4EditSlot::Init(CString &rItemPath, bool fOriginal)
	{
	CWaitCursor WaitCursor;
	SetStatus("IDS_STATUS_INITEDIT");

	// Item path
	ItemPath = rItemPath;

	// Original
	Original = fOriginal;

	// Type by extension
	int iType = C4IT_Unknown;
	if (*GetExtension(ItemPath))
		iType = GetItemType(GetExtension(ItemPath));

	// Editor by type
	switch (ItemType[iType].Edit)
		{
		case C4ITE_Text:				EditorPath=GetCfg()->Explorer.EditorText;				break;
		case C4ITE_RichText:		EditorPath=GetCfg()->Explorer.EditorRichText;		break;
		case C4ITE_Sound:				EditorPath=GetCfg()->Explorer.EditorSound;			break;
		case C4ITE_Bitmap:			EditorPath=GetCfg()->Explorer.EditorBitmap;			break;
		case C4ITE_PNG:					EditorPath=GetCfg()->Explorer.EditorPNG;				break;
		case C4ITE_Music:				EditorPath=GetCfg()->Explorer.EditorMusic;			break;
		case C4ITE_Script:			EditorPath=GetCfg()->Explorer.EditorScript;			break;
		case C4ITE_Zip:					EditorPath=GetCfg()->Explorer.EditorZip;				break;
		case C4ITE_Definition:	EditorPath=GetCfg()->Explorer.EditorDefinition;	break;
		case C4ITE_Html:				EditorPath=GetCfg()->Explorer.EditorHtml;				break;
		}
	// No editor specified
	if (EditorPath.IsEmpty()) return false;
	
	// Create edit folder
	if (!CreateEditFolder(EditFolder)) return false;

	// Compose edit path
	EditPath.Format("%s\\%s%s",EditFolder,(Original && GetCfg()->Explorer.Kindersicherung) ? "~" : "",GetFilename(ItemPath));

	// Extract item to edit path
	C4Group ParentGroup;
	CString ParentPath = NoBackslash(GetPath(ItemPath));
	if ( !ParentGroup.Open(ParentPath)
		|| !ParentGroup.ExtractEntry(GetFilename(rItemPath),EditPath)
		|| !ParentGroup.Close() )
			return false;

	// Directory item
	C4Group_UnpackDirectory(EditPath);
	if (DirectoryExists(EditPath)) Directory=true;

	// Get edit file time
	ItemTime = FileTime(EditPath);

	// Launch edit slots for any directory child items...!

	// Create edit process
	if (!LaunchEditor()) return false;

	return true;
	}

bool C4EditSlot::CreateEditFolder(CString &rPath)
	{
	CString Foldername;
	CString Folderpath;
	int iIndex = 0;
	// Create unused edit folder path
	do
		{
		iIndex++;
		Foldername.Format("Editing%i",iIndex);
		Folderpath = GetCfg()->AtTempPath(Foldername);
		}
	while (ItemExists(Folderpath));
	// Create edit folder
	if (!CreateDirectory(Folderpath,NULL)) return false;
	// Success
	rPath = Folderpath;
	return true;
	}


bool C4EditSlot::IsModified()
	{
	if (ItemTime != FileTime(EditPath)) return true;
	return false;
	}

bool C4EditSlot::UpdateItem()
	{
	// Directory items are not updated
	if (Directory) return true;
	// Add file to group
	TRACE("Updating %s...\r\n",ItemPath);
	C4Group ParentGroup;
	CString ParentPath = NoBackslash(GetPath(ItemPath));
	if ( !ParentGroup.Open(ParentPath)
		|| !ParentGroup.Add(EditPath)
		|| !ParentGroup.Close() )
			return false;
	// Set new edit file time
	ItemTime = FileTime(EditPath);
	// Notify update
	GetApp()->m_pMainWnd->PostMessage(WM_USER_UPDATEITEM,(WPARAM)TRUE,(LPARAM)(const char*)ItemPath);
	// Success
	return true;
	}

bool C4EditSlot::IsFinished()
	{
	DWORD dwStatus;
	// Get Status
	if (!GetExitCodeProcess(hProcess,&dwStatus))
		{ TRACE("Error on edit process status\n\r"); return true; }
	// Still active
	if (dwStatus==STILL_ACTIVE) return false;
  // Finished
	return true;
	}

C4EditSlot* C4EditSlots::GetItemSlot(CString &rItemPath)
	{
	for (C4EditSlot *pSlot=First; pSlot; pSlot=pSlot->Next)
		if (ItemIdentical(pSlot->ItemPath,rItemPath))
			return pSlot;
	return NULL;
	}

bool C4EditSlot::LaunchEditor()
	{

	// Shell execute (not with directory items)
	if (GetCfg()->Explorer.EditorUseShell && !Directory)
		{
		hProcess = ShellExecute( GetApp()->m_pMainWnd->m_hWnd, "open", EditPath, NULL, EditFolder, SW_SHOW );
		if (!hProcess) return false;
		}

	// Custom editor
	else
		{
		CString CmdLine;
		CmdLine.Format("\"%s\" \"%s\"",EditorPath,EditPath); 
		STARTUPINFO StartupInfo; SetZero(StartupInfo);
		StartupInfo.cb = sizeof StartupInfo;
		PROCESS_INFORMATION ProcessInfo; SetZero(ProcessInfo);
		char CmdBuf[4096]; SCopy(CmdLine,CmdBuf);
		if (!CreateProcess(NULL, CmdBuf, NULL, NULL, TRUE, 0, NULL, NULL, &StartupInfo, &ProcessInfo)) return false;
		hProcess = ProcessInfo.hProcess;
		}

	return true;
	}

bool C4EditSlots::RemoveItem(const char *szItemPath)
	{
	C4EditSlot *pSlot,*pNext,*pPrev=NULL;
	for (pSlot=First; pSlot; pSlot=pNext)
		{
		pNext=pSlot->Next;
		if (pSlot->ItemPath.CompareNoCase(szItemPath)==0)
			{
			TRACE("Edit slot %s closed due to source modification\n\r",pSlot->ItemPath);
			if (pPrev) pPrev->Next = pNext; else First = pNext;
			delete pSlot;
			}
		else
			pPrev = pSlot;
		}
	return true;
	}

bool C4EditSlots::ClearItems(const char *szPath)
	{
	C4EditSlot *pSlot,*pNext,*pPrev=NULL;
	for (pSlot=First; pSlot; pSlot=pNext)
		{
		pNext=pSlot->Next;
		if (pSlot->ItemPath.Left(SLen(szPath)).CompareNoCase(szPath)==0)
			{
			TRACE("Edit slot %s closed due to source modification\n\r",pSlot->ItemPath);
			if (pPrev) pPrev->Next = pNext; else First = pNext;
			delete pSlot;
			}
		else
			pPrev = pSlot;
		}
	return true;
	}

void C4EditSlots::ClearLeftover()
	{
	EraseItems(GetCfg()->AtTempPath("Editing*"));
	}
