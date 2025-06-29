/*
 * LegacyClonk
 *
 * Copyright (c) 2025, The LegacyClonk Team and contributors
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

#include "StdOSVersion.h"

#import <Foundation/Foundation.h>

CStdOSVersion CStdOSVersion::GetLocal()
{
	const NSOperatingSystemVersion version{[[NSProcessInfo processInfo] operatingSystemVersion]};
	return {
		static_cast<std::uint16_t>(version.majorVersion),
		static_cast<std::uint16_t>(version.minorVersion),
		static_cast<std::uint16_t>(version.patchVersion)
	};
}

std::string CStdOSVersion::GetFriendlyOSName()
{
	const NSString *operatingSystemVersionString{[[NSProcessInfo processInfo] operatingSystemVersionString]};
	const char *const cstr{[operatingSystemVersionString cStringUsingEncoding:NSWindowsCP1252StringEncoding]};
	return cstr ? cstr : "";
}
