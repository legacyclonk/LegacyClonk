/* Copyright (C) 1998-2000  Matthes Bender  RedWolf Design */

/* The main content view */

/* Search in *.cpp for
   EVIL_C4GROUP_FINDENTRY_BUG
	 to find all places where code had to be changed due to
	 an evil C4Group Find(Next)Entry bug, which apparently
	 is kinda specific to my (carlo's) compiler (??).
	 In all cases, the workaround code can be deactivated
	 by changing "#if 1" to "#if 0" (blah).
*/

#include "C4Explorer.h"

#include "C4ExplorerDlg.h"
#include "C4TypeSelectDlg.h"
//#include "C4PlayerDlg.h"
#include "C4ScenarioDlg.h"
#include "C4ScenarioDefinitionsDlg.h"

#define ContentsDummy "Empty"

#define C4ViewportClassName "C4Viewport"

CImageList *pIconList = NULL; // Hack
C4ViewItem *pUsedEngine = NULL;

BEGIN_MESSAGE_MAP(C4ViewCtrl, CTreeCtrl)
	//{{AFX_MSG_MAP(C4ViewCtrl)
	ON_NOTIFY_REFLECT(TVN_ITEMEXPANDING, OnItemExpanding)
	ON_NOTIFY_REFLECT(TVN_SELCHANGED, OnSelChanged)
	ON_NOTIFY_REFLECT(TVN_ITEMEXPANDED, OnItemExpanded)
	ON_NOTIFY_REFLECT(TVN_ENDLABELEDIT, OnEndLabelEdit)
	ON_NOTIFY_REFLECT(TVN_KEYDOWN, OnKeydown)
	ON_NOTIFY_REFLECT(NM_DBLCLK, OnDoubleClick)
	ON_NOTIFY_REFLECT(NM_RETURN, OnReturn)
	ON_NOTIFY_REFLECT(TVN_BEGINDRAG, OnBeginDrag)
	ON_WM_MOUSEMOVE()
	ON_WM_LBUTTONUP()
	ON_WM_DROPFILES()
	ON_WM_CREATE()
	ON_WM_RBUTTONDOWN()
	ON_WM_RBUTTONUP()
	ON_WM_LBUTTONDOWN()
	ON_MESSAGE(WM_USER_INIT, OnInit)
	ON_WM_SYSKEYDOWN()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

C4ViewCtrl::C4ViewCtrl()
	{
	Default();
	}

C4ViewCtrl::~C4ViewCtrl()
	{
	Clear();
	}

void C4ViewCtrl::Default()
	{
	Mode = C4VC_Player;
	pDragItem = NULL;
	pDragImageList = NULL;
	ViewItemList.RemoveAll();
	fDragCopy = true;
	ActivationByScenario = false;
	}

void C4ViewCtrl::Clear()
	{
	pUsedEngine=NULL;
	UpdateItemInfo(NULL);
	ClearViewItems("");
	ImageList.DeleteImageList();
	StateList.DeleteImageList();
	if (IsWindow(m_hWnd)) DeleteAllItems();
	if (pDragImageList) delete pDragImageList; pDragImageList = NULL;
	}

bool C4ViewCtrl::Init(CString &rRootFolder, int iMode)
	{

	iMode = C4VC_Developer; // editor now runs in developer mode _only_

	// Mode
	Mode = iMode;
	ModifyStyle(0,TVS_HASBUTTONS | TVS_HASLINES | TVS_LINESATROOT); 

	// Root filename
	RootFilename = NoBackslash(rRootFolder);

	// Image lists
	LoadImageList(IDB_ITEMICONS,ImageList,16,16);
	SetImageList(&ImageList,TVSIL_NORMAL);
	LoadImageList(IDB_ITEMSTATE,StateList,16,16);	
	SetImageList(&StateList,TVSIL_STATE);
	pIconList = &ImageList; // Hack

	// Load root contents
	SetStatus("IDS_STATUS_RELOADING");
	LoadItems();
	
	return true;
	}


/* Load and insert all children to this parent view item */

bool C4ViewCtrl::LoadItems(C4ViewItem *pParentViewItem)
	{
	C4Group ParentGroup;
	CString ParentPath;	
	C4ViewItem ChildViewItem;
	CString ChildPath;
	char ChildName[_MAX_FNAME+1];
	CWaitCursor WaitCursor;

	// Prepare parent
	if (pParentViewItem) 
		{
		// Set parent filename
		ParentPath = pParentViewItem->Path;
		// Set parent flags & remove dummy contents
		pParentViewItem->ContentsLoaded = true;
		RemoveDummyItem(pParentViewItem->hItem);
		}
	else 
		ParentPath = RootFilename;

	TRACE("Loading children %s...\n\r",ParentPath);

	// Open parent group
	if (!ParentGroup.Open(ParentPath)) return false;

	// Search children
	// EVIL_C4GROUP_FINDENTRY_BUG
#if 1
	if (ParentGroup.GetStatus()==GRPF_Folder)
		{
		_finddata_t fdat;
		int hf = _findfirst(CString(ParentPath+"\\*.*"),&fdat);
		int res = hf;
		while (res != -1)
			{
			if (fdat.name[0] != '.')
				{
				ChildPath.Format("%s\\%s",ParentPath,fdat.name);
				// Load child info and insert child
				if (ChildViewItem.Load(ParentGroup,ChildPath,Mode))
					Insert(ChildViewItem,pParentViewItem);
				// Prepare child variable for next load
				// Avoid loss of dynamic data on destruction
				ChildViewItem.Default();
				}
			res = _findnext(hf,&fdat);
			}
		if (hf != -1) _findclose(hf);
		}
	else
		{
#endif
		bool fContinuedSearch = false;
		ParentGroup.ResetSearch();
		while (ParentGroup.FindNextEntry("*",ChildName,NULL,NULL,fContinuedSearch))
			{
			fContinuedSearch = true;
			ChildPath.Format("%s\\%s",ParentPath,ChildName);
			// Load child info and insert child
			if (ChildViewItem.Load(ParentGroup,ChildPath,Mode))
				Insert(ChildViewItem,pParentViewItem);
			// Prepare child variable for next load
			// Avoid loss of dynamic data on destruction
			ChildViewItem.Default(); 
			}
#if 1
		} // EVIL_C4GROUP_FINDENTRY_BUG
#endif

	// Close parent group
	ParentGroup.Close();

	return true;
	}	


/* Add view item to list, insert tree item */

bool C4ViewCtrl::Insert(C4ViewItem &rViewItem, 
												C4ViewItem *pParentViewItem)
	{
	
	// Completely and always ignore CVS/SVN directories
	if (SEqualNoCase(GetFilename(rViewItem.Path), "CVS") || SEqualNoCase(GetFilename(rViewItem.Path), ".svn"))
		return false;

	// Get parent tree node
	HTREEITEM hParentItem = NULL;
	if (pParentViewItem) hParentItem = pParentViewItem->hItem;
	
	// Insert position
	HTREEITEM hInsertAfter = GetInsertPosition(&rViewItem,hParentItem);

	// Insert item and set view item reference
	rViewItem.hItem = InsertItem(rViewItem.Title,
															 hParentItem,
															 hInsertAfter ? hInsertAfter : TVI_FIRST);
	
	// Set item image
	SetItemImage(rViewItem.hItem,rViewItem.Icon,rViewItem.Icon);

	// EnableActivation: set state image mask
	if (rViewItem.EnableActivation)
		SetItemState(rViewItem.hItem,INDEXTOSTATEIMAGEMASK(rViewItem.GetState(ActivationByScenario)),TVIS_STATEIMAGEMASK);

	// Folder: bold
	if (Mode == C4VC_Player)
		if (rViewItem.Type == C4IT_Folder)
			SetItemState(rViewItem.hItem, TVIS_BOLD, TVIS_BOLD);

	// Unregistered and non-free folder: grayed out
	/*if (!GetCfg()->Registered())  FREEWARE
		if (Mode == C4VC_Player)
			if (rViewItem.Type == C4IT_Folder)
				if ( !GetCfg()->IsFreeFolder(rViewItem.Filename, rViewItem.Author) 
					&& !(rViewItem.Filename == "Demos.c4f") )
					SetItemState(rViewItem.hItem, TVIS_CUT, TVIS_CUT | TVIS_BOLD);*/

	// Insert dummy contents for groups
	if (ItemType[rViewItem.Type].Expand)
		InsertItem(ContentsDummy,rViewItem.hItem);
	else
		rViewItem.ContentsLoaded = true;
	
	// Add view item to data list
	ViewItemList.AddTail(rViewItem);
	
	return true;
	}

void C4ViewCtrl::OnItemExpanding(NMHDR* pNMHDR, LRESULT* pResult) 
	{

	NM_TREEVIEW* pNMTreeView = (NM_TREEVIEW*)pNMHDR;
	HTREEITEM hItem = pNMTreeView->itemNew.hItem;
	C4ViewItem *pViewItem=GetViewItem(hItem);
	*pResult = 0;
	if (!pViewItem) return;

	// Status message
	CString sText; sText.Format(LoadResStr("IDS_STATUS_LOADING"),GetItemTitle(pViewItem)); SetStatus(sText);	

	switch (pNMTreeView->action)
		{
		case TVE_EXPAND:
			// Load contents if necessary, else just expand
			if (!pViewItem->ContentsLoaded)									
				if (!LoadItems(pViewItem))
					*pResult = TRUE; // No contents, no expansion
			break;
		}

	}

C4ViewItem* C4ViewCtrl::GetViewItem(HTREEITEM hItem)
	{
	POSITION pos=ViewItemList.GetHeadPosition();
	while (pos)
		{
		C4ViewItem &rViewItem = ViewItemList.GetNext(pos);
		if (rViewItem.hItem==hItem)
			return &rViewItem;
		}	
	return NULL;
	}

