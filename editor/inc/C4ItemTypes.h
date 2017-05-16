/* Copyright (C) 1998-2000  Matthes Bender  RedWolf Design */

/* Types for main view items */

const int C4IT_WorkingDirectory = -1;

const int C4IT_Directory	= 0,
					C4IT_Player			= 1,
					C4IT_Folder			= 2,
					C4IT_Scenario		= 3,
					C4IT_Group			= 4,
					C4IT_Material   = 5,
					C4IT_Help				= 6,
					C4IT_Log				= 7,
					C4IT_Text				= 8,
					C4IT_RichText		= 9,
					C4IT_Bitmap			= 10,
					C4IT_PNG				= 11,
					C4IT_Script			= 12,
					C4IT_Definition	= 13,
					C4IT_DefFolder	= 14,
					C4IT_ObjectInfo	=	15,
					C4IT_Sound			= 16,
					C4IT_Music			= 17,
					C4IT_MusicOgg   = 18,
					C4IT_Video			= 19,
					C4IT_Binary			= 20,
					C4IT_Executable	= 21,
					C4IT_Engine			= 22,
					C4IT_Zip				= 23,
					C4IT_Animation	= 24,
					C4IT_Html				= 25,
					C4IT_WebLibLink	= 26,
					C4IT_WebLibCat	= 27,
					C4IT_WebLibEntry= 28,
					C4IT_KeyFile		= 29,
					C4IT_Update 		= 30,
					C4IT_Backup 		= 31,
					C4IT_Unknown		= 32;

const int C4IT_Max				= 33;

const int C4ITE_None			  = 0,
					C4ITE_Text			  = 1,
					C4ITE_Bitmap		  = 2,
					C4ITE_PNG				  = 3,
					C4ITE_Sound			  = 4,
					C4ITE_RichText	  = 5,
					C4ITE_Music			  = 6,
					C4ITE_Script		  = 7,
					C4ITE_Zip				  = 8,
					C4ITE_Definition  = 9,
					C4ITE_Html			  = 10,
					C4ITE_ShellLaunch = 11;

const int C4ITI_Rank			= 83,
					C4ITI_RankNum		= 24,
					C4ITI_Player		= 107,
					C4ITI_PlayerNum = 12,
					C4ITI_Scenario	= 24,
					C4ITI_Captain		= 82;

const int C4IT_SortByIcon1 = 26,
					C4IT_SortByIcon2 = 35;

struct C4ItemType
	{
	const char *Name;
	const char *Extension;
	int Icon,Icon2;
	int Expand;
	int PlayerView;
	int Edit;
	int New;
	int NewBinary;
	};

extern C4ItemType ItemType[C4IT_Max];

int GetItemType(const char *szExtension);

int FileItemType(const char *szFilename);

