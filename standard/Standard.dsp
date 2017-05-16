# Microsoft Developer Studio Project File - Name="Standard" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=Standard - Win32 NoNetwork Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "Standard.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "Standard.mak" CFG="Standard - Win32 NoNetwork Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "Standard - Win32 Release Editor" (based on "Win32 (x86) Static Library")
!MESSAGE "Standard - Win32 Debug Editor" (based on "Win32 (x86) Static Library")
!MESSAGE "Standard - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE "Standard - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "Standard - Win32 Extreme Debug Editor" (based on "Win32 (x86) Static Library")
!MESSAGE "Standard - Win32 Debug Console" (based on "Win32 (x86) Static Library")
!MESSAGE "Standard - Win32 Release Console" (based on "Win32 (x86) Static Library")
!MESSAGE "Standard - Win32 NoNetwork Release" (based on "Win32 (x86) Static Library")
!MESSAGE "Standard - Win32 NoNetwork Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "Standard - Win32 Release Editor"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Standard___Win32_Release_Frontend"
# PROP BASE Intermediate_Dir "Standard___Win32_Release_Frontend"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "..\intermediate\vc6\StandardFE\Release"
# PROP Intermediate_Dir "..\intermediate\vc6\StandardFE\Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /I "./inc" /I "./zlib" /I "./lpng121" /I "./" /D "NDEBUG" /D "OLD_GFX" /D "WIN32" /D "_MBCS" /D "_LIB" /D "C4ENGINE" /FR /YX /FD /c
# ADD CPP /nologo /MT /W3 /GX /O2 /I "./inc" /I "./zlib" /I "./lpng121" /I "./freetype2" /I "./" /D "NDEBUG" /D "C4FRONTEND" /D "GLEW_STATIC" /D "WIN32" /D "_MBCS" /D "_LIB" /D "C4MULTIMONITOR" /D "FT2_BUILD_LIBRARY" /FR /FD /c
# ADD BASE RSC /l 0x407 /d "NDEBUG"
# ADD RSC /l 0x407 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo /out:".\lib\StandardFE.lib"
# ADD LIB32 /nologo /out:".\lib\StandardFE.lib"

!ELSEIF  "$(CFG)" == "Standard - Win32 Debug Editor"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Standard___Win32_Debug_Frontend"
# PROP BASE Intermediate_Dir "Standard___Win32_Debug_Frontend"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "..\intermediate\vc6\StandardFE\Debug"
# PROP Intermediate_Dir "..\intermediate\vc6\StandardFE\Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /I "./inc" /I "./zlib" /I "./lpng121" /D "_DEBUG" /D "OLD_GFX" /D "WIN32" /D "_MBCS" /D "_LIB" /D "C4ENGINE" /YX /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /I "./gl" /I "./inc" /I "./zlib" /I "./lpng121" /I "./freetype2" /I "./" /D "_DEBUG" /D "_WINDOWS" /D "C4FRONTEND" /D "GLEW_STATIC" /D "WIN32" /D "_MBCS" /D "_LIB" /D "C4MULTIMONITOR" /D "FT2_BUILD_LIBRARY" /FR /FD /GZ /c
# ADD BASE RSC /l 0x407 /d "_DEBUG"
# ADD RSC /l 0x407 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo /out:".\lib\StandardDFE.lib"
# ADD LIB32 /nologo /out:".\lib\StandardDFE.lib"

!ELSEIF  "$(CFG)" == "Standard - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Standard___Win32_Debug"
# PROP BASE Intermediate_Dir "Standard___Win32_Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "..\intermediate\vc6\Standard\Debug"
# PROP Intermediate_Dir "..\intermediate\vc6\Standard\Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /I "./inc" /I "./zlib" /I "./lpng121" /I "./" /D "_DEBUG" /D "C4ENGINE" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "NEW_GFX" /D "_LIB" /YX /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /I "./inc" /I "./zlib" /I "./lpng121" /I "./freetype2" /I "./" /D "_DEBUG" /D "C4ENGINE" /D "_WINDOWS" /D "USE_GL" /D "USE_DIRECTX" /D "GLEW_STATIC" /D "WIN32" /D "_MBCS" /D "_LIB" /D "C4MULTIMONITOR" /D "FT2_BUILD_LIBRARY" /FR /FD /GZ /c
# ADD BASE RSC /l 0x407 /d "_DEBUG"
# ADD RSC /l 0x407 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo /out:".\lib\StandardD.lib"
# ADD LIB32 /nologo /out:".\lib\StandardD.lib"

!ELSEIF  "$(CFG)" == "Standard - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Standard___Win32_Release"
# PROP BASE Intermediate_Dir "Standard___Win32_Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "..\intermediate\vc6\Standard\Release"
# PROP Intermediate_Dir "..\intermediate\vc6\Standard\Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /I "./inc" /I "./zlib" /I "./lpng121" /I "./" /D "NDEBUG" /D "NEW_GFX" /D "WIN32" /D "_MBCS" /D "_LIB" /D "C4ENGINE" /FR /YX /FD /c
# ADD CPP /nologo /MT /W3 /GX /O2 /I "./inc" /I "./zlib" /I "./lpng121" /I "./freetype2" /I "./" /D "NDEBUG" /D "C4ENGINE" /D "USE_DIRECTX" /D "USE_GL" /D "GLEW_STATIC" /D "WIN32" /D "_MBCS" /D "_LIB" /D "C4MULTIMONITOR" /D "FT2_BUILD_LIBRARY" /FR /FD /c
# ADD BASE RSC /l 0x407 /d "NDEBUG"
# ADD RSC /l 0x407 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo /out:".\lib\Standard.lib"
# ADD LIB32 /nologo /out:".\lib\Standard.lib"

