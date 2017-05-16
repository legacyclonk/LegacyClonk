# Microsoft Developer Studio Project File - Name="anigrab" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

CFG=anigrab - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "anigrab.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "anigrab.mak" CFG="anigrab - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "anigrab - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "anigrab - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "anigrab - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "..\intermediate\vc6\anigrab\Release"
# PROP Intermediate_Dir "..\intermediate\vc6\anigrab\Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /MT /W3 /GX /O2 /I "..\standard\inc" /I "..\standard\lpng121" /I "..\standard\zlib" /I ".\loadpng" /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD BASE RSC /l 0x407 /d "NDEBUG"
# ADD RSC /l 0x407 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib shell32.lib advapi32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib vfw32.lib /nologo /subsystem:console /machine:I386 /out:"..\..\tools\anigrab\anigrab.exe"

!ELSEIF  "$(CFG)" == "anigrab - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "..\intermediate\vc6\anigrab\Debug"
# PROP Intermediate_Dir "..\intermediate\vc6\anigrab\Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /I "..\standard\inc" /I "..\standard\lpng121" /I "..\standard\zlib" /I ".\loadpng" /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /FR /YX /FD /c
# ADD BASE RSC /l 0x407 /d "_DEBUG"
# ADD RSC /l 0x407 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib vfw32.lib /nologo /subsystem:console /debug /machine:I386 /out:"C:\temp\anigrab.exe" /pdbtype:sept

!ENDIF 

# Begin Target

# Name "anigrab - Win32 Release"
# Name "anigrab - Win32 Debug"
# Begin Group "standard"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\standard\src\Bitmap256.cpp
# End Source File
# Begin Source File

SOURCE=..\standard\src\CStdFile.cpp
# End Source File
# Begin Source File

SOURCE=..\standard\src\Standard.cpp
# End Source File
# Begin Source File

SOURCE=..\standard\src\StdBitmap.cpp
# End Source File
# Begin Source File

SOURCE=..\standard\src\StdBuf.cpp
# End Source File
# Begin Source File

SOURCE=..\standard\src\StdFile.cpp
# End Source File
# Begin Source File

SOURCE=..\standard\src\StdVideo.cpp
# End Source File
# End Group
# Begin Group "zlib"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\standard\zlib\adler32.c
# End Source File
# Begin Source File

SOURCE=..\standard\zlib\compress.c
# End Source File
# Begin Source File

SOURCE=..\standard\zlib\crc32.c
# End Source File
# Begin Source File

SOURCE=..\standard\zlib\deflate.c
# End Source File
# Begin Source File

SOURCE=..\standard\zlib\gzio.c
# End Source File
# Begin Source File

SOURCE=..\standard\zlib\infback.c
# End Source File
# Begin Source File

SOURCE=..\standard\zlib\inffast.c
# End Source File
# Begin Source File

SOURCE=..\standard\zlib\inflate.c
# End Source File
# Begin Source File

SOURCE=..\standard\zlib\inftrees.c
# End Source File
# Begin Source File

SOURCE=..\standard\zlib\trees.c
# End Source File
# Begin Source File

SOURCE=..\standard\zlib\uncompr.c
# End Source File
# Begin Source File

SOURCE=..\standard\zlib\zutil.c
# End Source File
# End Group
# Begin Group "loadpng"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\loadpng\common.cpp
# End Source File
# Begin Source File

SOURCE=.\loadpng\loadpng.cpp
# End Source File
# Begin Source File

SOURCE=.\loadpng\savepng.cpp
# End Source File
# End Group
# Begin Group "pnglib"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\standard\lpng121\png.c
# End Source File
# Begin Source File

SOURCE=..\standard\lpng121\pngerror.c
# End Source File
# Begin Source File

SOURCE=..\standard\lpng121\pnggccrd.c
# End Source File
# Begin Source File

SOURCE=..\standard\lpng121\pngget.c
# End Source File
# Begin Source File

SOURCE=..\standard\lpng121\pngmem.c
# End Source File
# Begin Source File

SOURCE=..\standard\lpng121\pngpread.c
# End Source File
# Begin Source File

SOURCE=..\standard\lpng121\pngread.c
# End Source File
# Begin Source File

SOURCE=..\standard\lpng121\pngrio.c
# End Source File
# Begin Source File

SOURCE=..\standard\lpng121\pngrtran.c
# End Source File
# Begin Source File

SOURCE=..\standard\lpng121\pngrutil.c
# End Source File
# Begin Source File

SOURCE=..\standard\lpng121\pngset.c
# End Source File
# Begin Source File

SOURCE=..\standard\lpng121\pngtrans.c
# End Source File
# Begin Source File

SOURCE=..\standard\lpng121\pngvcrd.c
# End Source File
# Begin Source File

SOURCE=..\standard\lpng121\pngwio.c
# End Source File
# Begin Source File

SOURCE=..\standard\lpng121\pngwrite.c
# End Source File
# Begin Source File

SOURCE=..\standard\lpng121\pngwtran.c
# End Source File
# Begin Source File

SOURCE=..\standard\lpng121\pngwutil.c
# End Source File
# End Group
# Begin Group "engine"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\engine\inc\C4Version.h
# End Source File
# End Group
# Begin Source File

SOURCE=.\anigrab.cpp
# End Source File
# End Target
# End Project