void C4ViewCtrl::OnSelChanged(NMHDR* pNMHDR, LRESULT* pResult) 
	{
	NM_TREEVIEW* pNMTreeView = (NM_TREEVIEW*)pNMHDR;
	HTREEITEM hItem = pNMTreeView->itemNew.hItem;
	C4ViewItem *pViewItem=GetViewItem(hItem);
	*pResult = 0;
	UpdateItemInfo(pViewItem);
	UpdateActivationState(pViewItem);
	UpdateUsedEngine(pViewItem);
	}

void C4ViewCtrl::OnItemExpanded(NMHDR* pNMHDR, LRESULT* pResult) 
	{
	NM_TREEVIEW* pNMTreeView = (NM_TREEVIEW*)pNMHDR;
	HTREEITEM hItem = pNMTreeView->itemNew.hItem;
	C4ViewItem *pViewItem=GetViewItem(hItem);
	*pResult = 0;
	if (!pViewItem) return;

	// Set icons
	switch (pNMTreeView->action)
		{
		case TVE_EXPAND:
			SetItemImage(pViewItem->hItem,pViewItem->Icon2,pViewItem->Icon2);
			break;
		case TVE_COLLAPSE:
			SetItemImage(pViewItem->hItem,pViewItem->Icon,pViewItem->Icon);
			break;
		}

	}	

bool C4ViewCtrl::OnRename()
	{
	C4ViewItem *pViewItem = GetViewItem(GetSelectedItem());
	if (!pViewItem) return false;

	// Check modification access
	if (!ModificationOkay(pViewItem)) return false;

	fEditLabelOK=true;
	EditLabel(pViewItem->hItem);

	return true;
	}

bool C4ViewCtrl::OnDelete()
	{
	C4ViewItem *pViewItem = GetViewItem(GetSelectedItem());
	if (!pViewItem) return false;

	// Check modification access
	if (!ModificationOkay(pViewItem,true)) return false;

	// Confirm prompt
	const char* idPrompt = "IDS_MSG_PROMPTDELETE"; 
	if (IsOriginal(pViewItem)) 
		idPrompt = "IDS_MSG_DELETEORIGINAL";
	CString sText; sText.Format(LoadResStr(idPrompt),GetItemTitle(pViewItem));
	if (C4MessageBox(sText, true)!=IDOK) 
		{ SetFocus(); return false; }

	// Wait cursor & status message
	CWaitCursor WaitCursor;
	sText.Format(LoadResStr("IDS_STATUS_DELETING"),GetItemTitle(pViewItem));
	SetStatus(sText);

	// Update participants
	GetCfg()->RemoveModule(pViewItem->Path,GetCfg()->General.Participants);
	GetCfg()->RemoveModule(pViewItem->Path,GetCfg()->Explorer.Definitions);

	// Delete file
	C4Group ParentGroup;
	CString ParentPath = NoBackslash(GetPath(pViewItem->Path));
	if ( !ParentGroup.Open(ParentPath)
		|| !ParentGroup.DeleteEntry(pViewItem->Filename, true)
		|| !ParentGroup.Close() )
			{
			C4MessageBox(LoadResStr("IDS_FAIL_DELETE"));
			return false;
			}

	// Remove item
	RemoveItem(pViewItem->Path,true);

	// Reset focus
	SetFocus();

	return true;
	}

void C4ViewCtrl::OnEndLabelEdit(NMHDR* pNMHDR, LRESULT* pResult) 
	{
	TV_DISPINFO* pTVDispInfo = (TV_DISPINFO*)pNMHDR;
	HTREEITEM hItem = pTVDispInfo->item.hItem;
	C4ViewItem *pViewItem=GetViewItem(hItem);
	C4ViewItem *pParentViewItem=GetParentViewItem(hItem);
	*pResult = 0;
	if (!pTVDispInfo->item.pszText || !pViewItem) return;
	if (!fEditLabelOK) return;

	// Check modification access
	if (!ModificationOkay(pViewItem)) return;

	// Prepare data
	CWaitCursor WaitCursor;
	C4Group ParentGroup;
	CString ParentPath = NoBackslash(GetPath(pViewItem->Path));
	CString OldFilename = pViewItem->Filename;
	CString OldName = pViewItem->Title;
	CString NewName = pTVDispInfo->item.pszText; RemoveCharacters(NewName,"!\"§%&/=?+*#:;<>\\");

	// Status message
	CString sText; sText.Format(LoadResStr("IDS_STATUS_RENAMING"),GetItemTitle(pViewItem));
	SetStatus(sText);

	// Set new filename
	CString NewFilename = NewName;
	if (Mode == C4VC_Player)
		if (ItemType[pViewItem->Type].Extension[0])
			NewFilename.Format("%s.%s",NewName,ItemType[pViewItem->Type].Extension);
	CString NewPath; NewPath.Format("%s\\%s",ParentPath,NewFilename);
	
	// No change?
	if ((NewName==OldName) && (NewFilename==OldFilename)) return;

	// Item of that name already exists - cannot replace using editor-rename
	if (ParentGroup.Open(ParentPath))
	{
		if (ParentGroup.FindEntry(NewFilename))
			{	C4MessageBox(LoadResStr("IDS_FAIL_RENAMEDUPLICATE")); ParentGroup.Close(); SetFocus(); return; }
		ParentGroup.Close();
	}

	// Rename file
	if ( !ParentGroup.Open(ParentPath)
		|| !ParentGroup.Rename(OldFilename,NewFilename)
		|| !ParentGroup.Close() )
			{	C4MessageBox(LoadResStr("IDS_FAIL_RENAME")); SetFocus(); return; }

	// Player mode operation
	if (Mode == C4VC_Player)
		{
		C4Group ItemGroup;
		C4PlayerInfoCore C4P;
		C4ObjectInfoCore C4I;
		C4Scenario C4S;
		C4DefCore C4D;
		switch (pViewItem->Type)
			{
			// - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
			case C4IT_Player:
				// Modify player info core
				if ( !ItemGroup.Open(NewPath)	|| !C4P.Load(ItemGroup) )	{	C4MessageBox("IDS_FAIL_MODIFY"); return; }
				SCopy(NewName,C4P.PrefName,C4MaxName);
				if ( !C4P.Save(ItemGroup)	|| !ItemGroup.Close() )	{	C4MessageBox("IDS_FAIL_MODIFY"); return; }
				break;
			// - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
			case C4IT_ObjectInfo:
				// Modify object info core
				if ( !ItemGroup.Open(NewPath)	|| !C4I.Load(ItemGroup) )	{	C4MessageBox("IDS_FAIL_MODIFY"); return; }
				SCopy(NewName,C4I.Name,C4MaxName);
				if ( !C4I.Save(ItemGroup, NULL)	|| !ItemGroup.Close() )	{	C4MessageBox("IDS_FAIL_MODIFY"); return; }
				break;
			// - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
			case C4IT_Folder: case C4IT_Group: case C4IT_WebLibLink:
				// Remove title component
				if ( !ItemGroup.Open(NewPath)	)	{	C4MessageBox("IDS_FAIL_MODIFY"); return; }
				ItemGroup.Delete(C4CFN_Titles); ItemGroup.Delete(C4CFN_DefNameFiles);
				if ( !ItemGroup.Close() )	{	C4MessageBox("IDS_FAIL_MODIFY"); return; }
				break;
			// - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
			case C4IT_Scenario:
				// Modify scenario core and remove title component
				if ( !ItemGroup.Open(NewPath)	|| !C4S.Load(ItemGroup) )	{	C4MessageBox("IDS_FAIL_MODIFY"); return; }
				SCopy(NewName,C4S.Head.Title,C4MaxTitle);
				ItemGroup.Delete(C4CFN_Titles);
				if ( !C4S.Save(ItemGroup)	|| !ItemGroup.Close() )	{	C4MessageBox("IDS_FAIL_MODIFY"); return; }
				break;
			// - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
			case C4IT_Definition:
				// Modify def core and remove title component
				if ( !ItemGroup.Open(NewPath)	)	{	C4MessageBox("IDS_FAIL_MODIFY"); return; }
				if (C4D.Load(ItemGroup)) { C4D.Name = NewName; C4D.Save(ItemGroup); }
				ItemGroup.Delete(C4CFN_DefNameFiles); ItemGroup.Delete(C4CFN_Titles); 
				if ( !ItemGroup.Close() )	{	C4MessageBox("IDS_FAIL_MODIFY"); return; }
				break;
			// - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
			}
		}

	// Update modules
	if (GetCfg()->RemoveModule(pViewItem->Path,GetCfg()->General.Participants))
		GetCfg()->AddModule(NewPath,GetCfg()->General.Participants);
	if (GetCfg()->RemoveModule(pViewItem->Path,GetCfg()->Explorer.Definitions))
		GetCfg()->AddModule(NewPath,GetCfg()->Explorer.Definitions);

	// Remove old item
	RemoveItem(pViewItem->Path,true);

	// Load new item
	LoadItem(NewPath,pParentViewItem);

	// Reselect item
	SelectViewItem(GetViewItem(NewPath));

	}

#define VK_APPS 0x5D

void C4ViewCtrl::OnKeydown(NMHDR* pNMHDR, LRESULT* pResult) 
	{
	TV_KEYDOWN* pTVKeyDown = (TV_KEYDOWN*)pNMHDR;

	switch (pTVKeyDown->wVKey)
		{
		case VK_DELETE: OnDelete(); break;
		case VK_F2: OnRename(); break;
		case VK_F5: OnReload(); break;
		case VK_F6: OnSwitchToEngine(); break;
		case VK_SPACE: OnActivate(); break;
		case VK_INSERT: OnNew(); break;
		case VK_APPS: OnContextMenu(); break;
		}

	*pResult = 0;
	}

