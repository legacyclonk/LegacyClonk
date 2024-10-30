/*
 * LegacyClonk
 *
 * Copyright (c) 2024, The LegacyClonk Team and contributors
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

#include "C4TextEncoding.h"

#ifdef HAVE_ICONV
#include <langinfo.h>
#endif

#ifdef HAVE_ICONV

void C4TextEncodingConverter::CreateConverters(const char *const charsetCodeName)
{
	const char *systemCodeSet{nl_langinfo(CODESET)};
	if (!systemCodeSet)
	{
		systemCodeSet = "ASCII";
	}

	const std::lock_guard lock{iconvMutex};
	clonkToSystem = {iconv_open(systemCodeSet, charsetCodeName)};
	systemToClonk = {iconv_open(charsetCodeName, systemCodeSet)};
	clonkToUtf8 = {iconv_open("UTF-8", charsetCodeName)};
	utf8ToClonk = {iconv_open(charsetCodeName, "UTF-8")};
}

#endif

C4TextEncodingConverter TextEncodingConverter;
