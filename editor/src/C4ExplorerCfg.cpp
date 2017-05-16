/* Copyright (C) 1998-2000  Matthes Bender  RedWolf Design */

/* Config module which can also check for the presence of selected groups */

#include "C4Explorer.h"

bool C4ExplorerCfg::AddModule(const char *szPath, char *szModules)
	{
	return SAddModule(szModules,szPath);
	}

bool C4ExplorerCfg::IsModule(const char *szPath, char *szModules)
	{
	return SIsModule(szModules,szPath);
	}

bool C4ExplorerCfg::RemoveModule(const char *szPath, char *szModules)
	{
	return SRemoveModule(szModules,szPath);
	}

bool C4ExplorerCfg::ValidateModules(const char *szPath, char *szModules, bool fAllowLocal)
	{
	char buf[_MAX_PATH+1], buf2[_MAX_PATH+1];
	CString NewModules,ModulePath;
	C4Group PlayerGroup;
	for (int cseg=0; SCopySegment(szModules,cseg,buf,';',_MAX_PATH); cseg++)
		{
		SClearFrontBack(buf);
		// Local file specification: prepend path
		if (fAllowLocal)
			if (!SSearch(buf, "\\") && !SSearch(buf, "/"))
			{
				sprintf(buf2, "%s%s", szPath, buf);
				SCopy(buf2, buf);
			}
		// Module must be valid group
		if (PlayerGroup.Open(buf)) 
			{
			ModulePath=buf;
			// Module must be in specified path
			if (SEqualNoCase( szPath, ModulePath.Left(SLen(szPath)) ))
				{
				// Add to list of validated modules
				if (!NewModules.IsEmpty()) NewModules += ";"; 
				NewModules += buf;
				}
			PlayerGroup.Close();
			}
		}
	SCopy(NewModules,szModules);
	return true;
	}
