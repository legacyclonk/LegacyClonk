/*
 * LegacyClonk
 *
 * Copyright (c) RedWolf Design
 * Copyright (c) 2017-2019, The LegacyClonk Team and contributors
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

/* Update package support */

#pragma once

#include "C4Constants.h"
#include "C4Group.h"

const int C4UP_MaxUpGrpCnt = 50;

class C4UpdatePackageCore
{
public:
	C4UpdatePackageCore();

public:
	int32_t RequireVersion[4];
	char Name[C4MaxName + 1];
	char DestPath[_MAX_PATH + 1];
	int32_t GrpUpdate;
	int32_t UpGrpCnt; // number of file versions that can be updated by this package
	uint32_t GrpChks1[C4UP_MaxUpGrpCnt], GrpChks2;
	uint32_t GrpContentsCRC1[C4UP_MaxUpGrpCnt], GrpContentsCRC2;

public:
	void CompileFunc(StdCompiler *pComp);
	bool Load(C4Group &hGroup);
	bool Save(C4Group &hGroup);
};


class C4UpdatePackage : public C4UpdatePackageCore
{
public:
	enum class CheckResult
	{
		Ok,
		NoSource,
		BadSource,
		AlreadyUpdated,
		BadVersion
	};

	bool Load(C4Group *pGroup);
	bool Execute(C4Group *pGroup);
	static bool Optimize(C4Group *pGrpFrom, const char *strTarget);
	CheckResult Check(C4Group *pGroup);
	bool MakeUpdate(const char *strFile1, const char *strFile2, const char *strUpdateFile, const char *strName = nullptr);

protected:
	bool DoUpdate(C4Group *pGrpFrom, class C4GroupEx *pGrpTo, const char *strFileName);
	bool DoGrpUpdate(C4Group *pUpdateData, class C4GroupEx *pGrpTo);
	static bool Optimize(C4Group *pGrpFrom, class C4GroupEx *pGrpTo, const char *strFileName);

	bool MkUp(C4Group *pGrp1, C4Group *pGrp2, C4GroupEx *pUpGr, bool *fModified);

	CStdFile Log;

	template<typename... Args>
	void WriteLog(const char *strMsg, Args... args)
	{
		const auto output = FormatString(strMsg, args...);
		Log.Write(output.getData(), output.getLength());
		Log.Flush();
	}
};

bool C4Group_ApplyUpdate(C4Group &hGroup);
