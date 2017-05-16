/* Copyright (C) 1998-2000  Matthes Bender  RedWolf Design */

/* Item to represent loaded file data in the main view */

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
#include "C4Update.h"
#include "../res/resource.h"

#include "FileVersion.h"

#include <sys/stat.h>

extern CImageList *pIconList; // Hack

BOOL Updating_ApplyQuery;
BOOL Updating_ApplyAll;
BOOL Updating_DeleteQuery;
BOOL Updating_DeleteAll;

C4ViewItem::C4ViewItem()
	{
	Default();
	}

C4ViewItem::~C4ViewItem()
	{
	Clear();
	}

void C4ViewItem::Default()
	{
	Type=C4IT_Unknown;
	Icon=0;
	Activated=0;
	EnableActivation=false;
	EnableProperties=false;
	EnableJoin=false;
	EnablePack=false;
	EnableModify=true;
	LockActivation=false;
	C4XVer1=C4XVer2=C4XVer3=C4XVer4=0;
	Path.Empty(); Filename.Empty(); 
	Title.Empty();
	Author.Empty();
	Version.Empty();
	ShowAuthor.Empty();
	Creation=0;
	id=C4ID_None;
	Experience=0;
	GroupStatus=GRPF_Inactive;
	GroupOriginal=0;
	hItem=NULL;
	ContentsLoaded=false;
	Desc.Default();
	DescText=false;
	MinPlayer=MaxPlayer=0;
	UnregisteredAccess=SaveGame=EnableReplay=false;
	NoRuntimeJoin=0;
	Picture.Default();
	DeveloperMode=0;
	Definitions.Default();
	PNGPicture.Clear(false);
	EnableColor = false;
	ColorByParent = false;
	dwColor = 0x00;
	Melee = false;
	Difficulty = 0;
	FolderIndex = 0;
	}

void C4ViewItem::Clear()
	{
	Desc.Clear();
	Picture.Clear();
	PNGPicture.Clear(true);
	}

bool C4ViewItem::Load(C4Group &ParentGroup, CString &rPath,	int Mode)
	{
	
	// Set path & filename
	Path = rPath;
	Filename = GetFilename(Path);

  // check if path is too long
  if(Path.GetLength() > _MAX_PATH)
    return false;

	// Check item existence
	// EVIL_C4GROUP_FINDENTRY_BUG
#if 1
	BOOL exists = ParentGroup.GetStatus()==GRPF_Folder
		? (Filename != "." && Filename != ".." && FileExists(Path))
		: ParentGroup.FindEntry(Filename);
	if (!exists)
		return false;
#else
	if (!ParentGroup.FindEntry(Filename))
		return false;
#endif

	Type = FileItemType(Filename);

	// Open child group
	bool fItemGroup = false;
	C4Group ItemGroup;
	if (ItemGroup.OpenAsChild(&ParentGroup,Filename))
		{
		fItemGroup = true;
		GroupStatus = ItemGroup.GetStatus(); 
		GroupOriginal = ItemGroup.GetOriginal();

		if (Type==C4IT_Unknown)
			Type = C4IT_Directory;
		}

	// Special: load bitmaps in player mode if located in screenshots folder
	bool fLoadAnyway = false;
	if (Mode == C4VC_Player)
		if (Inside(Type, C4IT_Bitmap, C4IT_PNG))
			if (SEqual(GetFilename(ParentGroup.GetName()), GetCfg()->General.ScreenshotFolder.getData()))
				fLoadAnyway = true;

	// Ignore this type in this mode
	if ((Mode==C4VC_Player) && !ItemType[Type].PlayerView)
		if (!fLoadAnyway)
			return false;

	// Author & creation
	if (fItemGroup) 
		{ Author = ItemGroup.GetMaker(); Creation = ItemGroup.GetCreation(); }
	if (Author=="Open directory") 
		{ 
		Author=LoadResStr("IDS_CTL_OPENDIRECTORY"); 
		if (ItemExists(Path+"\\cvs")) Author+=" (CVS)"; 
		if (ItemExists(Path+"\\.svn")) Author+=" (SVN)"; 
		}
	if (Author=="New C4Group") 
		Author.Empty();
	ShowAuthor.Empty();

	// Info
	LoadInfo(ItemGroup,Mode,ParentGroup);
	
	return true;
	}