bool C4ViewCtrl::EditLabelOK()
	{
	CEdit *ctlEdit = GetEditControl();
	if (!ctlEdit) return false;
	ctlEdit->DestroyWindow();
	return true;
	}

bool C4ViewCtrl::EditLabelCancel()
	{
	CEdit *ctlEdit = GetEditControl();
	if (!ctlEdit) return false;
	fEditLabelOK=false;
	ctlEdit->DestroyWindow();
	return true;
	}

void C4ViewCtrl::ClearViewItems(CString byPath)
	{
	bool fRemoved; int iRemoved=0;
	do
		{
		fRemoved=false;
		POSITION pos=ViewItemList.GetHeadPosition();
		POSITION cpos;
		while (pos)
			{
			cpos=pos;
			C4ViewItem &rViewItem = ViewItemList.GetNext(pos);
			if (SEqual2(rViewItem.Path,byPath))
				{
				((C4ExplorerDlg *)GetParent())->OnClearViewItem(&rViewItem);
				ViewItemList.RemoveAt(cpos);
				iRemoved++; fRemoved=true;
				break;
				}	
			}
		}
	while (fRemoved);
	TRACE("%i view items cleared\n\r",iRemoved);
	}

C4ViewItem* C4ViewCtrl::GetParentViewItem(HTREEITEM hItem)
	{
	return GetViewItem( GetParentItem(hItem) );
	}

void C4ViewCtrl::OnDoubleClick(NMHDR* pNMHDR, LRESULT* pResult) 
	{

	// Check selected item
	C4ViewItem *pViewItem = GetViewItem(GetSelectedItem());
	if (!pViewItem) return;
	
	*pResult = OnEdit();
	}

void C4ViewCtrl::OnReturn(NMHDR* pNMHDR, LRESULT* pResult) 
	{
	*pResult = OnEdit();
	}

bool C4ViewCtrl::OnEdit()
	{
	bool fResult;
	// Check selected item specified editor
	C4ViewItem *pViewItem = GetViewItem(GetSelectedItem());
	if (!pViewItem || !ItemType[pViewItem->Type].Edit) return false;
	// Shell launch (no edit)
	if (ItemType[pViewItem->Type].Edit==C4ITE_ShellLaunch) fResult=OnLaunch();
	// Launch edit slot
	else fResult=GetApp()->EditSlots.EditItem(pViewItem->Path,IsOriginal(pViewItem));
	// Reset focus
	SetFocus();
	// Return
	return fResult;
	}

void C4ViewCtrl::UpdateItemInfo(C4ViewItem *pViewItem)
	{
	if (IsWindow(m_hWnd))
		((C4ExplorerDlg*)GetParent())->UpdateItemInfo(pViewItem);
	}

bool C4ViewCtrl::OnActivate()
	{
	C4ViewItem *pViewItem = GetViewItem(GetSelectedItem());
	if (!pViewItem || !pViewItem->EnableActivation) return false;

	// Locked item
	if (pViewItem->LockActivation)
		{ C4MessageBox("IDS_MSG_LOCKACTIVATION"); return false; }

	CWaitCursor WaitCursor;

	// Toggle view item state
	pViewItem->Activated = !pViewItem->Activated;

	// Set item state
	SetItemState(pViewItem->hItem,INDEXTOSTATEIMAGEMASK(pViewItem->GetState(ActivationByScenario)),TVIS_STATEIMAGEMASK);

	// need to check max cfg-string length for max item activation!

	// Object operation
	C4Group ItemGroup;
	C4ObjectInfoCore C4I;
	switch (pViewItem->Type)
		{
		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
		case C4IT_Player:
			if (pViewItem->Activated)
				GetCfg()->AddModule(pViewItem->Path,GetCfg()->General.Participants);
			else
				GetCfg()->RemoveModule(pViewItem->Path,GetCfg()->General.Participants);
			break;
		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
		case C4IT_ObjectInfo:
			// Modify object info core
			if ( !ItemGroup.Open(pViewItem->Path)	|| !C4I.Load(ItemGroup) )	{	C4MessageBox("IDS_FAIL_MODIFY"); return false; }
			C4I.Participation = pViewItem->Activated;
			if ( !C4I.Save(ItemGroup, NULL)	|| !ItemGroup.Close() )	{	C4MessageBox("IDS_FAIL_MODIFY"); return false; }
			break;
		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
		case C4IT_Definition:
			if (pViewItem->Activated)
				{
				GetCfg()->AddModule(pViewItem->Path,GetCfg()->Explorer.Definitions);
				if (SLen(GetCfg()->Explorer.Definitions)>900)
					C4MessageBox("IDS_MSG_TOOMANYDEFS");
				}
			else
				GetCfg()->RemoveModule(pViewItem->Path,GetCfg()->Explorer.Definitions);
			break;
		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
		case C4IT_Engine:
			if (pViewItem->Activated)
				GetCfg()->AddModule(pViewItem->Path,GetCfg()->Explorer.Engines);
			else
				GetCfg()->RemoveModule(pViewItem->Path,GetCfg()->Explorer.Engines);
			break;
		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
		}

	UpdateItemInfo(pViewItem);

	return true;
	}

C4ViewItem* C4ViewCtrl::GetSelectedViewItem()
	{
	return GetViewItem(GetSelectedItem());
	}

void C4ViewCtrl::OnNew()
	{
	C4TypeSelectDlg TypeSelectDlg;
	C4ViewItem *pViewItem;

	// Determine target location
	CString ParentPath = RootFilename;
	CString ParentTitle = LoadResStr("IDS_FN_ROOT");
	C4ViewItem *pParentViewItem = GetSelectedViewItem();
	if (pParentViewItem)
		{
		// Get parent path & title
		ParentPath = pParentViewItem->Path;
		ParentTitle = GetFullItemTitle(pParentViewItem);
		// Expand parent item
		if (!pParentViewItem->ContentsLoaded) LoadItems(pParentViewItem);
		}

	// Type selection dialog
	TypeSelectDlg.Location.Format(LoadResStr("IDS_CTL_LOCATION"),ParentTitle);
	TypeSelectDlg.NewMode=1; if (Mode==C4VC_Player) TypeSelectDlg.NewMode=2;
	TypeSelectDlg.DoModal();
	int iType = TypeSelectDlg.Selection;

	// Reset focus
	SetFocus();

	// Create new item at temp path
	CWaitCursor WaitCursor;
	CString TempItemPath; 
	switch (iType)
		{
		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
		case C4IT_Directory:
			TempItemPath = GetCfg()->AtTempPath(LoadResStr("IDS_FN_NEWDIR"));
			// Create new item
			if (!CreateDirectory(TempItemPath,0)) return;
			break;
		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
		default:
			TempItemPath.Format("%sNew.%s",GetCfg()->General.TempPath,ItemType[iType].Extension);
			// Type failure
			if (!ItemType[iType].NewBinary) return;
			// Extract binary resource to temp file
			if (!SaveBinaryResource(ItemType[iType].NewBinary,TempItemPath))
				{ C4MessageBox("IDS_FAIL_EXTRACT"); return; }
			// Add additional components to new file
			switch (iType)
				{
				case C4IT_Player:
					// Open player group 
					C4Group hGroup;
					if (hGroup.Open(TempItemPath))
						{
						// Create an info core
						C4PlayerInfoCore C4P;
						C4P.Default(&GetApp()->PlayerRanks);
						// Set name, color, comment
						SCopy(LoadResStr("IDS_PLR_NEWPLAYER"), C4P.PrefName, C4MaxName);
						C4P.PrefColor = Random(8);
						C4P.PrefColorDw = C4P.GetPrefColorValue(C4P.PrefColor);
						SCopy(LoadResStr("IDS_PLR_NEWCOMMENT"), C4P.Comment, C4MaxComment);
						// Save info core
						C4P.Save(hGroup);
						// Add a portrait
						BYTE *pBytes; unsigned int iSize;
						if (GetPortrait(&pBytes,&iSize)) 
							hGroup.Add(C4CFN_Portrait, pBytes, iSize, FALSE, TRUE);
						// Close group
						hGroup.Close();
						}
					break;
				}
			break;
		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
		}

	// Move temp item to target location
	if (!TransferItem( TempItemPath, pParentViewItem, true, false, (iType==C4IT_Player) ))
		{ EraseItem(TempItemPath); return; }

	// Rename new item
	CString NewPath; NewPath.Format("%s\\%s",ParentPath,GetFilename(TempItemPath));
	if (pViewItem=GetViewItem(NewPath))
		{	SelectItem(pViewItem->hItem);	OnRename();	}

	}

C4ViewItem* C4ViewCtrl::GetParentViewItem(C4ViewItem *pViewItem)
	{
	if (!pViewItem) return NULL;
	return GetParentViewItem(pViewItem->hItem);
	}


/* Load and insert view item (and tree item) to parent view item */

bool C4ViewCtrl::LoadItem(CString &LoadPath, C4ViewItem *pParentViewItem)
	{
	C4Group ParentGroup;
	C4ViewItem LoadItem;
	CString ParentPath;
	CWaitCursor WaitCursor;

	TRACE("Loading item %s...\n\r",LoadPath);

	// Prepare parent
	if (pParentViewItem) 
		{
		// Set parent filename
		ParentPath = pParentViewItem->Path;
		// Set parent flags & remove dummy contents
		pParentViewItem->ContentsLoaded = true;
		RemoveDummyItem(pParentViewItem->hItem);
		}
	else 
		ParentPath = RootFilename;

	// Load item
	if (!ParentGroup.Open(ParentPath))
		{	C4MessageBox("IDS_FAIL_LOAD"); return false; }
	if (LoadItem.Load(ParentGroup,LoadPath,Mode))
		Insert(LoadItem,pParentViewItem);
	ParentGroup.Close();
	// Avoid loss of dynamic data on destruction
	LoadItem.Default(); 

	return true;
	}

