/*
 * LegacyClonk
 *
 * Copyright (c) 1998-2000, Matthes Bender (RedWolf Design)
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

#ifndef C4CONFIGSHAREWARE_H_INC
#define C4CONFIGSHAREWARE_H_INC

#include <C4Config.h>
#include <C4Group.h>

const int MaxRegDataLen = 4096;
const char szInvalidKeyData[] = "r>iU218f3_030\r\n0ipX>ZeuX\r\nZichtVLpI=WeUt\r\nYNtzNcQy=EZs1\r\nAhVsloP=PYptk TE0e\r\n5Vtms5_0:-0^-0>\r\nDfhCqHy=27CIxFpB\r\n\r\nnxSR+?bD50+H[:fEnyW^UcASVTSR9n>Oez`2qHN3YWbz8P;SSqkvXtXMM6Z1UQNT\r\nrpFHRy/6pZ2T6E1iGF1Dt\370Ofw7f\370bUvuM3_jl8TsxWN8;d0kCj3v/JRWBO/Gvxpx\r\nTrBomp81>gkZoddjFcyTwx[J/dNIKEzt]Tj5em=]60w@\r\n";

void UnscrambleString(char *szString);

class C4ConfigShareware: public C4Config
	{
	public:
		C4ConfigShareware();
		~C4ConfigShareware();
	protected:
		bool RegistrationValid;
		char RegData[MaxRegDataLen + 1];
		char KeyFile[CFG_MaxString + 1];
		char InvalidKeyFile[CFG_MaxString + 1];
	public:
		void Default();
		BOOL Save();
		BOOL Load(BOOL forceWorkingDirectory=TRUE, const char *szCustomFile=NULL);
	public:
		void ClearRegistrationError();
		BOOL Registered();
		BOOL IsFreeFolder(const char *szFoldername, const char *szMaker);
		bool LoadRegistration();
		bool LoadRegistration(const char *szFrom);
		const char* GetRegistrationData(const char* strField);
		const char* GetRegistrationError();
		const char* GetKeyFilename();
		const char* GetInvalidKeyFilename();
		const char* GetKeyPath();
		StdStrBuf GetKeyMD5();
	protected:
		StdStrBuf RegistrationError;
		bool HandleError(const char *strMessage);
	};

extern C4ConfigShareware Config;
#endif // C4CONFIGSHAREWARE_H_INC