bool C4ViewItem::LoadDesc(C4Group &ParentGroup, C4Group &ItemGroup)
	{
	CString DescFilename;
	C4ComponentHost TempHost;
	switch (Type)
		{
		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
		case C4IT_Scenario: case C4IT_Folder: 
		case C4IT_Group: case C4IT_Directory:
		case C4IT_WebLibLink:	case C4IT_Definition:

			// Load desc for currently selected language
			if (Desc.LoadEx("Desc", ItemGroup, C4CFN_ScenarioDesc, GetCfg()->General.LanguageEx))
				break;

			// No desc found: try loading desc for any language
			if (Desc.Load("Desc", ItemGroup, C4CFN_ScenarioDesc, "*"))
				break;

			// fallthru: definitions only
			if(Type != C4IT_Definition)
				break;

		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
		if (TempHost.LoadEx("Temp", ItemGroup, C4CFN_DefDesc, GetCfg()->General.LanguageEx))
			Desc.Set(MakeRTF(Title, TempHost.GetData()));
			break;

		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
		case C4IT_RichText:
			Desc.Load("RichText", ParentGroup, Filename);
			break;
		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
		case C4IT_Text: case C4IT_Log: 
		case C4IT_Script: case C4IT_Material:
			// Maximum limit for text (conversion prob)
			if (ParentGroup.EntrySize(Filename)>10000) break;
			// Load text to buffer
			if(TempHost.Load("Temp", ParentGroup, Filename))
				{
				Desc.Set(MakeRTF(NULL, TempHost.GetData()));
				}
			DescText=true;
			break;
		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
		case C4IT_Engine:
			CString strDesc = LoadResStr("IDS_DESC_ENGINE");
			Desc.Set(MakeRTF(Title, strDesc));
			// get engine file info
			/*CFileVersion VerInfo(Path);
			// show name
			bpDescBuf = new unsigned char [101];
			sprintf((char*) bpDescBuf, "Version: %s", VerInfo.GetFileVersion());
			Desc.Set(MakeRTF(VerInfo.GetFileDescription(), (char*) bpDescBuf));
			delete[] bpDescBuf;*/
			break;
		}
	return true;
	}

/*

	LoadInfo load sequence

	Core
	Title
	Desc
	Picture (with defs, loaded next to core)
	Icon
	Author
	Version

*/