C4ViewItem* C4ViewCtrl::GetViewItem(CString &byPath)
	{
	POSITION pos=ViewItemList.GetHeadPosition();
	while (pos)
		{
		C4ViewItem &rViewItem = ViewItemList.GetNext(pos);
		if (rViewItem.Path.CompareNoCase(byPath)==0)
			return &rViewItem;
		}	
	return NULL;
	}

bool C4ViewCtrl::OnReload()
	{
	GetApp()->EditSlots.Clear();
	CString bufRootFilename = RootFilename;
	int bufMode = Mode;
	Clear();
	Default();
	Init(bufRootFilename,bufMode);
	return true;
	}

HTREEITEM C4ViewCtrl::GetInsertPosition(C4ViewItem *pViewItem, HTREEITEM hParentItem)
	{
	HTREEITEM hInsertAfter = NULL, hItem;

	if (hParentItem) hItem = GetChildItem(hParentItem);
	else hItem = GetRootItem();
	
	while (hItem)
		{
		C4ViewItem *pViewItem2 = GetViewItem(hItem);
		if (pViewItem2)
			{
			bool bInsertAfter = false;
			// 1. sort by type
			if(pViewItem2->Type < pViewItem->Type)
				bInsertAfter = true;
			if(pViewItem2->Type > pViewItem->Type)
				break;
			// 2. by folder index
			if (!bInsertAfter)
				if (pViewItem->FolderIndex || pViewItem2->FolderIndex)
				{
					// undefined folder index goes to the end
					if (!pViewItem->FolderIndex) pViewItem->FolderIndex = 10000;
					if (!pViewItem2->FolderIndex) pViewItem2->FolderIndex = 10000;
					if (pViewItem2->FolderIndex < pViewItem->FolderIndex)
						bInsertAfter = true;
					if (pViewItem2->FolderIndex > pViewItem->FolderIndex)
						break;
				}
			// 3. by difficulty
			if (!bInsertAfter)
				if (pViewItem->Difficulty || pViewItem2->Difficulty)
				{
					// undefined difficulty goes to the end
					if (!pViewItem->Difficulty) pViewItem->Difficulty = 10000;
					if (!pViewItem2->Difficulty) pViewItem2->Difficulty = 10000;
					if (pViewItem2->Difficulty < pViewItem->Difficulty)
						bInsertAfter = true;
					if (pViewItem2->Difficulty > pViewItem->Difficulty)
						break;
				}
			// 4. sort by icon (if in range)
			if(!bInsertAfter)
				if(Inside(pViewItem->Icon, C4IT_SortByIcon1, C4IT_SortByIcon2) &&
					 Inside(pViewItem2->Icon, C4IT_SortByIcon1, C4IT_SortByIcon2))
				{
					if(pViewItem2->Icon < pViewItem->Icon)
						bInsertAfter = true;
					if(pViewItem2->Icon > pViewItem->Icon)
						break;
				}
			// 5. by id, experience (ObjectInfo)
			if(!bInsertAfter)
				if (pViewItem->Type == C4IT_ObjectInfo && pViewItem2->Type == C4IT_ObjectInfo)
				{
					if (pViewItem2->id > pViewItem->id)
						bInsertAfter = true;
					if (pViewItem2->id < pViewItem->id)
						break;
				}
			if(!bInsertAfter)
				if (pViewItem->Type == C4IT_ObjectInfo && pViewItem2->Type == C4IT_ObjectInfo)
				{
					if (pViewItem2->Experience > pViewItem->Experience)
						bInsertAfter = true;
					if (pViewItem2->Experience < pViewItem->Experience)
						break;
				}
			// 6. By Title
			if(!bInsertAfter)
			{
				int icmp = pViewItem2->Title.Compare(pViewItem->Title);
				if(icmp < 0)
					bInsertAfter = true;
				if(icmp > 0)
					break;
			}
		}
		hInsertAfter = hItem;
		hItem = GetNextSiblingItem(hItem);
		}

	return hInsertAfter;
	}

bool C4ViewCtrl::OnProperties()
	{
	C4ViewItem *pViewItem = GetViewItem(GetSelectedItem());
	if (!pViewItem || !pViewItem->EnableProperties) return false;

	C4Group ItemGroup;
	CString ItemPath = pViewItem->Path;
	C4PlayerInfoCore C4P;
	C4ScenarioDlg ScenarioDlg;

	switch (pViewItem->Type)
		{
		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
		case C4IT_Scenario:
			// Modify scenario file by scenario prop dlg
			if (!ScenarioDlg.Init( pViewItem->Path, IsOriginal(pViewItem) ))	
				{	C4MessageBox("IDS_FAIL_MODIFY"); return false; }
			// Update modified item
			switch (ScenarioDlg.DoModal())
				{
				case IDCANCEL: break;
				case IDOK: RefreshItem(pViewItem); UpdateActivationState(pViewItem); break;
				case IDOK_AUTOCOPY: 
					LoadItem(ScenarioDlg.Filename,NULL); 
					SelectViewItem(GetViewItem(ScenarioDlg.Filename)); // immediate rename?
					break;
				}
			// Clear concerned edit slots
			GetApp()->EditSlots.ClearItems(ItemPath);
			break;
		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
		}

	// Reset focus
	SetFocus();

	return true;
	}

/* Keep path, contents, and tree item, reload item data only */

bool C4ViewCtrl::RefreshItem(C4ViewItem *pViewItem)
	{
	if (!pViewItem) return false;
	// Refresh item
	pViewItem->Refresh(Mode);
	// Update dialog item info
	UpdateItemInfo(GetSelectedViewItem());
	// Update icons
	SetItemImage(pViewItem->hItem,pViewItem->Icon,pViewItem->Icon);
	// Success
	return true;
	}

bool C4ViewCtrl::RefreshItem(const char *szItemPath)
	{
	return RefreshItem(GetViewItem(szItemPath));
	}

bool C4ViewCtrl::RefreshParentItems(C4ViewItem *pViewItem)
	{
	// Refresh this item
	if (!RefreshItem(pViewItem)) return false;
	// Refresh parent view item
	C4ViewItem *pParent = GetParentViewItem(pViewItem);
	if (pParent)
		if (!RefreshParentItems(pParent))
			return false;
	// Success
	return true;
	}

bool C4ViewCtrl::RefreshChildren(C4ViewItem *pViewItem)
	{
	HTREEITEM hItem;
	// Get first child
	hItem=GetChildItem(pViewItem->hItem);
	// Refresh all children
	while (hItem && GetViewItem(hItem))
		{
		// Refresh this item
		if (!RefreshItem(GetViewItem(hItem))) return false;
		// Refresh children
		if (!RefreshChildren(GetViewItem(hItem))) return false;
		// Get next child
		hItem=GetNextItem(hItem,TVGN_NEXT);
		}
	// Done
	return true;
	}

/* Remove old item, load and insert new item (to old parent view item). 
   Does definition update messages.
	 Refresh parent view item on request.
	 Reselect item if it was selected. */

bool C4ViewCtrl::ReloadItem(CString &rItemPath, bool fRefreshParent, bool fReloadOnly)
	{
	// Get full path
	CString ItemPath; 
	//ItemPath=GetCfg()->AtExePath(rItemPath);
	SetCurrentDirectory(GetCfg()->General.ExePath);
	_fullpath(ItemPath.GetBuffer(_MAX_PATH+1),rItemPath,_MAX_PATH); ItemPath.ReleaseBuffer();
	// Get existing view item
	C4ViewItem *pViewItem = GetViewItem(ItemPath);
	// View item not found and reload only specified: do nothing (don't load new item)
	if (!pViewItem && fReloadOnly) return true;
	// Determine selection state
	bool fWasSelected = (pViewItem==GetSelectedViewItem());
	// Get parent view item by view item or by path
  C4ViewItem *pParentViewItem;
	if (pViewItem) 
		pParentViewItem = GetParentViewItem(pViewItem);
	else
		if (!(pParentViewItem = GetViewItem(NoBackslash(GetPath(ItemPath)))))
			if (!SEqualNoCase(GetPath(ItemPath), GetCfg()->AtExePath("")))
				return true;
	if (pParentViewItem && !pParentViewItem->ContentsLoaded) return true;
	// Remove item
	if (pViewItem) RemoveItem(pViewItem->Path);
	// Load item
	if (!LoadItem(ItemPath,pParentViewItem)) return false;
	// Refresh parent if desired
	if (fRefreshParent) RefreshItem(pParentViewItem);
	// Reselect item
	if (fWasSelected) SelectViewItem(pViewItem);
	// Success
	return true;
	}

void C4ViewCtrl::OnBeginDrag(NMHDR* pNMHDR, LRESULT* pResult) 
	{
	NM_TREEVIEW* pNMTreeView = (NM_TREEVIEW*)pNMHDR;
	HTREEITEM hItem = pNMTreeView->itemNew.hItem;
	C4ViewItem *pViewItem=GetViewItem(hItem);
	*pResult = 0;
	if (!pViewItem) return;

	// Set drag item
	pDragItem = pViewItem;
	// Set cursor
	SetCursor(LoadCursor(NULL,MAKEINTRESOURCE(IDC_ARROW)));
	// Create drag image
	pDragImageList = CreateDragImage(pDragItem->hItem);
	// Begin image dragging
	pDragImageList->DragShowNolock(TRUE);
	pDragImageList->SetDragCursorImage(0,CPoint(0,0));
	pDragImageList->BeginDrag(0,CPoint(0,0));
	pDragImageList->DragMove(CPoint(0,0));
	pDragImageList->DragEnter(this,CPoint(0,0));
	SetCapture();
	// Mode
	fDragCopy = false;
	// Set cursor
	SetCursor(LoadCursor(NULL,MAKEINTRESOURCE(IDC_ARROW)));
	
	}