!ELSEIF  "$(CFG)" == "Standard - Win32 Extreme Debug Editor"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Standard___Win32_Extreme_Debug_Frontend"
# PROP BASE Intermediate_Dir "Standard___Win32_Extreme_Debug_Frontend"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "..\intermediate\vc6\StandardFE\ExtremeDebug"
# PROP Intermediate_Dir "..\intermediate\vc6\StandardFE\ExtremeDebug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /I "./inc" /I "./zlib" /I "./lpng121" /I "./" /D "NDEBUG" /D "WIN32" /D "_MBCS" /D "_LIB" /D "NEW_GFX" /FR /YX /FD /c
# ADD CPP /nologo /MT /W3 /GX /Zi /O2 /I "./inc" /I "./zlib" /I "./lpng121" /I "./freetype2" /I "./" /D "NDEBUG" /D "C4FRONTEND" /D "GLEW_STATIC" /D "WIN32" /D "_MBCS" /D "_LIB" /D "C4MULTIMONITOR" /D "FT2_BUILD_LIBRARY" /FR /FD /c
# ADD BASE RSC /l 0x407 /d "NDEBUG"
# ADD RSC /l 0x407 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo /out:".\lib\StandardFE.lib"
# ADD LIB32 /nologo /out:".\lib\StandardXDFE.lib"

!ELSEIF  "$(CFG)" == "Standard - Win32 Debug Console"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Standard___Win32_Debug_Console"
# PROP BASE Intermediate_Dir "Standard___Win32_Debug_Console"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "..\intermediate\vc6\StandardConsole\Debug"
# PROP Intermediate_Dir "..\intermediate\vc6\StandardConsole\Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /I "./inc" /I "./zlib" /I "./lpng121" /I "./" /D "_DEBUG" /D "C4ENGINE" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "_LIB" /D "USE_DIRECTX" /FR /YX /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /I "./inc" /I "./zlib" /I "./lpng121" /I "./freetype2" /I "./" /D "_DEBUG" /D "C4ENGINE" /D "_WINDOWS" /D "USE_CONSOLE" /D "GLEW_STATIC" /D "WIN32" /D "_MBCS" /D "_LIB" /D "C4MULTIMONITOR" /D "FT2_BUILD_LIBRARY" /FR /FD /GZ /c
# ADD BASE RSC /l 0x407 /d "_DEBUG"
# ADD RSC /l 0x407 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo /out:".\lib\StandardDCon.lib"
# ADD LIB32 /nologo /out:".\lib\StandardDCon.lib"

!ELSEIF  "$(CFG)" == "Standard - Win32 Release Console"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Standard___Win32_Release_Console"
# PROP BASE Intermediate_Dir "Standard___Win32_Release_Console"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "..\intermediate\vc6\StandardConsole\Release"
# PROP Intermediate_Dir "..\intermediate\vc6\StandardConsole\Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /I "./inc" /I "./zlib" /I "./lpng121" /I "./" /D "NDEBUG" /D "WIN32" /D "_MBCS" /D "_LIB" /D "C4ENGINE" /FR /YX /FD /c
# ADD CPP /nologo /MT /W3 /GX /O2 /I "./inc" /I "./zlib" /I "./lpng121" /I "./freetype2" /I "./" /D "NDEBUG" /D "C4ENGINE" /D "USE_CONSOLE" /D "GLEW_STATIC" /D "WIN32" /D "_MBCS" /D "_LIB" /D "C4MULTIMONITOR" /D "FT2_BUILD_LIBRARY" /FR /FD /c
# ADD BASE RSC /l 0x407 /d "NDEBUG"
# ADD RSC /l 0x407 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo /out:".\lib\StandardCon.lib"
# ADD LIB32 /nologo /out:".\lib\StandardCon.lib"

!ELSEIF  "$(CFG)" == "Standard - Win32 NoNetwork Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Standard___Win32_NoNetwork_Release"
# PROP BASE Intermediate_Dir "Standard___Win32_NoNetwork_Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Standard___Win32_NoNetwork_Release"
# PROP Intermediate_Dir "Standard___Win32_NoNetwork_Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /I "./inc" /I "./zlib" /I "./lpng121" /I "./freetype2" /I "./" /D "NDEBUG" /D "C4ENGINE" /D "USE_DIRECTX" /D "USE_GL" /D "GLEW_STATIC" /D "WIN32" /D "_MBCS" /D "_LIB" /D "C4MULTIMONITOR" /D "FT2_BUILD_LIBRARY" /FR /FD /c
# ADD CPP /nologo /MT /W3 /GX /O2 /I "./inc" /I "./zlib" /I "./lpng121" /I "./freetype2" /I "./" /D "NDEBUG" /D "C4ENGINE" /D "USE_DIRECTX" /D "USE_GL" /D "GLEW_STATIC" /D "WIN32" /D "_MBCS" /D "_LIB" /D "C4MULTIMONITOR" /D "FT2_BUILD_LIBRARY" /FR /FD /c
# ADD BASE RSC /l 0x407 /d "NDEBUG"
# ADD RSC /l 0x407 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo /out:".\lib\Standard.lib"
# ADD LIB32 /nologo /out:".\lib\Standard.lib"

!ELSEIF  "$(CFG)" == "Standard - Win32 NoNetwork Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Standard___Win32_NoNetwork_Debug"
# PROP BASE Intermediate_Dir "Standard___Win32_NoNetwork_Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Standard___Win32_NoNetwork_Debug"
# PROP Intermediate_Dir "Standard___Win32_NoNetwork_Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /I "./inc" /I "./zlib" /I "./lpng121" /I "./freetype2" /I "./" /D "_DEBUG" /D "C4ENGINE" /D "_WINDOWS" /D "USE_GL" /D "USE_DIRECTX" /D "GLEW_STATIC" /D "WIN32" /D "_MBCS" /D "_LIB" /D "C4MULTIMONITOR" /D "FT2_BUILD_LIBRARY" /FR /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /I "./inc" /I "./zlib" /I "./lpng121" /I "./freetype2" /I "./" /D "_DEBUG" /D "C4ENGINE" /D "_WINDOWS" /D "USE_GL" /D "USE_DIRECTX" /D "GLEW_STATIC" /D "WIN32" /D "_MBCS" /D "_LIB" /D "C4MULTIMONITOR" /D "FT2_BUILD_LIBRARY" /FR /FD /GZ /c
# ADD BASE RSC /l 0x407 /d "_DEBUG"
# ADD RSC /l 0x407 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo /out:".\lib\StandardD.lib"
# ADD LIB32 /nologo /out:".\lib\StandardD.lib"

