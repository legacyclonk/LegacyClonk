# Microsoft Developer Studio Project File - Name="c4group" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

CFG=c4group - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "c4group.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "c4group.mak" CFG="c4group - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "c4group - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "c4group - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "c4group - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "../intermediate/vc6/C4Group/Release"
# PROP Intermediate_Dir "../intermediate/vc6/C4Group/Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /W3 /GX /O2 /I "../engine/inc" /I "../engine/sec" /I "../standard/inc" /I "../standard" /I "../standard/lpng121" /I "../standard/zlib" /I "../engine/fmod" /I "../editor/inc" /D "NDEBUG" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /D "C4GROUP" /FR /YX /FD /c
# ADD BASE RSC /l 0x407 /d "NDEBUG"
# ADD RSC /l 0x407 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 kernel32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386 /nodefaultlib:"libc.lib" /out:"..\..\tools\c4group\c4group.exe"

!ELSEIF  "$(CFG)" == "c4group - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "../intermediate/vc6/C4Group/Debug"
# PROP Intermediate_Dir "../intermediate/vc6/C4Group/Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /GZ /c
# ADD CPP /nologo /W3 /Gm /GX /ZI /Od /I "../engine/fmod" /I "../editor/inc" /I "../engine/inc" /I "../engine/sec" /I "../standard/inc" /I "../standard" /I "../standard/lpng121" /I "../standard/zlib" /D "_DEBUG" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /D "C4GROUP" /FR /YX /FD /GZ /c
# ADD BASE RSC /l 0x407 /d "_DEBUG"
# ADD RSC /l 0x407 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /nodefaultlib:"libcd.lib" /out:"..\..\tools\c4group\c4groupD.exe" /pdbtype:sept

!ENDIF 

# Begin Target

# Name "c4group - Win32 Release"
# Name "c4group - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Group "Engine"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\engine\src\C4ComponentHost.cpp
# End Source File
# Begin Source File

SOURCE=..\engine\src\C4Config.cpp
# End Source File
# Begin Source File

SOURCE=..\engine\sec\C4ConfigShareware.cpp
# End Source File
# Begin Source File

SOURCE=..\engine\src\C4Def.cpp
# End Source File
# Begin Source File

SOURCE=..\engine\src\C4Group.cpp
# End Source File
# Begin Source File

SOURCE=..\engine\src\C4GroupSet.cpp
# End Source File
# Begin Source File

SOURCE=..\engine\src\C4Id.cpp
# End Source File
# Begin Source File

SOURCE=..\engine\src\C4IDList.cpp
# End Source File
# Begin Source File

SOURCE=..\engine\src\C4InfoCore.cpp
# End Source File
# Begin Source File

SOURCE=..\engine\src\C4InputValidation.cpp
# End Source File
# Begin Source File

SOURCE=..\engine\src\C4Language.cpp
# End Source File
# Begin Source File

SOURCE=..\engine\src\C4NameList.cpp
# End Source File
# Begin Source File

SOURCE=..\engine\src\C4RankSystem.cpp
# End Source File
# Begin Source File

SOURCE=..\engine\src\C4Scenario.cpp
# End Source File
# Begin Source File

SOURCE=..\Engine\sec\C4SecurityCertificates.cpp
# End Source File
# Begin Source File

SOURCE=..\engine\src\C4Shape.cpp
# End Source File
# Begin Source File

SOURCE=..\engine\src\C4StringTable.cpp
# End Source File
# Begin Source File

SOURCE=..\Engine\src\C4Update.cpp
# End Source File
# Begin Source File

SOURCE=..\engine\src\C4Value.cpp
# End Source File
# Begin Source File

SOURCE=..\engine\src\C4ValueList.cpp
# End Source File
# Begin Source File

SOURCE=..\engine\src\C4ValueMap.cpp
# End Source File
# End Group
# Begin Source File

SOURCE=.\c4group_cmdl.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\engine\inc\C4Group.h
# End Source File
# Begin Source File

SOURCE=..\engine\inc\C4Update.h
# End Source File
# Begin Source File

SOURCE=..\Engine\inc\C4Version.h
# End Source File
# End Group
# Begin Group "Library Files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\standard\lib\Standard.lib

!IF  "$(CFG)" == "c4group - Win32 Release"

!ELSEIF  "$(CFG)" == "c4group - Win32 Debug"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\standard\lib\StandardD.lib

!IF  "$(CFG)" == "c4group - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "c4group - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\Standard\openssl\libeay32.lib

!IF  "$(CFG)" == "c4group - Win32 Release"

!ELSEIF  "$(CFG)" == "c4group - Win32 Debug"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\Standard\openssl\libeay32_d.lib

!IF  "$(CFG)" == "c4group - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "c4group - Win32 Debug"

!ENDIF 

# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Resource.rc
# End Source File
# End Group
# End Target
# End Project