void C4ViewCtrl::OnMouseMove(UINT nFlags, CPoint point) 
	{

	// Dragging
	if (pDragItem)
		{

		// Scroll top/bottom
		RECT rect; GetClientRect(&rect);
		if (Inside(point.x,rect.left,rect.right) && Inside(point.y,rect.top,rect.bottom))
			{
			if (point.y<20)
				{
				pDragImageList->DragLeave(this);
				SendMessage(WM_VSCROLL,SB_LINEUP,0);
				pDragImageList->DragEnter(this, point);
				}
			if (rect.bottom-point.y<20)
				{
				pDragImageList->DragLeave(this);
				SendMessage(WM_VSCROLL,SB_LINEDOWN,0);
				pDragImageList->DragEnter(this, point);
				}
			}
		// Copy or move?
		if (!(nFlags & MK_SHIFT)) 
			{ if (!fDragCopy) { fDragCopy=true; SetCursor(LoadCursor(AfxGetResourceHandle(),MAKEINTRESOURCE(IDC_CURSORCOPY))); } }
		else 
			{ if (fDragCopy) { fDragCopy=false; SetCursor(LoadCursor(NULL,MAKEINTRESOURCE(IDC_ARROW))); } }
		// Drag the image
		pDragImageList->DragMove(point);
		// Drop target selection
		HTREEITEM hTarget; UINT dwFlags;
		hTarget = HitTest(point,&dwFlags); if (!(dwFlags & TVHT_ONITEM)) hTarget=NULL;
		if (hTarget != GetSelectedItem())
			{
			pDragImageList->DragLeave(this);
			SelectDropTarget(hTarget); SelectItem(hTarget);
			pDragImageList->DragEnter(this, point);
			}
		}
	
	CTreeCtrl::OnMouseMove(nFlags, point);
	}

void C4ViewCtrl::OnLButtonUp(UINT nFlags, CPoint point) 
	{
	HTREEITEM hTarget; UINT dwFlags;
	CTreeCtrl::OnLButtonUp(nFlags, point);

	// Button up after dragging
	C4ViewItem *pTargetViewItem;
	RECT rect; 
	if (pDragItem)
		{
		
		// Clear drag image
		pDragImageList->DragLeave(this);
		pDragImageList->EndDrag();
		delete pDragImageList; pDragImageList = NULL;
		ReleaseCapture();
		SelectDropTarget(NULL);
		
		// Drag and drop definition item to C4Viewport
		POINT pointTarget = point; ClientToScreen(&pointTarget);
		HWND hWndTarget = ::WindowFromPoint(pointTarget);
		char szClassName[1024];	GetClassName(hWndTarget,szClassName,1024);
		if (hWndTarget && SEqual(szClassName,C4ViewportClassName))
			if (pDragItem->id)
				{
				::ScreenToClient(hWndTarget,&pointTarget);
				::SendMessage(hWndTarget,WM_USER_DROPDEF,MAKEWPARAM(pointTarget.x,pointTarget.y),pDragItem->id);
				}

		// Drag and drop action
		GetClientRect(&rect);
		if (Inside(point.x,rect.left,rect.right) && Inside(point.y,rect.top,rect.bottom))
			{
			hTarget = HitTest(point,&dwFlags); if (!(dwFlags & TVHT_ONITEM)) hTarget=NULL;
			pTargetViewItem = GetViewItem(hTarget);
			TransferItem(pDragItem,pTargetViewItem,!fDragCopy);
			}
		
		// Clear drag item
		pDragItem = NULL;
		
		}

	// Button up after click (never gets here unless clicked on no item?)
	else
		{
		// Selection
		hTarget = HitTest(point,&dwFlags); if (!(dwFlags & TVHT_ONITEM)) hTarget=NULL;
		SelectItem(hTarget);
		}

	}

void C4ViewCtrl::OnRButtonUp(UINT nFlags, CPoint point) 
	{
	CTreeCtrl::OnRButtonUp(nFlags, point);
	}
	
bool C4ViewCtrl::RemoveDummyItem(HTREEITEM hFromParent)
	{
	if (GetItemText(GetChildItem(hFromParent))==ContentsDummy)
		DeleteItem(GetChildItem(hFromParent));
	return true;
	}

void C4ViewCtrl::OnRButtonDown(UINT nFlags, CPoint point) 
	{
	// Selection
	HTREEITEM hTarget; UINT dwFlags;
	hTarget = HitTest(point,&dwFlags); if (!(dwFlags & TVHT_ONITEM)) hTarget=NULL;
	SelectItem(hTarget);
	
	ClientToScreen(&point);
	DoContextMenu(point);
	}

/* Copy/move file to target parent view item, overwrite duplicate target item,
   load, insert, and select copied item */

bool C4ViewCtrl::TransferItem(const char *szFilename, C4ViewItem *pTargetParent, 
															bool fMove, bool fConfirm, bool fUnregistered)
	{
	CString sText,sFormat,TransferAs;
	const char* idImportAs = 0;
	// Is source an existing view item?
	C4ViewItem *pSourceViewItem = GetViewItem(szFilename);
	// No unregistered editing
	/*if (!fUnregistered) FREEWARE
		if (!GetCfg()->Registered())
			{ C4MessageBox("IDS_MSG_NOUNREGEDIT"); return false; }*/
	// Target parent must be group
	if (pTargetParent) 
		if (!ItemType[pTargetParent->Type].Expand)
			{ sText.Format(LoadResStr("IDS_MSG_TARGETNOGROUP"),pTargetParent->Title);	C4MessageBox(sText); return false; }
	// Check target parent modification
	if (!ModificationOkay(pTargetParent)) return false;
	// Check source item modification
	if (fMove) if (!ModificationOkay(pSourceViewItem)) return false;
	// Player mode prompt
	if (Mode == C4VC_Player)
		{
		// Check type transfer
		if (pTargetParent)
			{
			// Determine item types
			int iType1 = pSourceViewItem ? pSourceViewItem->Type : FileItemType(szFilename);	
			int iType2 = pTargetParent->Type;
			// Transfer type check
			if (!TransferTypeCheck(iType1, iType2, TransferAs, &idImportAs)) 
				{
				CString sName1 = LoadResStr(ItemType[iType1].Name);
				CString sName2 = LoadResStr(ItemType[iType2].Name);
				sText.Format(LoadResStr("IDS_MSG_TRANSFERNOTYPE"),sName1,sName2);	
				C4MessageBox(sText); 
				return false;
				}
			}
		// Confirm transfer
		if (fConfirm)
			{
			sFormat = LoadResStr(fMove ? "IDS_MSG_PROMPTMOVE" : "IDS_MSG_PROMPTCOPY");
			if (idImportAs) 
				sFormat = LoadResStr("IDS_MSG_PROMPTIMPORT");
			sText.Format(sFormat,GetFullItemTitle(szFilename), GetFullItemTitle(pTargetParent), LoadResStr(idImportAs));	
			if (C4MessageBox(sText,true)!=IDOK) 
				return false;
			}
		}
	// Wait cursor 
	CWaitCursor WaitCursor;
	// Status message
	sFormat = LoadResStr(fMove ? "IDS_STATUS_MOVING" : "IDS_STATUS_COPYING");
	sText.Format(sFormat,GetItemTitle(szFilename),GetItemTitle(pTargetParent));	SetStatus(sText);
	// Target path
	CString TargetPath = RootFilename; 
	if (pTargetParent) TargetPath = pTargetParent->Path; 
	TargetPath += "\\";
	// Standard target filename (source filename at target path
	CString TargetFilename = TargetPath + GetFilename(szFilename);
	// TransferTypeCheck specification overrides target filename
	if (!TransferAs.IsEmpty()) 
		TargetFilename = TargetPath + TransferAs;

	// Special case: portrait import into a crew member
	if (TransferAs == C4CFN_Portrait)
		if (pTargetParent && (pTargetParent->Type == C4IT_ObjectInfo))
		{
			// Clear PictureFile entry in ObjectInfoCore
			C4Group ItemGroup;
			C4ObjectInfoCore C4I;
			if (ItemGroup.Open(pTargetParent->Path) && C4I.Load(ItemGroup))	
			{
				C4I.PortraitFile[0] = 0;
				if (!C4I.Save(ItemGroup, NULL) || !ItemGroup.Close()) 
					C4MessageBox("IDS_FAIL_MODIFY");
			}
			else
				C4MessageBox("IDS_FAIL_MODIFY");
		}

	// Check identical
	if (ItemIdentical(szFilename, TargetFilename)) return true;
	// Check recursive copy
	if (SEqualNoCase(szFilename, TargetPath.Left(TargetPath.GetLength() - 1)))
		{	C4MessageBox("IDS_FAIL_RECURSIVE"); return false; }
	// Check target item (overwrite) modification (matters for targets in root only)
	if (!ModificationOkay(GetViewItem(TargetFilename))) return false;
	// Expand target parent item
	if (pTargetParent) if (!pTargetParent->ContentsLoaded) LoadItems(pTargetParent);
	// Move item
	if (fMove)
		{
		if (!C4Group_MoveItem(szFilename, TargetFilename, true)) {	C4MessageBox("IDS_FAIL_MOVE"); return false; }
		TRACE("%s moved to %s\n\r",GetFilename(szFilename),TargetPath);
		}
	// Copy item
	else
		{
		if (!C4Group_CopyItem(szFilename, TargetFilename, true))	{	C4MessageBox("IDS_FAIL_COPY"); return false; }
		TRACE("%s copied to %s\n\r",GetFilename(szFilename),TargetPath);
		// De-original copied item
		if (pSourceViewItem && pSourceViewItem->GroupOriginal) C4Group_SetOriginal(TargetFilename,FALSE);
		}
	// Overwrite double view item
	RemoveItem(TargetFilename,true);
	// Move: remove original item
	if (fMove) RemoveItem(szFilename,true);
	// Load copied item
	LoadItem(TargetFilename,pTargetParent);
	// Refresh target parent item
	RefreshItem(pTargetParent);
	// Select copied item
	C4ViewItem *pViewItem;
	if (pViewItem=GetViewItem(TargetFilename)) SelectItem(pViewItem->hItem);
	// Was a registration key: check registration
	if (FileItemType(szFilename) == C4IT_KeyFile)
		((C4ExplorerDlg*)GetApp()->m_pMainWnd)->CheckRegistration();
	// Success
	return true;
	}