!ENDIF 

# Begin Target

# Name "Standard - Win32 Release Editor"
# Name "Standard - Win32 Debug Editor"
# Name "Standard - Win32 Debug"
# Name "Standard - Win32 Release"
# Name "Standard - Win32 Extreme Debug Editor"
# Name "Standard - Win32 Debug Console"
# Name "Standard - Win32 Release Console"
# Name "Standard - Win32 NoNetwork Release"
# Name "Standard - Win32 NoNetwork Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Group "LibPng"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\lpng121\png.c
# End Source File
# Begin Source File

SOURCE=.\lpng121\pngerror.c
# End Source File
# Begin Source File

SOURCE=.\lpng121\pnggccrd.c
# End Source File
# Begin Source File

SOURCE=.\lpng121\pngget.c
# End Source File
# Begin Source File

SOURCE=.\lpng121\pngmem.c
# End Source File
# Begin Source File

SOURCE=.\lpng121\pngpread.c
# End Source File
# Begin Source File

SOURCE=.\lpng121\pngread.c
# End Source File
# Begin Source File

SOURCE=.\lpng121\pngrio.c
# End Source File
# Begin Source File

SOURCE=.\lpng121\pngrtran.c
# End Source File
# Begin Source File

SOURCE=.\lpng121\pngrutil.c
# End Source File
# Begin Source File

SOURCE=.\lpng121\pngset.c
# End Source File
# Begin Source File

SOURCE=.\lpng121\pngtrans.c
# End Source File
# Begin Source File

SOURCE=.\lpng121\pngvcrd.c
# End Source File
# Begin Source File

SOURCE=.\lpng121\pngwio.c
# End Source File
# Begin Source File

SOURCE=.\lpng121\pngwrite.c
# End Source File
# Begin Source File

SOURCE=.\lpng121\pngwtran.c
# End Source File
# Begin Source File

SOURCE=.\lpng121\pngwutil.c
# End Source File
# End Group
# Begin Group "DirectX"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\d3dutil.cpp

!IF  "$(CFG)" == "Standard - Win32 Release Editor"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Standard - Win32 Debug Editor"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Standard - Win32 Debug"

!ELSEIF  "$(CFG)" == "Standard - Win32 Release"

!ELSEIF  "$(CFG)" == "Standard - Win32 Extreme Debug Editor"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Standard - Win32 Debug Console"

!ELSEIF  "$(CFG)" == "Standard - Win32 Release Console"

!ELSEIF  "$(CFG)" == "Standard - Win32 NoNetwork Release"

!ELSEIF  "$(CFG)" == "Standard - Win32 NoNetwork Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\dxutil.cpp

!IF  "$(CFG)" == "Standard - Win32 Release Editor"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Standard - Win32 Debug Editor"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Standard - Win32 Debug"

!ELSEIF  "$(CFG)" == "Standard - Win32 Release"

!ELSEIF  "$(CFG)" == "Standard - Win32 Extreme Debug Editor"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Standard - Win32 Debug Console"

!ELSEIF  "$(CFG)" == "Standard - Win32 Release Console"

!ELSEIF  "$(CFG)" == "Standard - Win32 NoNetwork Release"

!ELSEIF  "$(CFG)" == "Standard - Win32 NoNetwork Debug"

!ENDIF 

# End Source File
# End Group
# Begin Group "ZLib"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\zlib\adler32.c
# End Source File
# Begin Source File

SOURCE=.\zlib\compress.c
# End Source File
# Begin Source File

SOURCE=.\zlib\crc32.c
# End Source File
# Begin Source File

SOURCE=.\zlib\crc32.h
# End Source File
# Begin Source File

SOURCE=.\zlib\deflate.c
# End Source File
# Begin Source File

SOURCE=.\zlib\deflate.h
# End Source File
# Begin Source File

SOURCE=.\zlib\gzio.c
# End Source File
# Begin Source File

SOURCE=.\zlib\infback.c
# End Source File
# Begin Source File

SOURCE=.\zlib\infblock.h
# End Source File
# Begin Source File

SOURCE=.\zlib\infcodes.h
# End Source File
# Begin Source File

SOURCE=.\zlib\inffast.c
# End Source File
# Begin Source File

SOURCE=.\zlib\inffast.h
# End Source File
# Begin Source File

SOURCE=.\zlib\inffixed.h
# End Source File
# Begin Source File

SOURCE=.\zlib\inflate.c
# End Source File
# Begin Source File

SOURCE=.\zlib\inflate.h
# End Source File
# Begin Source File

SOURCE=.\zlib\inftrees.c
# End Source File
# Begin Source File

SOURCE=.\zlib\inftrees.h
# End Source File
# Begin Source File

SOURCE=.\zlib\trees.c
# End Source File
# Begin Source File

SOURCE=.\zlib\trees.h
# End Source File
# Begin Source File

SOURCE=.\zlib\uncompr.c
# End Source File
# Begin Source File

SOURCE=.\zlib\zconf.h
# End Source File
# Begin Source File

SOURCE=.\zlib\ZLib.h
# End Source File
# Begin Source File

SOURCE=.\zlib\zutil.c
# End Source File
# Begin Source File

SOURCE=.\zlib\zutil.h
# End Source File
# End Group
# Begin Group "gl"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\gl\glew.c
# End Source File
# End Group
# Begin Group "LibJpeg"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\jpeglib\jcapimin.c
# End Source File
# Begin Source File

