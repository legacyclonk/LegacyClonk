/* Copyright (C) 1998-2000  Matthes Bender  RedWolf Design */

/* Types for main view items */

#include "C4Explorer.h"

C4ItemType ItemType[C4IT_Max] = 
	
	{

	{ "IDS_TYPE_DIRECTORY", "",			2, 21, 1, 1, C4ITE_None,			  2, 0 },
	{ "IDS_TYPE_PLAYER",		"c4p",  6,  6, 1, 1, C4ITE_None,			  2, IDN_PLAYER },
	{ "IDS_TYPE_FOLDER",		"c4f",  4, 22, 1, 1, C4ITE_None,			  2, IDN_FOLDER },
	{ "IDS_TYPE_SCENARIO",	"c4s", 14, 14, 1, 1, C4ITE_None,			  2, IDN_SCENARIO },
	{ "IDS_TYPE_GROUP",			"c4g",  5,  5, 1, 0, C4ITE_None,			  0, 0 },
	{ "IDS_TYPE_MATERIAL",	"c4m", 20, 20, 0, 0, C4ITE_Text,			  0, 0 },
	{ "IDS_TYPE_HELP",			"hlp",  7,  7, 0, 0, C4ITE_None,			  0, 0 },
	{ "IDS_TYPE_LOG",				"log",  8,  8, 0, 0, C4ITE_Text,			  0, 0 },
	{ "IDS_TYPE_TEXT",			"txt",  9,  9, 0, 0, C4ITE_Text,			  1, IDN_TEXT },
	{ "IDS_TYPE_RICHTEXT",	"rtf", 11, 11, 0, 0, C4ITE_RichText,	  1, IDN_RICHTEXT },
	{ "IDS_TYPE_BITMAP",		"bmp", 15, 15, 0, 0, C4ITE_Bitmap,		  0, 0 },
	{ "IDS_TYPE_BITMAP",		"png", 15, 15, 0, 0, C4ITE_PNG,				  1, IDN_PNG },
	{ "IDS_TYPE_SCRIPT",		"c",	 10, 10, 0, 0, C4ITE_Script,		  1, IDN_SCRIPT },
	{ "IDS_TYPE_DEFINITION","c4d", 12, 12, 1, 1, C4ITE_None,			  1, IDN_DEFINITION },
	{ "IDS_TYPE_DEFFOLDER", "c4d", 12, 12, 0, 0, C4ITE_None,			  2, IDN_DEFFOLDER },
	{ "IDS_TYPE_OBJECTINFO","c4i", 13, 13, 1, 1, C4ITE_None,			  0, 0 },
	{ "IDS_TYPE_SOUND",			"wav", 17, 17, 0, 0, C4ITE_Sound,			  1, IDN_SOUND },
	{ "IDS_TYPE_MUSIC",			"mid", 16, 16, 0, 0, C4ITE_Music,			  0, 0 },
	{ "IDS_TYPE_MUSIC",			"ogg", 16, 16, 0, 0, C4ITE_Music,	      0, 0 },
	{ "IDS_TYPE_VIDEO",			"avi", 18, 18, 0, 0, C4ITE_None,			  0, 0 },
	{ "IDS_TYPE_BINARY",		"c4b", 19, 19, 0, 0, C4ITE_None,			  0, 0 },
	{	"IDS_TYPE_EXECUTABLE","exe",  1,  1, 0, 0, C4ITE_None,			  0, 0 },
	{ "IDS_TYPE_ENGINE",		"exe",  3,  3, 0, 1, C4ITE_None,			  0, 0 },
	{ "IDS_TYPE_ZIP",				"zip", 76, 76, 0, 1, C4ITE_Zip,				  0, 0 },
	{ "IDS_TYPE_ANIMATION",	"c4v", 18, 18, 0, 0, C4ITE_None,			  0, 0 },
	{ "IDS_TYPE_HTML",			"html", 9,  9, 0, 0, C4ITE_ShellLaunch, 0, 0 },
	{	"IDS_TYPE_LINK",			"c4l", 77, 77, 1, 1, C4ITE_None,			  0, 0 },
	{	"IDS_TYPE_KEYFILE",		 "c4k",	0,  0, 0, 0, C4ITE_None,			  0, 0 },
	{	"IDS_TYPE_UPDATE",		"c4u", 78, 78, 1, 0, C4ITE_None,			  0, 0 },
	{	"IDS_TYPE_BACKUP",		"bak",  2, 21, 1, 0, C4ITE_None,			  0, 0 },
	{	"IDS_TYPE_NONE"		 		 "",			0,  0, 0, 0, C4ITE_None,			  0, 0 }
		
	};

int GetItemType(const char *szExtension)
	{
	for (int ctype=0; ctype<C4IT_Max; ctype++)
		if (SEqualNoCase(szExtension,ItemType[ctype].Extension))
			return ctype;
	return C4IT_Unknown;
	}

int FileItemType(const char *szFilename)
	{
	int Type;

	// Type unknown
	Type = C4IT_Unknown;
	// Type by extension
	if (*GetExtension(szFilename))
		Type = GetItemType(GetExtension(szFilename));
	// Type open directory
	if (Type==C4IT_Unknown)
		if (DirectoryExists(szFilename))
			Type = C4IT_Directory;
	// Type packed directory
	C4Group hTest;
	if (Type==C4IT_Unknown)
		if (hTest.Open(szFilename))
			{
			Type = C4IT_Directory;
			hTest.Close();
			}
	// special for exe-engines
	if (Type==C4IT_Executable)
		if (SEqual2NoCase(szFilename, "clonk"))
			Type = C4IT_Engine;
	return Type;
	}