void C4ViewCtrl::RemoveItem(CString byPath, bool fRemoveEditSlot)
	{
	// Get view item
  C4ViewItem *pViewItem = GetViewItem(byPath);
	// Delete tree item
	if (pViewItem) DeleteItem(pViewItem->hItem);
	// Clear view items
	ClearViewItems(byPath);
	// Remove edit slot
	if (fRemoveEditSlot) GetApp()->EditSlots.RemoveItem(byPath);
	}

void C4ViewCtrl::OnDropFiles( HDROP hDropInfo )
	{
	
	// Get target
	HTREEITEM hTarget; UINT dwFlags;	CPoint Point;
	DragQueryPoint(hDropInfo, &Point);
	C4ViewItem *pTargetViewItem;
	hTarget = HitTest(Point,&dwFlags); if (!(dwFlags & TVHT_ONITEM)) hTarget=NULL;
	pTargetViewItem = GetViewItem(hTarget);

	// Get filenames
	char szFilenames[100*_MAX_PATH+1]="";
	int cnt,iFileCount;
	iFileCount=DragQueryFile(hDropInfo,0xFFFFFFFF,NULL,0);
	for (cnt=0; cnt<iFileCount; cnt++)
		{
		SNewSegment(szFilenames);
		DragQueryFile(hDropInfo,cnt,szFilenames+SLen(szFilenames),100*_MAX_PATH-SLen(szFilenames));
		}

	// Copy items
	char szFilename[_MAX_PATH+1];
	for (cnt=0; SCopySegment(szFilenames,cnt,szFilename,';'); cnt++)
		if (!TransferItem(szFilename, pTargetViewItem, false, true, SEqualNoCase(GetExtension(szFilename), "c4k")))
			break;

	}

bool C4ViewCtrl::OnPack()
	{
	// Get and check view item
	C4ViewItem *pViewItem = GetViewItem(GetSelectedItem());
	if (!pViewItem || !pViewItem->EnablePack) return false;

	// Check modification access
	if (!ModificationOkay(pViewItem)) return false;

	// Pack / unpack
	if (pViewItem->GroupStatus==GRPF_File) return UnpackItem(pViewItem);
	if (pViewItem->GroupStatus==GRPF_Folder) return PackItem(pViewItem);
	
	return false;
	}

int C4ViewCtrl::OnCreate(LPCREATESTRUCT lpCreateStruct) 
	{
	if (CTreeCtrl::OnCreate(lpCreateStruct) == -1) return -1;
	return 0;
	}

int C4ViewCtrl::AddIcon(HBITMAP hIcon)
	{
	return ImageList.Add(CBitmap::FromHandle(hIcon), TRANSPCOLOR);
	}

CString C4ViewCtrl::GetItemTitle(C4ViewItem *pViewItem)
	{
	CString sResult;
	if (pViewItem) sResult = pViewItem->Title;
	else sResult = LoadResStr("IDS_FN_ROOT");
	return sResult;
	}

CString C4ViewCtrl::GetItemTitle(const char *szFilename)
	{
	C4ViewItem *pViewItem;
	if (pViewItem = GetViewItem(szFilename)) 
		return GetItemTitle(pViewItem);
	return GetFilename(szFilename);
	}

C4ViewItem* C4ViewCtrl::GetViewItem(const char *szByPath)
	{
	POSITION pos=ViewItemList.GetHeadPosition();
	while (pos)
		{
		C4ViewItem &rViewItem = ViewItemList.GetNext(pos);
		if (SEqualNoCase(rViewItem.Path,szByPath))
			return &rViewItem;
		}	
	return NULL;
	}

CString C4ViewCtrl::GetFullItemTitle(C4ViewItem *pViewItem)
	{
	CString sResult,sBuf;
	if (pViewItem) 
		{
		sResult = pViewItem->Title;
		for (C4ViewItem *pParentViewItem=GetParentViewItem(pViewItem); pParentViewItem; pParentViewItem=GetParentViewItem(pParentViewItem))
			{ sBuf.Format("%s\\%s",GetItemTitle(pParentViewItem),sResult); sResult=sBuf; }
		}
	else sResult = LoadResStr("IDS_FN_ROOT");
	return sResult;
	}

CString C4ViewCtrl::GetFullItemTitle(const char *szFilename)
	{
	C4ViewItem *pViewItem;
	if (pViewItem = GetViewItem(szFilename)) 
		return GetFullItemTitle(pViewItem);
	return szFilename;
	}


bool C4ViewCtrl::TransferItem(C4ViewItem *pViewItem, C4ViewItem *pTargetItem, bool fMove)
	{
	if (!pViewItem) return false;
	return TransferItem(pViewItem->Path,pTargetItem,fMove);
	}

bool C4ViewCtrl::TransferTypeCheck(int iSource, int iTarget, CString &ImportAs, const char **idImportName)
	{
	switch (iSource)
		{
		case C4IT_Player:
			if ((iTarget==C4IT_WorkingDirectory) || (iTarget==C4IT_Directory)) return true;
			break;
		case C4IT_ObjectInfo:
			if (iTarget==C4IT_Player) return true;
			break;
		case C4IT_Scenario:
			if ((iTarget==C4IT_WorkingDirectory) || (iTarget==C4IT_Directory) || (iTarget==C4IT_Folder)) return true;
			break;
		case C4IT_Folder:
			if ((iTarget==C4IT_WorkingDirectory) || (iTarget==C4IT_Directory) || (iTarget==C4IT_Folder)) return true;
			break;
		case C4IT_Directory:
			if ((iTarget==C4IT_WorkingDirectory) || (iTarget==C4IT_Directory)) return true;
			break;
		case C4IT_DefFolder:
			if ((iTarget==C4IT_WorkingDirectory) || (iTarget==C4IT_Directory) || (iTarget==C4IT_Definition) || (iTarget==C4IT_Scenario) || (iTarget==C4IT_Folder)) return true;
			break;
		case C4IT_Definition:
			if ((iTarget==C4IT_WorkingDirectory) || (iTarget==C4IT_Directory) || (iTarget==C4IT_Definition) || (iTarget==C4IT_Scenario) || (iTarget==C4IT_Folder)) return true;
			break;
		case C4IT_Bitmap:
			if ((iTarget==C4IT_Player) || (iTarget==C4IT_ObjectInfo))
				{ ImportAs=C4CFN_Portrait_Old; *idImportName = "IDS_TYPE_PORTRAIT"; return true;}
			break;
		case C4IT_PNG:
			if ((iTarget==C4IT_Player) || (iTarget==C4IT_ObjectInfo))
				{ ImportAs=C4CFN_Portrait; *idImportName = "IDS_TYPE_PORTRAIT"; return true;}
			break;
		}
	return false;
	}