SOURCE=.\jpeglib\jcapistd.c
# End Source File
# Begin Source File

SOURCE=.\jpeglib\jccoefct.c
# End Source File
# Begin Source File

SOURCE=.\jpeglib\jccolor.c
# End Source File
# Begin Source File

SOURCE=.\jpeglib\jcdctmgr.c
# End Source File
# Begin Source File

SOURCE=.\jpeglib\jchuff.c
# End Source File
# Begin Source File

SOURCE=.\jpeglib\jcinit.c
# End Source File
# Begin Source File

SOURCE=.\jpeglib\jcmainct.c
# End Source File
# Begin Source File

SOURCE=.\jpeglib\jcmarker.c
# End Source File
# Begin Source File

SOURCE=.\jpeglib\jcmaster.c
# End Source File
# Begin Source File

SOURCE=.\jpeglib\jcomapi.c
# End Source File
# Begin Source File

SOURCE=.\jpeglib\jcparam.c
# End Source File
# Begin Source File

SOURCE=.\jpeglib\jcphuff.c
# End Source File
# Begin Source File

SOURCE=.\jpeglib\jcprepct.c
# End Source File
# Begin Source File

SOURCE=.\jpeglib\jcsample.c
# End Source File
# Begin Source File

SOURCE=.\jpeglib\jctrans.c
# End Source File
# Begin Source File

SOURCE=.\jpeglib\jdapimin.c
# End Source File
# Begin Source File

SOURCE=.\jpeglib\jdapistd.c
# End Source File
# Begin Source File

SOURCE=.\jpeglib\jdatadst.c
# End Source File
# Begin Source File

SOURCE=.\jpeglib\jdatasrc.c
# End Source File
# Begin Source File

SOURCE=.\jpeglib\jdcoefct.c
# End Source File
# Begin Source File

SOURCE=.\jpeglib\jdcolor.c
# End Source File
# Begin Source File

SOURCE=.\jpeglib\jddctmgr.c
# End Source File
# Begin Source File

SOURCE=.\jpeglib\jdhuff.c
# End Source File
# Begin Source File

SOURCE=.\jpeglib\jdinput.c
# End Source File
# Begin Source File

SOURCE=.\jpeglib\jdmainct.c
# End Source File
# Begin Source File

SOURCE=.\jpeglib\jdmarker.c
# End Source File
# Begin Source File

SOURCE=.\jpeglib\jdmaster.c
# End Source File
# Begin Source File

SOURCE=.\jpeglib\jdmerge.c
# End Source File
# Begin Source File

SOURCE=.\jpeglib\jdphuff.c
# End Source File
# Begin Source File

SOURCE=.\jpeglib\jdpostct.c
# End Source File
# Begin Source File

SOURCE=.\jpeglib\jdsample.c
# End Source File
# Begin Source File

SOURCE=.\jpeglib\jdtrans.c
# End Source File
# Begin Source File

SOURCE=.\jpeglib\jerror.c
# End Source File
# Begin Source File

SOURCE=.\jpeglib\jfdctflt.c
# End Source File
# Begin Source File

SOURCE=.\jpeglib\jfdctfst.c
# End Source File
# Begin Source File

SOURCE=.\jpeglib\jfdctint.c
# End Source File
# Begin Source File

SOURCE=.\jpeglib\jidctflt.c
# End Source File
# Begin Source File

SOURCE=.\jpeglib\jidctfst.c
# End Source File
# Begin Source File

SOURCE=.\jpeglib\jidctint.c
# End Source File
# Begin Source File

SOURCE=.\jpeglib\jidctred.c
# End Source File
# Begin Source File

SOURCE=.\jpeglib\jmemansi.c
# End Source File
# Begin Source File

SOURCE=.\jpeglib\jmemmgr.c
# End Source File
# Begin Source File

SOURCE=.\jpeglib\jquant1.c
# End Source File
# Begin Source File

SOURCE=.\jpeglib\jquant2.c
# End Source File
# Begin Source File

SOURCE=.\jpeglib\jutils.c
# End Source File
# End Group
# Begin Group "Freetype"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\freetype2\src\autofit\autofit.c

!IF  "$(CFG)" == "Standard - Win32 Release Editor"

# PROP Exclude_From_Build 1
# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "Standard - Win32 Debug Editor"

# PROP Exclude_From_Build 1
# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "Standard - Win32 Debug"

!ELSEIF  "$(CFG)" == "Standard - Win32 Release"

!ELSEIF  "$(CFG)" == "Standard - Win32 Extreme Debug Editor"

# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "Standard - Win32 Debug Console"

# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "Standard - Win32 Release Console"

# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "Standard - Win32 NoNetwork Release"

!ELSEIF  "$(CFG)" == "Standard - Win32 NoNetwork Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\freetype2\src\bdf\bdf.c

!IF  "$(CFG)" == "Standard - Win32 Release Editor"

# PROP Exclude_From_Build 1
# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "Standard - Win32 Debug Editor"

# PROP Exclude_From_Build 1
# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "Standard - Win32 Debug"

!ELSEIF  "$(CFG)" == "Standard - Win32 Release"

!ELSEIF  "$(CFG)" == "Standard - Win32 Extreme Debug Editor"

# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "Standard - Win32 Debug Console"

# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "Standard - Win32 Release Console"

# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "Standard - Win32 NoNetwork Release"

!ELSEIF  "$(CFG)" == "Standard - Win32 NoNetwork Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\freetype2\src\cff\cff.c

!IF  "$(CFG)" == "Standard - Win32 Release Editor"

# PROP Exclude_From_Build 1
# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "Standard - Win32 Debug Editor"

# PROP Exclude_From_Build 1
# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "Standard - Win32 Debug"

!ELSEIF  "$(CFG)" == "Standard - Win32 Release"

!ELSEIF  "$(CFG)" == "Standard - Win32 Extreme Debug Editor"

# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "Standard - Win32 Debug Console"

# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "Standard - Win32 Release Console"

# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "Standard - Win32 NoNetwork Release"

!ELSEIF  "$(CFG)" == "Standard - Win32 NoNetwork Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\freetype2\src\base\ftbase.c

!IF  "$(CFG)" == "Standard - Win32 Release Editor"

# PROP Exclude_From_Build 1
# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "Standard - Win32 Debug Editor"

# PROP Exclude_From_Build 1
# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "Standard - Win32 Debug"

!ELSEIF  "$(CFG)" == "Standard - Win32 Release"

!ELSEIF  "$(CFG)" == "Standard - Win32 Extreme Debug Editor"

# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "Standard - Win32 Debug Console"

# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "Standard - Win32 Release Console"

# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "Standard - Win32 NoNetwork Release"

!ELSEIF  "$(CFG)" == "Standard - Win32 NoNetwork Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\freetype2\src\base\ftbitmap.c

!IF  "$(CFG)" == "Standard - Win32 Release Editor"

# PROP Exclude_From_Build 1
# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "Standard - Win32 Debug Editor"

# PROP Exclude_From_Build 1
# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "Standard - Win32 Debug"

!ELSEIF  "$(CFG)" == "Standard - Win32 Release"

!ELSEIF  "$(CFG)" == "Standard - Win32 Extreme Debug Editor"

# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "Standard - Win32 Debug Console"

# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "Standard - Win32 Release Console"

# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "Standard - Win32 NoNetwork Release"

!ELSEIF  "$(CFG)" == "Standard - Win32 NoNetwork Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\freetype2\src\cache\ftcache.c

!IF  "$(CFG)" == "Standard - Win32 Release Editor"

# PROP Exclude_From_Build 1
# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "Standard - Win32 Debug Editor"

# PROP Exclude_From_Build 1
# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "Standard - Win32 Debug"

!ELSEIF  "$(CFG)" == "Standard - Win32 Release"

!ELSEIF  "$(CFG)" == "Standard - Win32 Extreme Debug Editor"

# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "Standard - Win32 Debug Console"

# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "Standard - Win32 Release Console"

# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "Standard - Win32 NoNetwork Release"

!ELSEIF  "$(CFG)" == "Standard - Win32 NoNetwork Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\freetype2\src\base\ftglyph.c

!IF  "$(CFG)" == "Standard - Win32 Release Editor"

# PROP Exclude_From_Build 1
# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "Standard - Win32 Debug Editor"

# PROP Exclude_From_Build 1
# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "Standard - Win32 Debug"

!ELSEIF  "$(CFG)" == "Standard - Win32 Release"

!ELSEIF  "$(CFG)" == "Standard - Win32 Extreme Debug Editor"

# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "Standard - Win32 Debug Console"

# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "Standard - Win32 Release Console"

# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "Standard - Win32 NoNetwork Release"

!ELSEIF  "$(CFG)" == "Standard - Win32 NoNetwork Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\freetype2\src\gzip\ftgzip.c

!IF  "$(CFG)" == "Standard - Win32 Release Editor"

# PROP Exclude_From_Build 1
# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "Standard - Win32 Debug Editor"

# PROP Exclude_From_Build 1
# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "Standard - Win32 Debug"

!ELSEIF  "$(CFG)" == "Standard - Win32 Release"

!ELSEIF  "$(CFG)" == "Standard - Win32 Extreme Debug Editor"

# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "Standard - Win32 Debug Console"

# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "Standard - Win32 Release Console"

# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "Standard - Win32 NoNetwork Release"

!ELSEIF  "$(CFG)" == "Standard - Win32 NoNetwork Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\freetype2\src\base\ftinit.c

!IF  "$(CFG)" == "Standard - Win32 Release Editor"

# PROP Exclude_From_Build 1
# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "Standard - Win32 Debug Editor"

# PROP Exclude_From_Build 1
# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "Standard - Win32 Debug"

!ELSEIF  "$(CFG)" == "Standard - Win32 Release"

!ELSEIF  "$(CFG)" == "Standard - Win32 Extreme Debug Editor"

# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "Standard - Win32 Debug Console"

# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "Standard - Win32 Release Console"

# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "Standard - Win32 NoNetwork Release"

!ELSEIF  "$(CFG)" == "Standard - Win32 NoNetwork Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\freetype2\src\lzw\ftlzw.c

!IF  "$(CFG)" == "Standard - Win32 Release Editor"

# PROP Exclude_From_Build 1
# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "Standard - Win32 Debug Editor"

# PROP Exclude_From_Build 1
# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "Standard - Win32 Debug"

!ELSEIF  "$(CFG)" == "Standard - Win32 Release"

!ELSEIF  "$(CFG)" == "Standard - Win32 Extreme Debug Editor"

# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "Standard - Win32 Debug Console"

# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "Standard - Win32 Release Console"

# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "Standard - Win32 NoNetwork Release"

!ELSEIF  "$(CFG)" == "Standard - Win32 NoNetwork Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\freetype2\src\base\ftmm.c

!IF  "$(CFG)" == "Standard - Win32 Release Editor"

# PROP Exclude_From_Build 1
# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "Standard - Win32 Debug Editor"

# PROP Exclude_From_Build 1
# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "Standard - Win32 Debug"

!ELSEIF  "$(CFG)" == "Standard - Win32 Release"

!ELSEIF  "$(CFG)" == "Standard - Win32 Extreme Debug Editor"

# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "Standard - Win32 Debug Console"

# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "Standard - Win32 Release Console"

# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "Standard - Win32 NoNetwork Release"

!ELSEIF  "$(CFG)" == "Standard - Win32 NoNetwork Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\freetype2\src\base\ftsystem.c

!IF  "$(CFG)" == "Standard - Win32 Release Editor"

# PROP Exclude_From_Build 1
# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "Standard - Win32 Debug Editor"

