#import <AppKit/AppKit.h>

void requestUserAttention()
{
	[NSApp requestUserAttention: NSCriticalRequest];
}

bool sendFileToTrash(const char* szFilename)
{
    NSString* filename = [NSString stringWithCString: szFilename];
    return [[NSWorkspace sharedWorkspace]
            performFileOperation: NSWorkspaceRecycleOperation
            source: [filename stringByDeletingLastPathComponent]
            destination: @""
            files: [NSArray arrayWithObject: [filename lastPathComponent]]
            tag: 0];
}

bool isGerman()
{
    CFArrayRef list =
        (CFArrayRef)CFPreferencesCopyAppValue(CFSTR("AppleLanguages"), 
            kCFPreferencesCurrentApplication);
    return list && CFGetTypeID(list) == CFArrayGetTypeID() &&
        !CFStringCompare((CFStringRef)CFArrayGetValueAtIndex(list, 0), CFSTR("de"), 0);
}

void restart(char*[])
{
    NSString* filename = [[NSBundle mainBundle] bundlePath];
    NSString* cmd = [@"open " stringByAppendingString: filename];
    system([cmd UTF8String]);
}