bool C4ViewItem::LoadInfo(C4Group &ItemGroup, int Mode, C4Group &ParentGroup)
	{
	// Title by filename
	Title = Filename;	
	int iLastPeriod = Title.ReverseFind('.');
	if (iLastPeriod>=0) Title = Title.Left(iLastPeriod);

	// Icon by type
	Icon = ItemType[Type].Icon;
	Icon2 = ItemType[Type].Icon2;
	
	// Info by object core
	C4Scenario C4S;
	C4Folder C4F;
	C4PlayerInfoCore C4P;
	C4ObjectInfoCore C4I;
	C4DefCore C4D;
	CString sDesc1,sDesc2,sDesc3;
	CString strRank;

	switch (Type)
		{
		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
		case C4IT_Directory: 
			// Pack 
			EnablePack = true;
			break;
		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
		case C4IT_Scenario: 
			// Info core
			if (!C4S.Load(ItemGroup)) break;
			// Title
			Title = C4S.Head.Title; 
			// Engine Version
			C4XVer1 = C4S.Head.C4XVer[0];
			C4XVer2 = C4S.Head.C4XVer[1];
			C4XVer3 = C4S.Head.C4XVer[2];
			C4XVer4 = C4S.Head.C4XVer[3];
			// Icon
			if (Mode==C4VC_Player) Icon = Icon2 = C4S.Head.Icon + C4ITI_Scenario; 
			// MinPlayer/MaxPlayer
			MinPlayer = 1; // Notice: minimum player count is now always zero to allow network start of melees...
			MaxPlayer = C4S.Head.MaxPlayer;
			// Unregistered access
			UnregisteredAccess = C4S.Head.EnableUnregisteredAccess!=0 || C4S.Head.Replay!=0;
			// SaveGame
			SaveGame = C4S.Head.SaveGame!=0;
			// Replay
			EnableReplay = (C4S.Head.Replay != 0);
			// Definitions
			Definitions = C4S.Definitions;
			// Pack 
			EnablePack = true;
			// Editor allows properties on all scenarios
			EnableProperties = true;
			// Melee
			Melee = (C4S.Game.Goals.GetIDCount(C4Id("MELE")) > 0);
			// Engine to be used
			Engine = C4S.Head.Engine;
			// Difficulty
			Difficulty = C4S.Head.Difficulty;
			break;
		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
		case C4IT_Player: 
			// Info core
			if (!C4P.Load(ItemGroup)) break;
			// Title
			Title = C4P.PrefName; 
			// Desc
			//sDesc1.Format("%s %s",C4P.RankName,C4P.PrefName);
			sDesc1.Format("%s",C4P.PrefName);
			sDesc2.Format(LoadResStr("IDS_DESC_PLAYER"),C4P.Score,C4P.Rounds,C4P.RoundsWon,C4P.RoundsLost,TimeString(C4P.TotalPlayingTime),C4P.Comment);
			if (C4P.LastRound.Title.getLength() > 0)
				{
				sDesc3.Format(LoadResStr("IDS_DESC_LASTGAME"),C4P.LastRound.Title.getData(),DateString(C4P.LastRound.Date),TimeString(C4P.LastRound.Duration),C4P.LastRound.FinalScore);
				sDesc2+=sDesc3;
				}
			Desc.Set(MakeRTF(sDesc1,sDesc2));
			// Icon 
			if (Mode==C4VC_Player)
				Icon = Icon2 = C4ITI_Player + BoundBy<int>(C4P.PrefColor,0,C4ITI_PlayerNum);
			// Color
			EnableColor = true;
			dwColor = C4P.PrefColorDw;
			// Activation (if not savegame local)
			if (!SEqualNoCase(GetExtension(NoBackslash(GetPath(Path))),"c4s"))
				{
				EnableActivation = true;
				Activated = GetCfg()->IsModule(Path,GetCfg()->General.Participants);
				}
			// Properties
			//EnableProperties = true;
			// Pack 
			EnablePack = true;
			break;
		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
		case C4IT_ObjectInfo: 
			{
			// Info core
			if (!C4I.Load(ItemGroup)) break;
			// Title
			Title=C4I.Name;
			if (C4I.Rank>=C4ITI_RankNum) Title="* "+Title;
			// Id, experience
			id = C4I.id; Experience = C4I.Experience;
			// Icon
			Icon = Icon2 = C4ITI_Rank + BoundBy<int>(C4I.Rank, 0, C4ITI_RankNum*2-1)%C4ITI_RankNum;
			// Color by parent
			EnableColor = true;
			ColorByParent = true;
			// Desc Title
			sDesc1.Format("%s %s", C4I.sRankName.getData(), C4I.Name);
			// Desc Rank
			int32_t iNextRankExp; StdStrBuf sNextRankName;
			if (C4I.GetNextRankInfo(GetApp()->ClonkRanks, &iNextRankExp, &sNextRankName))
				strRank.Format(LoadResStr("IDS_DESC_PROMO"),sNextRankName.getData(),(int) iNextRankExp);
			else
				strRank.Format(LoadResStr("IDS_DESC_NOPROMO"));
			// Desc Main
			sDesc2.Format(LoadResStr("IDS_DESC_OBJECT"),
										C4I.TypeName,
										C4I.Experience,
										C4I.Rounds,
										C4I.DeathCount,
										strRank,
										TimeString(C4I.TotalPlayingTime),
										C4I.Birthday ? DateString(C4I.Birthday) : " --- "); 
			// Desc Physical
			int DescBarSize; DescBarSize=100;
			sDesc3  = MakeRTFBar(LoadResStr("IDS_DESC_ENERGY"),DescBarSize*C4I.Physical.Energy/C4MaxPhysical,1);
			sDesc3 += MakeRTFBar(LoadResStr("IDS_DESC_BREATH"),DescBarSize*C4I.Physical.Breath/C4MaxPhysical,2);
			sDesc3 += MakeRTFBar(LoadResStr("IDS_DESC_WALK"),DescBarSize*C4I.Physical.Walk/C4MaxPhysical,3);
			sDesc3 += MakeRTFBar(LoadResStr("IDS_DESC_JUMP"),DescBarSize*C4I.Physical.Jump/C4MaxPhysical,4);
			if (C4I.Physical.CanScale) sDesc3 += MakeRTFBar(LoadResStr("IDS_DESC_SCALE"),C4I.Physical.CanScale*DescBarSize*C4I.Physical.Scale/C4MaxPhysical,5);
			if (C4I.Physical.CanHangle) sDesc3 += MakeRTFBar(LoadResStr("IDS_DESC_HANGLE"),C4I.Physical.CanHangle*DescBarSize*C4I.Physical.Hangle/C4MaxPhysical,6);
			sDesc3 += MakeRTFBar(LoadResStr("IDS_DESC_DIG"),C4I.Physical.CanDig*DescBarSize*C4I.Physical.Dig/C4MaxPhysical,7);
			sDesc3 += MakeRTFBar(LoadResStr("IDS_DESC_SWIM"),DescBarSize*C4I.Physical.Swim/C4MaxPhysical,8);
			sDesc3 += MakeRTFBar(LoadResStr("IDS_DESC_THROW"),DescBarSize*C4I.Physical.Throw/C4MaxPhysical,9);
			sDesc3 += MakeRTFBar(LoadResStr("IDS_DESC_PUSH"),DescBarSize*C4I.Physical.Push/C4MaxPhysical,10);
			sDesc3 += MakeRTFBar(LoadResStr("IDS_DESC_FIGHT"),DescBarSize*C4I.Physical.Fight/C4MaxPhysical,11);
			if (C4I.Physical.Magic) sDesc3 += MakeRTFBar(LoadResStr("IDS_DESC_MAGIC"),DescBarSize*C4I.Physical.Magic/C4MaxPhysical,12);
			// Set Desc
			Desc.Set(MakeRTF(sDesc1,sDesc2,sDesc3));
			// Activation
			EnableActivation = true;
			Activated = C4I.Participation;
			// Pack 
			EnablePack = true;
			break;
			}
		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
		case C4IT_Definition: 
			// Info core
			if (C4D.Load(ItemGroup)) 
				{
				// Title
				Title = C4D.GetName(); 
				// ID
				id = C4D.id;
				}
			// Activation (if not subdef or local)
			if (!SEqualNoCase(GetExtension(NoBackslash(GetPath(Path))),"c4d"))
				if (!SEqualNoCase(GetExtension(NoBackslash(GetPath(Path))),"c4s"))
					if (!SEqualNoCase(GetExtension(NoBackslash(GetPath(Path))),"c4f"))
						{
						EnableActivation = true;
						Activated = GetCfg()->IsModule(Path,GetCfg()->Explorer.Definitions);
						if (SEqualNoCase(Path,GetCfg()->AtExePath(C4CFN_Objects))) LockActivation=true;
						}
			// Pack 
			EnablePack = true;
			// Override PictureRect if PictureRectFE is given
			if (C4D.PictureRectFE.Wdt > 0)
				C4D.PictureRect = C4D.PictureRectFE;
			// Load picture from BMP definition graphics as transparent section
			Picture.Load(ItemGroup, C4CFN_DefGraphics);
			Picture.CreateMask();
			Picture.SetSection(C4D.PictureRect.x, C4D.PictureRect.y, C4D.PictureRect.Wdt, C4D.PictureRect.Hgt);
			// Load picture from PNG definition graphics
			PNGPicture.Load(&ItemGroup, C4CFN_DefGraphicsPNG);
			PNGPicture.SetSection(C4D.PictureRect.x, C4D.PictureRect.y, C4D.PictureRect.Wdt, C4D.PictureRect.Hgt);
			PNGPicture.bSectionMode = true;
			break;
		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
		case C4IT_Folder: case C4IT_Group:
			// Info core
			if (C4F.Load(ItemGroup))
			{
				FolderIndex = C4F.Head.Index;
			}
			// Pack 
			EnablePack = true;
			break;
		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
		case C4IT_Engine:
			// Activation - currently, engine activation in editor view is not supported. the engine is just specified via the command line
			/*if (ParentGroup.GetStatus()==GRPF_Folder)
				{
				EnableActivation = true;
				Activated = GetCfg()->IsModule(Path,GetCfg()->Explorer.Engines);
				}*/
			// Determine module path
			CString strModule = Path;
			bool fTempCopy = false;
			// Engine is in a packed group
			if (!ItemExists(strModule))
			{
				// Extract item to temporary position just
				// to get the resource info (this is tough...!)
				strModule = GetCfg()->AtTempPath("Clonk.temp.c4x");
				if (C4Group_CopyItem(Path, strModule))
					fTempCopy = true;
			}
			// Get version info from file
			CFileVersion VerInfo(strModule);
			// Remove temp file
			if (fTempCopy) EraseItem(strModule);
			// Title
			Title = CString("Engine") + ' ' + VerInfo.GetFileVersion();
			// Version
			VS_FIXEDFILEINFO vsffi;
			if (VerInfo.GetFixedInfo(vsffi))
				{
				C4XVer1=HIWORD(vsffi.dwFileVersionMS);
				C4XVer2=LOWORD(vsffi.dwFileVersionMS);
				C4XVer3=HIWORD(vsffi.dwFileVersionLS);
				C4XVer4=LOWORD(vsffi.dwFileVersionLS);
				}
			break;
		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
		}

	// Title by component
	C4ComponentHost TempHost;
	if (TempHost.LoadEx("Title", ItemGroup, C4CFN_Title, GetCfg()->General.LanguageEx) || 
		  TempHost.LoadEx("Title", ItemGroup, C4CFN_DefNames, GetCfg()->General.LanguageEx))
	{
		StdStrBuf strTitle;
		if (TempHost.GetLanguageString(GetCfg()->General.LanguageEx, strTitle))
			Title = strTitle.getData();
	}

	// Desc
	LoadDesc(ParentGroup,ItemGroup);

	// Picture
	LoadPicture(ParentGroup,ItemGroup);

	// Title by original filename (developer)
	if (Mode==C4VC_Developer) 
		Title = Filename;

	// Custom icon
	HBITMAP hCustomIcon = NULL;
	int iIcon;
	if (Mode == C4VC_Player)
		if (ItemGroup.AccessEntry(C4CFN_ScenarioIcon))
			if (ItemGroup.ReadDDBSection(&hCustomIcon, NULL, 0,0, 16,16, 16,16, false))
				if ((iIcon = pIconList->Add(CBitmap::FromHandle(hCustomIcon), TRANSPCOLOR)) >= 0) // Hack!
					Icon = Icon2 = iIcon;
	if (hCustomIcon) DeleteObject(hCustomIcon);

  const char *strSecAuthors = "RedWolf Design;Clonk History Project;GWE-Team"; // Now hardcoded...

	// Custom author (only if group file is created by validated maker) ...disabled. Notice: custom authors were not displayed 
	/*char *bpText; unsigned int iSize;																	in editor anyway because there is no more player mode.	
	if (Mode == C4VC_Player)
		if (SIsModule(strSecAuthors, Author, NULL, true)) 
			if (ItemGroup.LoadEntry(C4CFN_Author,(char **)&bpText,&iSize,1))
				{
				// Load text file to string
				SReplaceChar(bpText,0x0D,0x00); 
				ShowAuthor = bpText;
				delete [] bpText;
				// No secured authors
				if (SSearchNoCase(ShowAuthor, strSecAuthors))
					ShowAuthor.Empty();
				}*/

	// Version info
	char *bpText; unsigned int iSize;
	if (ItemGroup.LoadEntry(C4CFN_Version, (char**) &bpText, &iSize, 1))
		{
		// Load text file to string
		SReplaceChar(bpText, 0x0D, 0x00); 
		Version = bpText;
		delete [] bpText;
		// Insert version info into loaded desc
		if ((Desc.GetDataSize() > 0) && !Version.IsEmpty())
			{
			CString strDesc = Desc.GetData();
			CString strVersion; strVersion.Format("\\par\\pard\\qr\\fs14 Version %s\\par", Version);
			int iInsertPos = SCharLastPos('}', strDesc);
			if (iInsertPos > -1) strDesc.Insert(iInsertPos, strVersion);
			Desc.Set(strDesc);
			}
		}

	return true;
	}