# PROP Exclude_From_Build 1
# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "Standard - Win32 Debug"

!ELSEIF  "$(CFG)" == "Standard - Win32 Release"

!ELSEIF  "$(CFG)" == "Standard - Win32 Extreme Debug Editor"

# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "Standard - Win32 Debug Console"

# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "Standard - Win32 Release Console"

# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "Standard - Win32 NoNetwork Release"

!ELSEIF  "$(CFG)" == "Standard - Win32 NoNetwork Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\freetype2\src\otvalid\otvalid.c

!IF  "$(CFG)" == "Standard - Win32 Release Editor"

# PROP Exclude_From_Build 1
# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "Standard - Win32 Debug Editor"

# PROP Exclude_From_Build 1
# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "Standard - Win32 Debug"

!ELSEIF  "$(CFG)" == "Standard - Win32 Release"

!ELSEIF  "$(CFG)" == "Standard - Win32 Extreme Debug Editor"

# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "Standard - Win32 Debug Console"

# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "Standard - Win32 Release Console"

# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "Standard - Win32 NoNetwork Release"

!ELSEIF  "$(CFG)" == "Standard - Win32 NoNetwork Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\freetype2\src\pcf\pcf.c

!IF  "$(CFG)" == "Standard - Win32 Release Editor"

# PROP Exclude_From_Build 1
# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "Standard - Win32 Debug Editor"

# PROP Exclude_From_Build 1
# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "Standard - Win32 Debug"

!ELSEIF  "$(CFG)" == "Standard - Win32 Release"

!ELSEIF  "$(CFG)" == "Standard - Win32 Extreme Debug Editor"

# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "Standard - Win32 Debug Console"

# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "Standard - Win32 Release Console"

# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "Standard - Win32 NoNetwork Release"

!ELSEIF  "$(CFG)" == "Standard - Win32 NoNetwork Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\freetype2\src\pfr\pfr.c

!IF  "$(CFG)" == "Standard - Win32 Release Editor"

# PROP Exclude_From_Build 1
# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "Standard - Win32 Debug Editor"

# PROP Exclude_From_Build 1
# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "Standard - Win32 Debug"

!ELSEIF  "$(CFG)" == "Standard - Win32 Release"

!ELSEIF  "$(CFG)" == "Standard - Win32 Extreme Debug Editor"

# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "Standard - Win32 Debug Console"

# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "Standard - Win32 Release Console"

# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "Standard - Win32 NoNetwork Release"

!ELSEIF  "$(CFG)" == "Standard - Win32 NoNetwork Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\freetype2\src\psaux\psaux.c

!IF  "$(CFG)" == "Standard - Win32 Release Editor"

# PROP Exclude_From_Build 1
# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "Standard - Win32 Debug Editor"

# PROP Exclude_From_Build 1
# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "Standard - Win32 Debug"

!ELSEIF  "$(CFG)" == "Standard - Win32 Release"

!ELSEIF  "$(CFG)" == "Standard - Win32 Extreme Debug Editor"

# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "Standard - Win32 Debug Console"

# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "Standard - Win32 Release Console"

# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "Standard - Win32 NoNetwork Release"

!ELSEIF  "$(CFG)" == "Standard - Win32 NoNetwork Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\freetype2\src\pshinter\pshinter.c

!IF  "$(CFG)" == "Standard - Win32 Release Editor"

# PROP Exclude_From_Build 1
# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "Standard - Win32 Debug Editor"

# PROP Exclude_From_Build 1
# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "Standard - Win32 Debug"

!ELSEIF  "$(CFG)" == "Standard - Win32 Release"

!ELSEIF  "$(CFG)" == "Standard - Win32 Extreme Debug Editor"

# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "Standard - Win32 Debug Console"

# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "Standard - Win32 Release Console"

# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "Standard - Win32 NoNetwork Release"

!ELSEIF  "$(CFG)" == "Standard - Win32 NoNetwork Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\freetype2\src\psnames\psnames.c

!IF  "$(CFG)" == "Standard - Win32 Release Editor"

# PROP Exclude_From_Build 1
# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "Standard - Win32 Debug Editor"

# PROP Exclude_From_Build 1
# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "Standard - Win32 Debug"

!ELSEIF  "$(CFG)" == "Standard - Win32 Release"

!ELSEIF  "$(CFG)" == "Standard - Win32 Extreme Debug Editor"

# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "Standard - Win32 Debug Console"

# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "Standard - Win32 Release Console"

# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "Standard - Win32 NoNetwork Release"

!ELSEIF  "$(CFG)" == "Standard - Win32 NoNetwork Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\freetype2\src\raster\raster.c

!IF  "$(CFG)" == "Standard - Win32 Release Editor"

# PROP Exclude_From_Build 1
# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "Standard - Win32 Debug Editor"

# PROP Exclude_From_Build 1
# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "Standard - Win32 Debug"

!ELSEIF  "$(CFG)" == "Standard - Win32 Release"

!ELSEIF  "$(CFG)" == "Standard - Win32 Extreme Debug Editor"

# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "Standard - Win32 Debug Console"

# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "Standard - Win32 Release Console"

# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "Standard - Win32 NoNetwork Release"

!ELSEIF  "$(CFG)" == "Standard - Win32 NoNetwork Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\freetype2\src\sfnt\sfnt.c

!IF  "$(CFG)" == "Standard - Win32 Release Editor"

# PROP Exclude_From_Build 1
# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "Standard - Win32 Debug Editor"

# PROP Exclude_From_Build 1
# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "Standard - Win32 Debug"

!ELSEIF  "$(CFG)" == "Standard - Win32 Release"

!ELSEIF  "$(CFG)" == "Standard - Win32 Extreme Debug Editor"

# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "Standard - Win32 Debug Console"

# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "Standard - Win32 Release Console"

# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "Standard - Win32 NoNetwork Release"

!ELSEIF  "$(CFG)" == "Standard - Win32 NoNetwork Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\freetype2\src\smooth\smooth.c