void C4ViewCtrl::DoContextMenu(CPoint point) 
	{

	CString sText;
	// Get selected item
	C4ViewItem *pViewItem = GetSelectedViewItem();

	UINT uDisable = 0; if (!pViewItem) uDisable = MF_GRAYED;	
	UINT uActivate = MF_GRAYED; if (pViewItem && pViewItem->EnableActivation) uActivate = 0;
	UINT uProps = MF_GRAYED; if (pViewItem && pViewItem->EnableProperties) uProps = 0;
	UINT uPack = MF_GRAYED; if (pViewItem && pViewItem->EnablePack) uPack = 0;
	UINT uStart = MF_GRAYED; if (pViewItem && (pViewItem->Type==C4IT_Scenario)) uStart = 0;
	UINT uReload = MF_GRAYED; if(pViewItem) uReload = 0;
	UINT uVirtual = MF_GRAYED; if(pViewItem) uVirtual = 0;
	UINT uNew = 0;
	// Create menu
	CMenu Menu;
	Menu.CreatePopupMenu();
	// New
	Menu.AppendMenu(MF_STRING | uNew,1,LoadResStr("IDS_BTN_NEW"));
	// Activate
	Menu.AppendMenu(MF_STRING | uDisable | uActivate,2,LoadResStr((pViewItem && pViewItem->Activated) ? "IDS_BTN_DEACTIVATE" : "IDS_BTN_ACTIVATE"));
	// Rename
	Menu.AppendMenu(MF_STRING | uDisable | uVirtual,3,LoadResStr("IDS_BTN_RENAME"));
	// Delete
	Menu.AppendMenu(MF_STRING | uDisable | uVirtual,4,LoadResStr("IDS_BTN_DELETE"));
	// Duplicate
	if (Mode == C4VC_Developer) 
		Menu.AppendMenu(MF_STRING | uDisable | uVirtual,9,LoadResStr("IDS_BTN_DUPLICATE"));
	// Reload
	Menu.AppendMenu(MF_STRING | uReload,12,LoadResStr("IDS_BTN_RELOAD"));	
	// Properties
	Menu.AppendMenu(MF_SEPARATOR,0,"");
	Menu.AppendMenu(MF_STRING | uDisable | uProps,5,LoadResStr("IDS_BTN_PROPERTIES"));
	if (pViewItem && pViewItem->Type==C4IT_Scenario)
		Menu.AppendMenu(MF_STRING, 8, LoadResStr("IDS_BTN_PRESETS"));
	// Pack
	if (Mode == C4VC_Developer) 
		{
		Menu.AppendMenu(MF_SEPARATOR,0,"");
		Menu.AppendMenu(MF_STRING | uDisable | uPack,6,LoadResStr((pViewItem && pViewItem->GroupStatus==GRPF_File) ? "IDS_BTN_UNPACK" : "IDS_BTN_PACK"));
		Menu.AppendMenu(MF_STRING | uDisable | uPack,11,LoadResStr("IDS_BTN_EXPLODE"));
		}
	// Start / join
	Menu.AppendMenu(MF_SEPARATOR,0,"");
	// (Send network message)
	if (pViewItem && pViewItem->EnableJoin)
		Menu.AppendMenu(MF_STRING,10,LoadResStr("IDS_BTN_MESSAGE"));
	
	const char *szStartText = "IDS_BTN_START";
	if(pViewItem && pViewItem->EnableJoin) szStartText = "IDS_BTN_JOIN";
	//if(pViewItem && pViewItem->pWebLibEntry) szStartText = "IDS_BTN_DL";
	Menu.AppendMenu(MF_STRING | uDisable | uStart,7,LoadResStr(szStartText));
	// Execute menu
	int MenuID = Menu.TrackPopupMenu( TPM_NONOTIFY | TPM_RETURNCMD | TPM_RIGHTBUTTON | TPM_LEFTBUTTON, point.x, point.y, this);
	Menu.DestroyMenu();
	// Command to list control
	switch (MenuID)
		{
		case 1: OnNew(); break;
		case 2: OnActivate(); break;
		case 3: OnRename(); break;
		case 4: OnDelete(); break;
		case 5: OnProperties(); break;
		case 6: OnPack(); break;
		case 7: OnStart(); break;
		case 8: OnDefinitions(); break;
		case 9: OnDuplicate(); break;
		//case 10: OnSendHostMessage(); break;
		case 11: OnExplode(); break;
		case 12: OnItemReload(); break;
		}
	}


bool C4ViewCtrl::IsOriginal(C4ViewItem *pViewItem)
	{
	// Is item or any parent ist original?
	while (pViewItem)
		{
		if (pViewItem->GroupOriginal) return true;
		pViewItem = GetParentViewItem(pViewItem);
		}
	return false;
	}

bool C4ViewCtrl::ModificationOkay(C4ViewItem *pViewItem, bool fDelete)
	{
	CString sText; 
	if (!pViewItem) return true;
	// contents of weblib -> never
	//if(pViewItem->pWebLibEntry) return false;
	// RedWolf Design may always
	if (SEqual(GetCfg()->General.Name, C4CFG_Company))
		if (GetCfg()->Registered())
			return true;
	// Okay if developer mode and safety is disabled
	if (Mode==C4VC_Developer)
		if (!GetCfg()->Explorer.Kindersicherung)
			return true;
	// Network reference: not allowed
	if (pViewItem->EnableJoin)
		{
		C4MessageBox("IDS_MSG_NOEDITNETREF"); return false;
		}
	// Original item
	if (IsOriginal(pViewItem))
		{
		// Delete and in non-original parent or root
		if (fDelete && !IsOriginal(GetParentViewItem(pViewItem)))
			{
			return true;
			}
		// Other action: not allowed
		else
			{
			sText.Format(LoadResStr("IDS_MSG_ITEMORIGINAL"),GetItemTitle(pViewItem));
			C4MessageBox(sText); return false;
			}
		}
	// Okay
	return true;
	}

void C4ViewCtrl::OnLButtonDown(UINT nFlags, CPoint point) 
	{
	CTreeCtrl::OnLButtonDown(nFlags, point);

	// Activation (by state image click)
	RECT rect; HTREEITEM hTarget; UINT dwFlags;
	hTarget = HitTest(point,&dwFlags); if (!(dwFlags & TVHT_ONITEM)) hTarget=NULL;
		if (hTarget)
			if (GetItemRect(hTarget,&rect,TRUE)) // Text only
				if (Inside<int>( point.x-(rect.left-32), 0, 16 ))
					if (Inside<int>( point.y-rect.top, 0, 16 ))
						OnActivate();
	
	}

bool C4ViewCtrl::SelectViewItem(C4ViewItem *pViewItem)
	{
	if (pViewItem)
		if (SelectItem(pViewItem->hItem))
			return true;
	return false;
	}

void C4ViewCtrl::OnStart()
	{
	if (IsWindow(m_hWnd))
		((C4ExplorerDlg*)GetParent())->OnStart();
	}

void C4ViewCtrl::OnDefinitions()
	{
	C4ViewItem *pViewItem = GetViewItem(GetSelectedItem());
	if (!pViewItem || (pViewItem->Type!=C4IT_Scenario)) return;
	
	C4ScenarioDefinitionsDlg ScenarioDefinitionsDlg;
	if (!ScenarioDefinitionsDlg.Init( pViewItem->Path, IsOriginal(pViewItem), (Mode==C4VC_Developer) ))	
		{	C4MessageBox("IDS_FAIL_MODIFY"); return; }

	if (ScenarioDefinitionsDlg.DoModal()==IDOK)
		{
		CString sPath = pViewItem->Path;
		// Reload item
		ReloadItem(sPath);
		// Reselect item
		if (pViewItem=GetViewItem(sPath))	SelectItem(pViewItem->hItem);
		}
	
	}

void C4ViewCtrl::OnOK()
	{
	C4ViewItem *pViewItem = GetViewItem(GetSelectedItem());
	if (!pViewItem) return;

	// Edit item
	if (ItemType[pViewItem->Type].Edit) 
		{ OnEdit(); return; }

	// Start item (Scenario or Weblib Entry)
	if ((pViewItem->Type == C4IT_Scenario)  /*|| pViewItem->pWebLibEntry*/)
		{ OnStart(); return; }

	}

LRESULT C4ViewCtrl::OnInit(WPARAM wParam, LPARAM lParam)
	{
	CString InitRoot = (const char*) lParam;
	Clear();
	Default();
	Init( InitRoot, (int) wParam );
	return true;	
	}

/* Reload multiple items */

bool C4ViewCtrl::ReloadItems(const char *szItemList, bool fRefreshParents, bool fReloadOnly)
	{
	// nothing to reload
	if (!szItemList || !*szItemList) 
		return true;
	CString sItem;
	for (int cnt=0; SGetModule(szItemList,cnt,sItem.GetBuffer(_MAX_PATH+1),_MAX_PATH); cnt++)
		{ 
		sItem.ReleaseBuffer(); 
		ReloadItem(sItem, fRefreshParents, fReloadOnly); 
		}
	return true;
	}
	
void C4ViewCtrl::SelectFirstItem()
	{
	SelectItem( GetNextItem(NULL,0) );
	}

void C4ViewCtrl::OnDuplicate()
	{
	C4ViewItem *pViewItem = GetViewItem(GetSelectedItem());
	if (!pViewItem) return;

	// Determine target location
	CString ParentPath = RootFilename;
	CString SourcePath = pViewItem->Path;
	C4ViewItem *pParentViewItem = GetParentViewItem(pViewItem);
	if (pParentViewItem) ParentPath = pParentViewItem->Path;

	// Determine target filename
	char NewFilename[_MAX_PATH+1]; SCopy(pViewItem->Filename,NewFilename);
	char szNumber[10];
	C4Group hParent; 
	if (!hParent.Open(ParentPath)) return;
	for (int iIndex=2; hParent.FindEntry(NewFilename); iIndex++)
		{
		SCopy(pViewItem->Filename,NewFilename); 
		sprintf(szNumber,"%d",iIndex);
		if (SCharLastPos('.',NewFilename)!=-1)
			SInsert(NewFilename,szNumber,SCharLastPos('.',NewFilename));
		else
			SAppend(szNumber,NewFilename);
		}
	hParent.Close();

	// Create copy in temp directory
	CString TempItemPath = GetCfg()->AtTempPath(NewFilename);
	if (!C4Group_CopyItem(pViewItem->Path, TempItemPath, true))
		{ C4MessageBox("IDS_FAIL_MODIFY"); return; }

	// Clear original flag to allow modifications
	C4Group_SetOriginal(TempItemPath, FALSE);

	// Move temp item to target location
	if (!TransferItem(TempItemPath, pParentViewItem, true))
		{ EraseItem(TempItemPath); C4MessageBox("IDS_FAIL_MODIFY"); return; }

	// dublicate au entries
	CString NewPath; NewPath.Format("%s\\%s",ParentPath,GetFilename(TempItemPath));

	// Rename new item
	if (pViewItem=GetViewItem(NewPath))
		{	SelectItem(pViewItem->hItem);	OnRename();	}

	}

