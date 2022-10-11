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

#import <AppKit/AppKit.h>
#import <Standard.h>

#include <string>

#ifndef USE_CONSOLE

void requestUserAttention()
{
	[NSApp requestUserAttention: NSCriticalRequest];
}

bool sendFileToTrash(const char *szFilename)
{
	NSString *filename = [NSString stringWithCString: szFilename];
	return [[NSWorkspace sharedWorkspace]
		performFileOperation: NSWorkspaceRecycleOperation
		source: [filename stringByDeletingLastPathComponent]
		destination: @""
		files: [NSArray arrayWithObject: [filename lastPathComponent]]
		tag: 0];
}

#endif

bool isGerman()
{
	CFArrayRef list =
		(CFArrayRef)CFPreferencesCopyAppValue(CFSTR("AppleLanguages"),
			kCFPreferencesCurrentApplication);
	return list && CFGetTypeID(list) == CFArrayGetTypeID() &&
		!CFStringCompare((CFStringRef)CFArrayGetValueAtIndex(list, 0), CFSTR("de"), 0);
}

void restart(char *[])
{
	NSString *filename = [[NSBundle mainBundle] bundlePath];
	NSString *cmd = [@"open " stringByAppendingString: filename];
	system([cmd UTF8String]);
}

std::string CStdApp::GetGameDataPath()
{
	return [[[NSBundle mainBundle] resourcePath] fileSystemRepresentation];
}