!IF  "$(CFG)" == "Standard - Win32 Release Editor"

# PROP Exclude_From_Build 1
# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "Standard - Win32 Debug Editor"

# PROP Exclude_From_Build 1
# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "Standard - Win32 Debug"

!ELSEIF  "$(CFG)" == "Standard - Win32 Release"

!ELSEIF  "$(CFG)" == "Standard - Win32 Extreme Debug Editor"

# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "Standard - Win32 Debug Console"

# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "Standard - Win32 Release Console"

# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "Standard - Win32 NoNetwork Release"

!ELSEIF  "$(CFG)" == "Standard - Win32 NoNetwork Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\freetype2\src\truetype\truetype.c

!IF  "$(CFG)" == "Standard - Win32 Release Editor"

# PROP Exclude_From_Build 1
# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "Standard - Win32 Debug Editor"

# PROP Exclude_From_Build 1
# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "Standard - Win32 Debug"

!ELSEIF  "$(CFG)" == "Standard - Win32 Release"

!ELSEIF  "$(CFG)" == "Standard - Win32 Extreme Debug Editor"

# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "Standard - Win32 Debug Console"

# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "Standard - Win32 Release Console"

# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "Standard - Win32 NoNetwork Release"

!ELSEIF  "$(CFG)" == "Standard - Win32 NoNetwork Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\freetype2\src\type1\type1.c

!IF  "$(CFG)" == "Standard - Win32 Release Editor"

# PROP Exclude_From_Build 1
# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "Standard - Win32 Debug Editor"

# PROP Exclude_From_Build 1
# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "Standard - Win32 Debug"

!ELSEIF  "$(CFG)" == "Standard - Win32 Release"

!ELSEIF  "$(CFG)" == "Standard - Win32 Extreme Debug Editor"

# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "Standard - Win32 Debug Console"

# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "Standard - Win32 Release Console"

# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "Standard - Win32 NoNetwork Release"

!ELSEIF  "$(CFG)" == "Standard - Win32 NoNetwork Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\freetype2\src\cid\type1cid.c

!IF  "$(CFG)" == "Standard - Win32 Release Editor"

# PROP Exclude_From_Build 1
# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "Standard - Win32 Debug Editor"

# PROP Exclude_From_Build 1
# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "Standard - Win32 Debug"

!ELSEIF  "$(CFG)" == "Standard - Win32 Release"

!ELSEIF  "$(CFG)" == "Standard - Win32 Extreme Debug Editor"

# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "Standard - Win32 Debug Console"

# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "Standard - Win32 Release Console"

# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "Standard - Win32 NoNetwork Release"

!ELSEIF  "$(CFG)" == "Standard - Win32 NoNetwork Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\freetype2\src\type42\type42.c

!IF  "$(CFG)" == "Standard - Win32 Release Editor"

# PROP Exclude_From_Build 1
# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "Standard - Win32 Debug Editor"

# PROP Exclude_From_Build 1
# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "Standard - Win32 Debug"

!ELSEIF  "$(CFG)" == "Standard - Win32 Release"

!ELSEIF  "$(CFG)" == "Standard - Win32 Extreme Debug Editor"

# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "Standard - Win32 Debug Console"

# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "Standard - Win32 Release Console"

# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "Standard - Win32 NoNetwork Release"

!ELSEIF  "$(CFG)" == "Standard - Win32 NoNetwork Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\freetype2\src\winfonts\winfnt.c

!IF  "$(CFG)" == "Standard - Win32 Release Editor"

# PROP Exclude_From_Build 1
# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "Standard - Win32 Debug Editor"

# PROP Exclude_From_Build 1
# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "Standard - Win32 Debug"

!ELSEIF  "$(CFG)" == "Standard - Win32 Release"

!ELSEIF  "$(CFG)" == "Standard - Win32 Extreme Debug Editor"

# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "Standard - Win32 Debug Console"

# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "Standard - Win32 Release Console"

# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "Standard - Win32 NoNetwork Release"

!ELSEIF  "$(CFG)" == "Standard - Win32 NoNetwork Debug"

!ENDIF 

# End Source File
# End Group
# Begin Source File

SOURCE=.\src\Bitmap256.cpp
# End Source File
# Begin Source File

SOURCE=.\src\CStdFile.cpp
# End Source File
# Begin Source File

SOURCE=.\src\DInputX.cpp
# End Source File
# Begin Source File

SOURCE=.\src\DSoundX.cpp
# End Source File
# Begin Source File

SOURCE=.\src\Fixed.cpp
# End Source File
# Begin Source File

SOURCE=.\src\Midi.cpp
# End Source File
# Begin Source File

SOURCE=.\src\OpenURL.cpp
# End Source File
# Begin Source File

SOURCE=.\src\PathFinder.cpp
# End Source File
# Begin Source File

SOURCE=.\src\Standard.cpp
# End Source File
# Begin Source File

SOURCE=.\src\StdBase64.cpp
# End Source File
# Begin Source File

SOURCE=.\src\StdBitmap.cpp
# End Source File
# Begin Source File

SOURCE=.\src\StdBuf.cpp
# End Source File
# Begin Source File

SOURCE=.\src\StdCompiler.cpp
# End Source File
# Begin Source File

SOURCE=.\src\StdConfig.cpp
# End Source File
# Begin Source File

SOURCE=.\src\StdD3D.cpp
# End Source File
# Begin Source File

SOURCE=.\src\StdDDraw2.cpp
# End Source File
# Begin Source File

SOURCE=.\src\StdFile.cpp
# End Source File
# Begin Source File

SOURCE=.\src\StdFont.cpp

!IF  "$(CFG)" == "Standard - Win32 Release Editor"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Standard - Win32 Debug Editor"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Standard - Win32 Debug"

!ELSEIF  "$(CFG)" == "Standard - Win32 Release"

