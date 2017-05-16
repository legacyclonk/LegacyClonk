/* Copyright (C) 1998-2000  Matthes Bender  RedWolf Design */

/* Config module which can also check for the presence of selected groups */

#if !defined(AFX_C4EXPLORERCFG_H__4A6E69C0_7A2E_11D2_8888_0040052C10D3__INCLUDED_)
#define AFX_C4EXPLORERCFG_H__4A6E69C0_7A2E_11D2_8888_0040052C10D3__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

class C4ExplorerCfg: public C4ConfigShareware
	{
	public:
		bool ValidateModules(const char *szPath, char *szModules, bool fAllowLocal = false);
		bool RemoveModule(const char *szPath, char *szModules);
		bool IsModule(const char *szPath, char *szModules);
		bool AddModule(const char *szPath, char *szModules);
	};

#endif // !defined(AFX_C4EXPLORERCFG_H__4A6E69C0_7A2E_11D2_8888_0040052C10D3__INCLUDED_)
