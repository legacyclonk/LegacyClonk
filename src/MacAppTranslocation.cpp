/*
 * LegacyClonk
 *
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

#include "MacAppTranslocation.h"

#include <memory>
#include <stdexcept>

#include <CoreFoundation/CFURL.h>
#include <CoreFoundation/CFString.h>
#include <dlfcn.h>

std::optional<std::string> GetNonTranslocatedPath(std::string_view path)
{
	const std::unique_ptr<void, decltype(&dlclose)> handle{dlopen("/System/Library/Frameworks/Security.framework/Security", RTLD_LAZY), dlclose};
	if (!handle)
	{
		return {};
	}

	bool isTranslocated = false;
	Boolean (*mySecTranslocateIsTranslocatedURL)(CFURLRef path, bool *isTranslocated, CFErrorRef * __nullable error);
	mySecTranslocateIsTranslocatedURL = reinterpret_cast<decltype(mySecTranslocateIsTranslocatedURL)>(dlsym(handle.get(), "SecTranslocateIsTranslocatedURL"));
	if (mySecTranslocateIsTranslocatedURL)
	{
		const auto stringRef = CFStringCreateWithBytes(nullptr, reinterpret_cast<const UInt8 *>(path.data()), path.size(), kCFStringEncodingUTF8, false);
		const auto urlRef = CFURLCreateWithFileSystemPath(nullptr, stringRef, kCFURLPOSIXPathStyle, false);
		if (mySecTranslocateIsTranslocatedURL(urlRef, &isTranslocated, nullptr))
		{
			if (isTranslocated)
			{
				CFURLRef __nullable (*mySecTranslocateCreateOriginalPathForURL)(CFURLRef translocatedPath, CFErrorRef * __nullable error);
				mySecTranslocateCreateOriginalPathForURL = reinterpret_cast<decltype(mySecTranslocateCreateOriginalPathForURL)>(dlsym(handle.get(), "SecTranslocateCreateOriginalPathForURL"));
				if (mySecTranslocateCreateOriginalPathForURL)
				{
					const auto originalUrl = mySecTranslocateCreateOriginalPathForURL(urlRef, nullptr);
					const auto originalStringRef = CFURLCopyPath(originalUrl);
					const auto originalCString = CFStringGetCStringPtr(originalStringRef, kCFStringEncodingUTF8);
					if (!originalCString)
					{
						throw std::runtime_error{"The path is translocated, but CFStringGetCStringPtr returned nullptr"};
					}
					return {originalCString};
				}
			}
		}
	}

	return {};
}