!ELSEIF  "$(CFG)" == "Standard - Win32 Extreme Debug Editor"

!ELSEIF  "$(CFG)" == "Standard - Win32 Debug Console"

!ELSEIF  "$(CFG)" == "Standard - Win32 Release Console"

!ELSEIF  "$(CFG)" == "Standard - Win32 NoNetwork Release"

!ELSEIF  "$(CFG)" == "Standard - Win32 NoNetwork Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\StdGL.cpp
# End Source File
# Begin Source File

SOURCE=.\src\StdGLCtx.cpp
# End Source File
# Begin Source File

SOURCE=.\src\StdJoystick.cpp
# End Source File
# Begin Source File

SOURCE=.\src\StdMarkup.cpp
# End Source File
# Begin Source File

SOURCE=.\src\StdNoGfx.cpp
# End Source File
# Begin Source File

SOURCE=.\src\StdPNG.cpp
# End Source File
# Begin Source File

SOURCE=.\src\StdRegistry.cpp
# End Source File
# Begin Source File

SOURCE=.\src\StdResStr2.cpp
# End Source File
# Begin Source File

SOURCE=.\src\StdScheduler.cpp
# End Source File
# Begin Source File

SOURCE=.\src\StdSurface2.cpp
# End Source File
# Begin Source File

SOURCE=.\src\StdSurface8.cpp
# End Source File
# Begin Source File

SOURCE=.\src\StdVideo.cpp
# End Source File
# Begin Source File

SOURCE=.\src\StdWindow.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\inc\Bitmap256.h
# End Source File
# Begin Source File

SOURCE=.\inc\CStdFile.h
# End Source File
# Begin Source File

SOURCE=.\inc\d3dutil.h
# End Source File
# Begin Source File

SOURCE=.\inc\DInputX.h
# End Source File
# Begin Source File

SOURCE=.\inc\DSoundX.h
# End Source File
# Begin Source File

SOURCE=.\inc\dxutil.h
# End Source File
# Begin Source File

SOURCE=.\inc\Fixed.h
# End Source File
# Begin Source File

SOURCE=.\inc\Midi.h
# End Source File
# Begin Source File

SOURCE=.\inc\PathFinder.h
# End Source File
# Begin Source File

SOURCE=.\lpng121\png.h
# End Source File
# Begin Source File

SOURCE=.\lpng121\pngasmrd.h
# End Source File
# Begin Source File

SOURCE=.\lpng121\pngconf.h
# End Source File
# Begin Source File

SOURCE=.\inc\Standard.h
# End Source File
# Begin Source File

SOURCE=.\inc\StdAdaptors.h
# End Source File
# Begin Source File

SOURCE=.\inc\StdBase64.h
# End Source File
# Begin Source File

SOURCE=.\inc\StdBitmap.h
# End Source File
# Begin Source File

SOURCE=.\inc\StdBuf.h
# End Source File
# Begin Source File

SOURCE=.\inc\StdColors.h
# End Source File
# Begin Source File

SOURCE=.\inc\StdCompiler.h
# End Source File
# Begin Source File

SOURCE=.\inc\StdConfig.h
# End Source File
# Begin Source File

SOURCE=.\inc\StdD3D.h
# End Source File
# Begin Source File

SOURCE=.\inc\StdDDraw2.h

!IF  "$(CFG)" == "Standard - Win32 Release Editor"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Standard - Win32 Debug Editor"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Standard - Win32 Debug"

!ELSEIF  "$(CFG)" == "Standard - Win32 Release"

!ELSEIF  "$(CFG)" == "Standard - Win32 Extreme Debug Editor"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Standard - Win32 Debug Console"

!ELSEIF  "$(CFG)" == "Standard - Win32 Release Console"

!ELSEIF  "$(CFG)" == "Standard - Win32 NoNetwork Release"

!ELSEIF  "$(CFG)" == "Standard - Win32 NoNetwork Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\inc\StdFacet.h
# End Source File
# Begin Source File

SOURCE=.\inc\StdFile.h
# End Source File
# Begin Source File

SOURCE=.\inc\StdFont.h
# End Source File
# Begin Source File

SOURCE=.\inc\StdGL.h
# End Source File
# Begin Source File

SOURCE=.\inc\StdJoystick.h
# End Source File
# Begin Source File

SOURCE=.\inc\StdMarkup.h
# End Source File
# Begin Source File

SOURCE=.\inc\StdNoGfx.h
# End Source File
# Begin Source File

SOURCE=.\inc\StdPNG.h
# End Source File
# Begin Source File

SOURCE=.\inc\StdRandom.h
# End Source File
# Begin Source File

SOURCE=.\inc\StdRegistry.h
# End Source File
# Begin Source File

SOURCE=.\inc\StdResStr.h
# End Source File
# Begin Source File

SOURCE=.\inc\StdResStr2.h
# End Source File
# Begin Source File

SOURCE=.\inc\StdScheduler.h
# End Source File
# Begin Source File

SOURCE=.\inc\StdSurface2.h

!IF  "$(CFG)" == "Standard - Win32 Release Editor"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Standard - Win32 Debug Editor"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Standard - Win32 Debug"

!ELSEIF  "$(CFG)" == "Standard - Win32 Release"

!ELSEIF  "$(CFG)" == "Standard - Win32 Extreme Debug Editor"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Standard - Win32 Debug Console"

!ELSEIF  "$(CFG)" == "Standard - Win32 Release Console"

!ELSEIF  "$(CFG)" == "Standard - Win32 NoNetwork Release"

!ELSEIF  "$(CFG)" == "Standard - Win32 NoNetwork Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\inc\StdSurface8.h
# End Source File
# Begin Source File

SOURCE=.\inc\StdSync.h
# End Source File
# Begin Source File

SOURCE=.\inc\StdVideo.h
# End Source File
# Begin Source File

SOURCE=.\inc\StdWindow.h
# End Source File
# End Group
# End Target
# End Project