bool C4ViewItem::LoadPicture(C4Group &ParentGroup, C4Group &ItemGroup)
	{
	switch (Type)
		{
		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
		case C4IT_Scenario: case C4IT_Folder: 
		case C4IT_Group: case C4IT_Directory:
		case C4IT_WebLibLink:
			Picture.Load(ItemGroup,C4CFN_ScenarioTitle);
			PNGPicture.Load(&ItemGroup, C4CFN_ScenarioTitlePNG);
			break;
		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
		case C4IT_Player: case C4IT_ObjectInfo: 
			// Try to load new type portrait
			if (!PNGPicture.Load(&ItemGroup, C4CFN_Portrait))
				// Else try to load old type portrait
				Picture.Load(ItemGroup, C4CFN_Portrait_Old);
			break;
		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
		case C4IT_Bitmap:
			Picture.Load(ParentGroup,Filename);
			break;
		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
		case C4IT_PNG:
			if(!PNGPicture.Load(&ParentGroup, Filename)) return false;
			break;
		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
		case C4IT_Definition:
			// Title.bmp in definition folder may override def picture from Graphics.bmp
			if (ItemGroup.FindEntry(C4CFN_ScenarioTitle))
			{
				Picture.Load(ItemGroup,C4CFN_ScenarioTitle);
				// clear png pic (title.bmp overwrites graphics.png)
				PNGPicture.Clear(true);
			}
			if (ItemGroup.FindEntry(C4CFN_ScenarioTitlePNG))
				PNGPicture.Load(&ItemGroup, C4CFN_ScenarioTitlePNG);
			break;
		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
		}
	return true;
	}

