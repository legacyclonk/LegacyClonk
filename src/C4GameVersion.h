/*
 * LegacyClonk
 *
 * Copyright (c) RedWolf Design
 * Copyright (c) 2017-2022, The LegacyClonk Team and contributors
 *
 * Distributed under the terms of the ISC license; see accompanying file
 * "COPYING" for details.
 *
 * "Clonk" is a registered trademark of Matthes Bender, used with permission.
 * See accompanying file "TRADEMARK" for details.
 *
 * To redistribute this file separately, substitute the full license texts
 * for the above references.
 */

#pragma once

#include "C4Version.h"
#include "C4InputValidation.h"

#include <cinttypes>

struct C4GameVersion
{
	ValidatedStdStrBuf<C4InVal::VAL_NameAllowEmpty> sEngineName; // status only - not used for comparison
	int32_t iVer[4];
	int32_t iBuild;

	C4GameVersion(const char *szEngine = C4ENGINENAME, int32_t iVer1 = C4XVER1, int32_t iVer2 = C4XVER2, int32_t iVer3 = C4XVER3, int32_t iVer4 = C4XVER4, int32_t iVerBuild = C4XVERBUILD)
	{
		Set(szEngine, iVer1, iVer2, iVer3, iVer4, iVerBuild);
	}

	void Set(const char *szEngine = C4ENGINENAME, int32_t iVer1 = C4XVER1, int32_t iVer2 = C4XVER2, int32_t iVer3 = C4XVER3, int32_t iVer4 = C4XVER4, int32_t iVerBuild = C4XVERBUILD)
	{
		sEngineName.CopyValidated(szEngine); iVer[0] = iVer1; iVer[1] = iVer2; iVer[2] = iVer3; iVer[3] = iVer4; iBuild = iVerBuild;
	}

	StdStrBuf GetString() const
	{
		return FormatString("%s %" PRId32 ".%" PRId32 ".%" PRId32 ".%" PRId32 " [%" PRId32 "]", sEngineName.getData(), iVer[0], iVer[1], iVer[2], iVer[3], iBuild);
	}

	bool operator==(const C4GameVersion &rCmp) const
	{
		return iVer[0] == rCmp.iVer[0] && iVer[1] == rCmp.iVer[1] && iVer[2] == rCmp.iVer[2] && iVer[3] == rCmp.iVer[3] && iBuild == rCmp.iBuild;
	}

	void CompileFunc(StdCompiler *pComp, bool fEngineName)
	{
		if (fEngineName)
		{
			pComp->Value(mkDefaultAdapt(sEngineName, ""));
			pComp->Separator();
		}
		else if (pComp->isCompiler())
			sEngineName = "";
		pComp->Value(mkArrayAdapt(iVer, 0));
		pComp->Separator();
		pComp->Value(mkDefaultAdapt(iBuild, 0));
	}
};

// helper
inline int CompareVersion(int iVer1, int iVer2, int iVer3, int iVer4, int iVerBuild,
	int iRVer1 = C4XVER1, int iRVer2 = C4XVER2, int iRVer3 = C4XVER3, int iRVer4 = C4XVER4, int iRVerBuild = C4XVERBUILD)
{

	if (iVer1 > iRVer1) return 1; if (iVer1 < iRVer1) return -1;
	if (iVer2 > iRVer2) return 1; if (iVer2 < iRVer2) return -1;
	if (iVer3 > iRVer3) return 1; if (iVer3 < iRVer3) return -1;
	if (iVer4 > iRVer4) return 1; if (iVer4 < iRVer4) return -1;

	if (iVerBuild > 0)
	{
		if (iVerBuild > iRVerBuild) return 1; if (iVerBuild < iRVerBuild) return -1;
	}

	return 0;
}
