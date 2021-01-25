/*
 * LegacyClonk
 *
 * Copyright (c) 2012-2016, The OpenClonk Team and contributors
 * Copyright (c) 2020, The LegacyClonk Team and contributors
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

#include "C4Network2UPnP.h"
#ifdef _WIN32
#include "C4Network2UPnPWin32.h"
#endif

C4Network2UPnP::C4Network2UPnP() : impl{
#ifdef _WIN32
									   new C4Network2UPnPImplWin32{}
#else
									   new C4Network2UPnPImpl{}
#endif
								   }
{
}

C4Network2UPnP::~C4Network2UPnP()
{
	ClearMappings();
}
