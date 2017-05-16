/* Copyright (C) 1998-2000  Matthes Bender  RedWolf Design */

/* Item to represent loaded file data in the main view */

#include "C4PNG.h"

class C4ViewItem  
	{
	public:
		C4ViewItem();
		virtual ~C4ViewItem();
	public:
		int Type,Icon,Icon2;
		int Activated;
		bool EnableActivation;
		bool EnableProperties;
		bool EnableJoin;
		bool EnablePack;
		bool EnableModify;
		bool EnableReplay;
		bool LockActivation;
		CString Title;
		CString Path,Filename;
		CString Author,ShowAuthor;
		CString Version;
		CString Engine;
		int Creation;
		int GroupStatus,GroupOriginal;
		HTREEITEM hItem;
		bool ContentsLoaded;
		C4ComponentHost Desc;
		bool DescText;
		CDIBitmap Picture;
		C4PNG PNGPicture;
		C4SDefinitions Definitions;
		int MinPlayer,MaxPlayer;
		int DeveloperMode,NoRuntimeJoin;
		bool UnregisteredAccess,SaveGame;
		C4ID id;
		int Experience;
		int C4XVer1,C4XVer2,C4XVer3,C4XVer4;
		bool EnableColor;
		bool ColorByParent;
		DWORD dwColor;
		bool Melee;
		int Difficulty;
		int FolderIndex;
	public:
		int GetState(bool fScenarioFixed);
		bool Refresh(int Mode);
		bool Load(C4Group &ParentGroup, CString &rPath, int Mode);
		void Clear();
		void Default();
		bool EngineVersionOk(C4ViewItem* pEngineItem);
		bool EngineVersionGreater(C4ViewItem* pEngineItem);
		bool EngineVersionLess(C4ViewItem* pEngineItem);
	protected:
		bool LoadPicture(C4Group &ParentGroup, C4Group &ItemGroup);
		bool LoadInfo(C4Group &ItemGroup, int Mode, C4Group &ParentGroup);
		bool LoadDesc(C4Group &ParentGroup, C4Group &ItemGroup);
	};