/* Keep path, contents, and tree item, reload item data only */
/* Does definition update messages */

bool C4ViewItem::Refresh(int Mode)
	{
	
	// Keep data
	CString KeepPath = Path;
	bool KeepContentsLoaded = ContentsLoaded;
	HTREEITEM KeepHItem = hItem;

	// Open parent group
	C4Group ParentGroup;
	CString ParentPath = NoBackslash(GetPath(KeepPath));
	if (!ParentGroup.Open(ParentPath)) false;

	// Reload
	Clear(); Default();
	if (!Load(ParentGroup,KeepPath,Mode)) return false; // This would suck

	// Kept data
	ContentsLoaded = KeepContentsLoaded;
	hItem = KeepHItem;

	// Close parent group
	ParentGroup.Close();

	return true;
	}

int C4ViewItem::GetState(bool fScenarioFixed)
	{
	switch (Type)
		{
		case C4IT_Player:
			if (Activated) return 1;
			return 2;
		case C4IT_ObjectInfo:
			if (Activated) return 3;
			return 4;
		case C4IT_Definition:
			if (fScenarioFixed)
				{
				if (Activated) return 7;
				return 8;
				}
			if (Activated) return 1;
			return 2;
		case C4IT_Engine:
			if (this==pUsedEngine)
				{
				if (Activated) return 9;
				return 1;
				}
			if (Activated) return 10;
			return 2;
		}
	return 0;
	}

bool C4ViewItem::EngineVersionOk(C4ViewItem *pEngineItem)
	{
	// check version number
	return ::EngineVersionOk(pEngineItem->C4XVer1,pEngineItem->C4XVer2,pEngineItem->C4XVer3,pEngineItem->C4XVer4,C4XVer1,C4XVer2,C4XVer3,C4XVer4, EnableJoin);
	}

bool C4ViewItem::EngineVersionGreater(C4ViewItem *pEngineItem)
{
	return ::EngineVersionGreater(pEngineItem->C4XVer1,pEngineItem->C4XVer2,pEngineItem->C4XVer3,pEngineItem->C4XVer4,C4XVer1,C4XVer2,C4XVer3,C4XVer4);
}

bool C4ViewItem::EngineVersionLess(C4ViewItem *pEngineItem)
{
	return ::EngineVersionLess(pEngineItem->C4XVer1,pEngineItem->C4XVer2,pEngineItem->C4XVer3,pEngineItem->C4XVer4,C4XVer1,C4XVer2,C4XVer3,C4XVer4);
}