void C4ViewCtrl::UpdateActivationState(C4ViewItem *pViewItem)
	{
	
	bool fByScenario=false;
	StdStrBuf szDefs;

	// Get preset specifications from selected scenario
	if (pViewItem && (pViewItem->Type==C4IT_Scenario))
		{
		pViewItem->Definitions.GetModules(&szDefs);
		if (pViewItem->Definitions.LocalOnly || szDefs) fByScenario = true;
		if (pViewItem->Definitions.LocalOnly) szDefs.Clear();
		}

	// Update states
	ActivationByScenario=fByScenario;
	if (fByScenario) SetActivationState(szDefs.getData(), true);
	else SetActivationState(GetCfg()->Explorer.Definitions, false);

	}

/* Quick note on terminology:
    Activated engine : Engine that is marked by user as preferred.
		                   There can be many Activated engines.
		                   Marked by green background in checkbox.
	  Used      engine : Engine that will be used to play Clonk.
		                   There can be only one Used engine at a time.
											 Marked by checkmark in checkbox.
	 An Activated engine has the "Active" data member set to TRUE.
	 The global variable pUsedEngine always points to the current Used engine (if any).
	 Sorry for the confusion.
*/
/* Quick note on Activated engines:
	 An Activated engine is "preferred", in that the closest matching Activated
	  engine will be used to play Clonk. Closest matching means that if the scenario
		requires version 4.62, and engines 4.63 and 4.65 are present, then 4.63 will be used
		if it is Activated, and 4.65 will be used if it is not activated.
	Sorry for the confusion.
*/
void C4ViewCtrl::UpdateUsedEngine(C4ViewItem *pViewItem, bool fNoCheck)
	{
	return; // currently, there is no engine selection through view item. the engine is simply entered into the command line

	// Un-Check current pUsedEngine
	if (pUsedEngine)
		{
		C4ViewItem *pOldUsedEngine = pUsedEngine;
		pUsedEngine = NULL;
		SetItemState(pOldUsedEngine->hItem,INDEXTOSTATEIMAGEMASK(pOldUsedEngine->GetState(false)),TVIS_STATEIMAGEMASK);
		}

	// Not a scenario?
	if (!fNoCheck && (!pViewItem || pViewItem->Type != C4IT_Scenario))
		return;

	C4ViewItem *pEngNonSel = NULL;
	C4ViewItem *pEngSel    = NULL;

	C4ViewItem *pEng;

	/* Iterate through engines. Determine best matching activated (=preferred) engine
	   and best matching non-activated (=not preferred) engine. */
	POSITION pos=ViewItemList.GetHeadPosition();
	while (pos)
		if (pEng = &ViewItemList.GetNext(pos))
			if (pEng->Type == C4IT_Engine)
			{
				// Scenario has an engine specification: priority matching by specified engine
				if (pViewItem && !pViewItem->Engine.IsEmpty())
				{
					if (GetCfg()->AtExeRelativePath(pEng->Path) == pViewItem->Engine)
						pEngSel = pEng;
				}
				// Normal matching by version and preference/activation
				else
				{
					if (fNoCheck || !pViewItem || pViewItem->EngineVersionOk(pEng))
						if (      pEng->Activated && (!pEngSel    || pEngSel->EngineVersionLess(pEng)))
							pEngSel = pEng;
						else if (!pEng->Activated && (!pEngNonSel || pEngNonSel->EngineVersionGreater(pEng)))
							pEngNonSel = pEng;
				}
			}

	// Determine Used engine
	pUsedEngine = pEngSel ? pEngSel : pEngNonSel;
	// Check-mark Used engine
	if (pUsedEngine)
		SetItemState(pUsedEngine->hItem,INDEXTOSTATEIMAGEMASK(pUsedEngine->GetState(false)),TVIS_STATEIMAGEMASK);
	}

void C4ViewCtrl::SetActivationState(const char *szDefinitions, bool fFixed)
	{
	C4ViewItem *pViewItem;
	POSITION pos=ViewItemList.GetHeadPosition();
	while (pos)
		if (pViewItem = &ViewItemList.GetNext(pos))
			if (pViewItem->Type==C4IT_Definition)
				if (pViewItem->EnableActivation)
					{
					// Check activation 
					pViewItem->Activated = false;
					if (SIsModule(szDefinitions,GetCfg()->AtExeRelativePath(pViewItem->Path)))
						pViewItem->Activated = true;
					if (SIsModule(szDefinitions,pViewItem->Path))
						pViewItem->Activated = true;
					// Set state
					SetItemState(pViewItem->hItem,INDEXTOSTATEIMAGEMASK(pViewItem->GetState(fFixed)),TVIS_STATEIMAGEMASK);					
					}	
	}

bool C4ViewCtrl::SelectViewItem(CString &rByPath)
	{
	return SelectViewItem(GetViewItem(rByPath));
	}

void C4ViewCtrl::OnSysKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags) 
	{
	
	switch (nChar)
		{
		case VK_RETURN: OnProperties(); return;
		}
	
	// Base class call
	CTreeCtrl::OnSysKeyDown(nChar, nRepCnt, nFlags);
	
	}

bool C4ViewCtrl::OnExplode()
	{
	// Get and check view item
	C4ViewItem *pViewItem = GetViewItem(GetSelectedItem());
	if (!pViewItem || !pViewItem->EnablePack) return false;

	// Check modification access
	if (!ModificationOkay(pViewItem)) return false;

	// Explode
	return ExplodeItem(pViewItem);
	}

bool C4ViewCtrl::PackItem(C4ViewItem *pViewItem)
	{
	CWaitCursor WaitCursor;

	// Set process routing to refresh items
	TRACE("Packing %s...\n\r",pViewItem->Path);
	GetApp()->SetProcessRouting(C4EX_ProcessRouting_Status,LoadResStr("IDS_STATUS_PACK"));

	// Pack directory
	if (!C4Group_PackDirectory(pViewItem->Path))
		{ C4MessageBox(LoadResStr("IDS_FAIL_PACK")); return false; }

	// Reload item
	if (!ReloadItem(pViewItem->Path)) 
		{ C4MessageBox(LoadResStr("IDS_FAIL_UPDATE")); return false; }

	// Set process routing
	GetApp()->SetProcessRouting(C4EX_ProcessRouting_None);

	// Success
	return true;
	}

bool C4ViewCtrl::ExplodeItem(C4ViewItem *pViewItem)
	{
	CWaitCursor WaitCursor;

	// Set process routing to refresh items
	TRACE("Exploding %s...\n\r",pViewItem->Path);
	GetApp()->SetProcessRouting(C4EX_ProcessRouting_Status,LoadResStr("IDS_STATUS_UNPACK"));

	// Explode directory
	if (!C4Group_ExplodeDirectory(pViewItem->Path))
		{ C4MessageBox(LoadResStr("IDS_FAIL_UNPACK")); return false; }

	// Set process routing
	GetApp()->SetProcessRouting(C4EX_ProcessRouting_None);

	// Refresh item, parents & children
	if (!RefreshItem(pViewItem)) 
		{ C4MessageBox(LoadResStr("IDS_FAIL_UPDATE")); return false; }
	if (!RefreshParentItems(pViewItem)) 
		{ C4MessageBox(LoadResStr("IDS_FAIL_UPDATE")); return false; }
	if (!RefreshChildren(pViewItem)) 
		{ C4MessageBox(LoadResStr("IDS_FAIL_UPDATE")); return false; }

	// Success
	return true;
	}

bool C4ViewCtrl::UnpackItem(C4ViewItem *pViewItem)
	{
	CWaitCursor WaitCursor;

	// Set process routing to refresh items
	TRACE("Unpacking %s...\n\r",pViewItem->Path);
	GetApp()->SetProcessRouting(C4EX_ProcessRouting_Status,LoadResStr("IDS_STATUS_UNPACK"));

	// Unpack directory
	if (!C4Group_UnpackDirectory(pViewItem->Path))
		{ C4MessageBox(LoadResStr("IDS_FAIL_UNPACK")); return false; }

	// Set process routing
	GetApp()->SetProcessRouting(C4EX_ProcessRouting_None);

	// Refresh item & parents
	if (!RefreshItem(pViewItem)) 
		{ C4MessageBox(LoadResStr("IDS_FAIL_UPDATE")); return false; }
	if (!RefreshParentItems(pViewItem)) 
		{ C4MessageBox(LoadResStr("IDS_FAIL_UPDATE")); return false; }

	// Success
	return true;
	}

bool C4ViewCtrl::OnLaunch()
	{
	// Check selected item specified editor
	C4ViewItem *pViewItem = GetViewItem(GetSelectedItem());
	if (!pViewItem || (ItemType[pViewItem->Type].Edit!=C4ITE_ShellLaunch)) return false;
	// Create process
	HANDLE hProcess = ShellExecute( GetApp()->m_pMainWnd->m_hWnd, "open", pViewItem->Path, NULL, GetPath(pViewItem->Path), SW_SHOW );
	if (!hProcess) return false;
	// Reset focus
	SetFocus();
	// Return
	return true;
	}

void C4ViewCtrl::OnItemReload()
{
	C4ViewItem *pViewItem = GetViewItem(GetSelectedItem());
	ReloadItem(pViewItem->Path);
}

void C4ViewCtrl::OnSwitchToEngine()
{
	if (IsWindow(m_hWnd))
		((C4ExplorerDlg*)GetParent())->OnSwitchToEngine();
}

void C4ViewCtrl::OnContextMenu()
{
	C4ViewItem *pViewItem = GetViewItem(GetSelectedItem());	
	RECT rect; CPoint point;
	// Item selected
	if (pViewItem)
		GetItemRect(pViewItem->hItem, &rect, true);
	// No item selected
	else
		GetClientRect(&rect);
	// Position menu
	GetParent()->ClientToScreen(&rect);
	point.x = rect.left + (rect.right - rect.left) / 2;
	point.y = rect.top + (rect.bottom - rect.top) / 2;
	// Do menu
	DoContextMenu(point);
}
