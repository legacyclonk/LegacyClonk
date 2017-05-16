# Microsoft Developer Studio Project File - Name="editor" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=editor - Win32 Debug Editor
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "editor.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "editor.mak" CFG="editor - Win32 Debug Editor"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "editor - Win32 Release Editor" (based on "Win32 (x86) Application")
!MESSAGE "editor - Win32 Debug Editor" (based on "Win32 (x86) Application")
!MESSAGE "editor - Win32 Extreme Debug Editor" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "editor - Win32 Release Editor"

# PROP BASE Use_MFC 5
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "frontend___Win32_Release_Frontend"
# PROP BASE Intermediate_Dir "frontend___Win32_Release_Frontend"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 5
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "..\intermediate\vc6\Editor\Release"
# PROP Intermediate_Dir "..\intermediate\vc6\Editor\Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /I ".\sec" /I ".\inc" /I "..\engine\inc" /I "..\standard\inc" /I ".\inc\xercesc" /I "..\standard\lpng121" /I "..\standard\zlib" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /Yu"C4Explorer.h" /FD /c
# ADD CPP /nologo /MT /W3 /GX /O2 /I ".\inc" /I "..\engine\inc" /I "..\standard\inc" /I ".\inc\xercesc" /I "..\standard\lpng121" /I "..\standard\zlib" /I "..\standard\xerces230\include" /I "..\standard" /I "..\engine\sec" /I "..\engine\fmod" /D "_WINDOWS" /D "NDEBUG" /D "C4FRONTEND" /D "WIN32" /D "_MBCS" /D "C4MULTIMONITOR" /D "GLEW_STATIC" /FR /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x407 /d "NDEBUG"
# ADD RSC /l 0x407 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 wsock32.lib dxguid.lib ddraw.lib /nologo /subsystem:windows /pdb:"..\intermediate\vc6\FrontendRelease\Planet.pdb" /machine:I386 /out:"..\..\planet\Planet.exe"
# SUBTRACT BASE LINK32 /pdb:none /incremental:yes
# ADD LINK32 /nologo /subsystem:windows /incremental:yes /machine:I386 /out:"..\..\planet\Editor.exe"
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "editor - Win32 Debug Editor"

# PROP BASE Use_MFC 5
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "frontend___Win32_Debug_Frontend"
# PROP BASE Intermediate_Dir "frontend___Win32_Debug_Frontend"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 5
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "..\intermediate\vc6\Editor\Debug"
# PROP Intermediate_Dir "..\intermediate\vc6\Editor\Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /I ".\sec" /I "..\standard\zlib" /I ".\inc" /I "..\engine\inc" /I "..\standard\inc" /I ".\inc\xercesc" /I "..\standard\lpng121" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /Yu"C4Explorer.h" /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /I "..\standard\zlib" /I ".\inc" /I "..\engine\inc" /I "..\standard\inc" /I "..\standard\lpng121" /I "..\standard\xerces230\include" /I "..\standard" /I "..\engine\sec" /I "..\engine\fmod" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "C4FRONTEND" /D "C4MULTIMONITOR" /D "GLEW_STATIC" /FR /YX /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x407 /d "_DEBUG"
# ADD RSC /l 0x407 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 wsock32.lib dxguid.lib ddraw.lib /nologo /subsystem:windows /pdb:"..\intermediate\vc6\FrontendDebug\Planet.pdb" /debug /machine:I386 /out:"..\..\planet\Planet.exe" /pdbtype:sept
# SUBTRACT BASE LINK32 /pdb:none
# ADD LINK32 /nologo /subsystem:windows /pdb:"..\intermediate\vc6\Editor\Debug\Clonk.pdb" /debug /machine:I386 /out:"..\..\planet\EditorD.exe" /pdbtype:sept
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "editor - Win32 Extreme Debug Editor"

# PROP BASE Use_MFC 5
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "frontend___Win32_Extreme_Debug_Frontend"
# PROP BASE Intermediate_Dir "frontend___Win32_Extreme_Debug_Frontend"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 5
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "..\intermediate\vc6\Editor\ExtremeDebug"
# PROP Intermediate_Dir "..\intermediate\vc6\Editor\ExtremeDebug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /I ".\inc" /I "..\engine\inc" /I "..\standard\inc" /I ".\inc\xercesc" /I "..\standard\lpng121" /I "..\standard\zlib" /I "..\standard\xerces230\include" /I "..\standard" /I "..\engine\sec" /I "..\engine\fmod" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "NEW_GFX" /FR /YX /FD /c
# ADD CPP /nologo /MT /W3 /GX /Zi /O2 /I ".\inc" /I "..\engine\inc" /I "..\standard\inc" /I ".\inc\xercesc" /I "..\standard\lpng121" /I "..\standard\zlib" /I "..\standard\xerces230\include" /I "..\standard" /I "..\engine\sec" /I "..\engine\fmod" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "C4FRONTEND" /D "GLEW_STATIC" /FR /Yu"C4Explorer.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x407 /d "NDEBUG"
# ADD RSC /l 0x407 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 wsock32.lib dxguid.lib ddraw.lib /nologo /subsystem:windows /incremental:yes /pdb:"..\intermediate\vc6\FrontendRelease\Planet.pdb" /machine:I386 /out:"..\..\planet\Clonk.exe"
# SUBTRACT BASE LINK32 /pdb:none
# ADD LINK32 /nologo /subsystem:windows /incremental:yes /pdb:"..\intermediate\vc6\Editor\ExtremeDebug\Clonk.pdb" /debug /machine:I386 /out:"..\..\planet\ClonkXD.exe" /pdbtype:sept
# SUBTRACT LINK32 /pdb:none

!ENDIF 

# Begin Target

# Name "editor - Win32 Release Editor"
# Name "editor - Win32 Debug Editor"
# Name "editor - Win32 Extreme Debug Editor"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Group "Controls"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\ButtonEx.cpp
# End Source File
# Begin Source File

SOURCE=.\src\DIBitmap.cpp
# End Source File
# Begin Source File

SOURCE=.\src\FileVersion.cpp
# End Source File
# Begin Source File

SOURCE=.\src\IconCombo.cpp
# End Source File
# Begin Source File

SOURCE=.\src\InputDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\src\ListBoxEx.cpp
# End Source File
# Begin Source File

SOURCE=.\src\RadioEx.cpp
# End Source File
# Begin Source File

SOURCE=.\src\RichEditEx.cpp
# End Source File
# Begin Source File

SOURCE=.\src\Slider.cpp
# End Source File
# Begin Source File

SOURCE=.\src\StaticEx.cpp
# End Source File
# Begin Source File

SOURCE=.\src\StdAfx.cpp
# End Source File
# Begin Source File

SOURCE=.\src\StringEx.cpp
# End Source File
# Begin Source File

SOURCE=.\src\TabCtrlEx.cpp
# End Source File
# End Group
# Begin Group "Engine"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\engine\src\C4ComponentHost.cpp
# SUBTRACT BASE CPP /YX /Yc /Yu
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\engine\src\C4Config.cpp
# SUBTRACT BASE CPP /YX /Yc /Yu
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\Engine\sec\C4ConfigShareware.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\engine\src\C4Def.cpp
# SUBTRACT BASE CPP /YX /Yc /Yu
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\engine\src\C4Facet.cpp
# SUBTRACT BASE CPP /YX /Yc /Yu
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\engine\src\C4FacetEx.cpp
# SUBTRACT BASE CPP /YX /Yc /Yu
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\engine\src\C4FileClasses.cpp
# SUBTRACT BASE CPP /YX /Yc /Yu
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\engine\src\C4Folder.cpp
# End Source File
# Begin Source File

SOURCE=..\engine\src\C4Group.cpp
# SUBTRACT BASE CPP /YX /Yc /Yu
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\engine\src\C4GroupSet.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\engine\src\C4Id.cpp
# SUBTRACT BASE CPP /YX /Yc /Yu
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\engine\src\C4IDList.cpp
# SUBTRACT BASE CPP /YX /Yc /Yu
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\engine\src\C4InfoCore.cpp
# SUBTRACT BASE CPP /YX /Yc /Yu
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\engine\src\C4InputValidation.cpp
# End Source File
# Begin Source File

SOURCE=..\engine\src\C4Language.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\engine\src\C4Map.cpp
# SUBTRACT BASE CPP /YX /Yc /Yu
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\engine\src\C4MapCreatorS2.cpp
# SUBTRACT BASE CPP /YX /Yc /Yu
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\engine\src\C4Material.cpp
# SUBTRACT BASE CPP /YX /Yc /Yu
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\engine\src\C4NameList.cpp
# SUBTRACT BASE CPP /YX /Yc /Yu
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\engine\src\C4ObjectInfo.cpp
# SUBTRACT BASE CPP /YX /Yc /Yu
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\engine\src\C4Random.cpp
# SUBTRACT BASE CPP /YX /Yc /Yu
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\engine\src\C4RankSystem.cpp
# SUBTRACT BASE CPP /YX /Yc /Yu
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\engine\src\C4Scenario.cpp
# SUBTRACT BASE CPP /YX /Yc /Yu
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\Engine\sec\C4SecurityCertificates.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\engine\src\C4Shape.cpp
# SUBTRACT BASE CPP /YX /Yc /Yu
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\engine\src\C4StringTable.cpp
# SUBTRACT BASE CPP /YX /Yc /Yu
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\engine\src\C4Texture.cpp
# SUBTRACT BASE CPP /YX /Yc /Yu
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\engine\src\C4Value.cpp
# SUBTRACT BASE CPP /YX /Yc /Yu
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\engine\src\C4ValueList.cpp
# SUBTRACT BASE CPP /YX /Yc /Yu
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\engine\src\C4ValueMap.cpp
# SUBTRACT BASE CPP /YX /Yc /Yu
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# End Group
# Begin Source File

SOURCE=.\src\C4AboutDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\src\C4DefListEx.cpp
# End Source File
# Begin Source File

SOURCE=.\src\C4EditorPage.cpp
# End Source File
# Begin Source File

SOURCE=.\src\C4EditSlot.cpp
# End Source File
# Begin Source File

SOURCE=.\src\C4Explorer.cpp
# ADD BASE CPP /Yc"C4Explorer.h"
# ADD CPP /Yc"C4Explorer.h"
# End Source File
# Begin Source File

SOURCE=.\src\C4ExplorerCfg.cpp
# End Source File
# Begin Source File

SOURCE=.\src\C4ExplorerDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\src\C4GamePage.cpp
# End Source File
# Begin Source File

SOURCE=.\src\C4IdCtrlButtons.cpp
# End Source File
# Begin Source File

SOURCE=.\src\C4IdListCtrl.cpp
# End Source File
# Begin Source File

SOURCE=.\src\C4IdSelectDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\src\C4ItemTypes.cpp
# End Source File
# Begin Source File

SOURCE=.\src\C4LandscapeCtrl.cpp
# End Source File
# Begin Source File

SOURCE=.\src\C4LicenseDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\src\C4MessageBox.cpp
# End Source File
# Begin Source File

SOURCE=.\src\C4OptionsSheet.cpp
# End Source File
# Begin Source File

SOURCE=.\src\C4PageDeveloper.cpp
# End Source File
# Begin Source File

SOURCE=.\src\C4PageEnvironment.cpp
# End Source File
# Begin Source File

SOURCE=.\src\C4PageLandscape.cpp
# End Source File
# Begin Source File

SOURCE=.\src\C4PagePlayerStart.cpp
# End Source File
# Begin Source File

SOURCE=.\src\C4PNG.cpp
# End Source File
# Begin Source File

SOURCE=.\src\C4ProgramPage.cpp
# End Source File
# Begin Source File

SOURCE=.\src\C4RegistrationDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\src\C4ScenarioDefinitionsDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\src\C4ScenarioDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\src\C4SplashDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\src\C4TypeSelectDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\src\C4Utilities.cpp
# End Source File
# Begin Source File

SOURCE=.\src\C4ViewCtrl.cpp
# End Source File
# Begin Source File

SOURCE=.\src\C4ViewItem.cpp
# End Source File
# Begin Source File

SOURCE=.\src\C4WeatherPage.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Group "Security Nr. 1"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\engine\sec\C4ConfigShareware.h
# End Source File
# End Group
# Begin Group "Engine Nr. 1"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\engine\inc\C4ComponentHost.h
# End Source File
# Begin Source File

SOURCE=..\engine\inc\C4Components.h
# End Source File
# Begin Source File

SOURCE=..\engine\inc\C4Config.h
# End Source File
# Begin Source File

SOURCE=..\engine\inc\C4Def.h
# End Source File
# Begin Source File

SOURCE=..\engine\inc\C4Fonts.h
# End Source File
# Begin Source File

SOURCE=..\engine\inc\C4Group.h
# End Source File
# Begin Source File

SOURCE=..\Engine\inc\C4GroupSet.h
# End Source File
# End Group
# Begin Source File

SOURCE=.\inc\ButtonEx.h
# End Source File
# Begin Source File

SOURCE=.\inc\C4AboutDlg.h
# End Source File
# Begin Source File

SOURCE=.\inc\C4CompilerWrapper.h
# End Source File
# Begin Source File

SOURCE=.\inc\C4DefListEx.h
# End Source File
# Begin Source File

SOURCE=.\inc\C4DownloadDlg.h
# End Source File
# Begin Source File

SOURCE=.\inc\C4EditSlot.h
# End Source File
# Begin Source File

SOURCE=.\inc\C4Explorer.h
# End Source File
# Begin Source File

SOURCE=.\inc\C4ExplorerCfg.h
# End Source File
# Begin Source File

SOURCE=.\inc\C4ExplorerDlg.h
# End Source File
# Begin Source File

SOURCE=..\engine\inc\C4Folder.h
# End Source File
# Begin Source File

SOURCE=.\inc\C4Frontend.h
# End Source File
# Begin Source File

SOURCE=.\inc\C4IdCtrlButtons.h
# End Source File
# Begin Source File

SOURCE=.\inc\C4IdListCtrl.h
# End Source File
# Begin Source File

SOURCE=.\inc\C4IdSelectDlg.h
# End Source File
# Begin Source File

SOURCE=.\inc\C4ItemTypes.h
# End Source File
# Begin Source File

SOURCE=.\inc\C4LandscapeCtrl.h
# End Source File
# Begin Source File

SOURCE=..\engine\inc\C4Language.h
# End Source File
# Begin Source File

SOURCE=.\inc\C4LeagueClient.h
# End Source File
# Begin Source File

SOURCE=.\inc\C4LicenseDlg.h
# End Source File
# Begin Source File

SOURCE=.\inc\C4MessageBox.h
# End Source File
# Begin Source File

SOURCE=.\inc\C4OptionsSheet.h
# End Source File
# Begin Source File

SOURCE=.\inc\C4PageDeveloper.h
# End Source File
# Begin Source File

SOURCE=.\inc\C4PageEditor.h
# End Source File
# Begin Source File

SOURCE=.\inc\C4PageEnvironment.h
# End Source File
# Begin Source File

SOURCE=.\inc\C4PageGame.h
# End Source File
# Begin Source File

SOURCE=.\inc\C4PageLandscape.h
# End Source File
# Begin Source File

SOURCE=.\inc\C4PagePlayerStart.h
# End Source File
# Begin Source File

SOURCE=.\inc\C4PageProgram.h
# End Source File
# Begin Source File

SOURCE=.\inc\C4PageWeather.h
# End Source File
# Begin Source File

SOURCE=.\inc\C4PNG.h
# End Source File
# Begin Source File

SOURCE=.\inc\C4RegistrationDlg.h
# End Source File
# Begin Source File

SOURCE=..\engine\inc\C4Scenario.h
# End Source File
# Begin Source File

SOURCE=.\inc\C4ScenarioDefinitionsDlg.h
# End Source File
# Begin Source File

SOURCE=.\inc\C4ScenarioDlg.h
# End Source File
# Begin Source File

SOURCE=.\inc\C4SplashDlg.h
# End Source File
# Begin Source File

SOURCE=.\inc\C4TypeSelectDlg.h
# End Source File
# Begin Source File

SOURCE=.\inc\C4Utilities.h
# End Source File
# Begin Source File

SOURCE=.\inc\C4ViewCtrl.h
# End Source File
# Begin Source File

SOURCE=.\inc\C4ViewItem.h
# End Source File
# Begin Source File

SOURCE=.\inc\CreditsThread.h
# End Source File
# Begin Source File

SOURCE=.\inc\DIBitmap.h
# End Source File
# Begin Source File

SOURCE=.\inc\DllGetVersion.h
# End Source File
# Begin Source File

SOURCE=.\inc\FileVersion.h
# End Source File
# Begin Source File

SOURCE=.\inc\IconCombo.h
# End Source File
# Begin Source File

SOURCE=.\inc\InputDlg.h
# End Source File
# Begin Source File

SOURCE=.\inc\ListBoxEx.h
# End Source File
# Begin Source File

SOURCE=.\inc\RadioEx.h
# End Source File
# Begin Source File

SOURCE=.\inc\RichEditEx.h
# End Source File
# Begin Source File

SOURCE=.\inc\Slider.h
# End Source File
# Begin Source File

SOURCE=.\inc\StaticEx.h
# End Source File
# Begin Source File

SOURCE=.\inc\StdAfx.h
# End Source File
# Begin Source File

SOURCE=.\inc\StringEx.h
# End Source File
# Begin Source File

SOURCE=.\inc\TabCtrlEx.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\res\avi1.bin
# End Source File
# Begin Source File

SOURCE=.\res\Button.bmp
# End Source File
# Begin Source File

SOURCE=.\res\CursorCopy.cur
# End Source File
# Begin Source File

SOURCE=.\res\DecoWeather.bmp
# End Source File
# Begin Source File

SOURCE=.\res\editor.ico
# End Source File
# Begin Source File

SOURCE=.\res\editor.ico
# End Source File
# Begin Source File

SOURCE=.\res\editor.rc
# End Source File
# Begin Source File

SOURCE=.\res\IconItem.bmp
# End Source File
# Begin Source File

SOURCE=.\res\IconScenario.bmp
# End Source File
# Begin Source File

SOURCE=.\res\IdCtrlButtons.bmp
# End Source File
# Begin Source File

SOURCE=.\res\ItemState.bmp
# End Source File
# Begin Source File

SOURCE=.\res\ListDecoGame.bmp
# End Source File
# Begin Source File

SOURCE=.\res\New.bmp
# End Source File
# Begin Source File

SOURCE=.\res\New.c.bin
# End Source File
# Begin Source File

SOURCE=.\res\New.c4d
# End Source File
# Begin Source File

SOURCE=.\res\New.c4f
# End Source File
# Begin Source File

SOURCE=.\res\New.c4p
# End Source File
# Begin Source File

SOURCE=.\res\New.c4s
# End Source File
# Begin Source File

SOURCE=.\res\New.png
# End Source File
# Begin Source File

SOURCE=.\res\New.rtf
# End Source File
# Begin Source File

SOURCE=.\res\New.txt
# End Source File
# Begin Source File

SOURCE=.\res\New.wav
# End Source File
# Begin Source File

SOURCE=.\res\NewFolder.c4d
# End Source File
# Begin Source File

SOURCE=.\res\Overlay.bmp
# End Source File
# Begin Source File

SOURCE=.\res\Palette.bmp
# End Source File
# Begin Source File

SOURCE=.\res\Paper.bmp
# End Source File
# Begin Source File

SOURCE=.\res\Paper2.bmp
# End Source File
# Begin Source File

SOURCE=.\res\resource.h
# End Source File
# Begin Source File

SOURCE=.\res\Splash.bmp
# End Source File
# Begin Source File

SOURCE=.\res\TabIcon.bmp
# End Source File
# End Group
# Begin Group "Library Files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\standard\lib\StandardFE.lib

!IF  "$(CFG)" == "editor - Win32 Release Editor"

!ELSEIF  "$(CFG)" == "editor - Win32 Debug Editor"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "editor - Win32 Extreme Debug Editor"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\standard\lib\StandardDFE.lib

!IF  "$(CFG)" == "editor - Win32 Release Editor"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "editor - Win32 Debug Editor"

!ELSEIF  "$(CFG)" == "editor - Win32 Extreme Debug Editor"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\Standard\openssl\libeay32.lib

!IF  "$(CFG)" == "editor - Win32 Release Editor"

!ELSEIF  "$(CFG)" == "editor - Win32 Debug Editor"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "editor - Win32 Extreme Debug Editor"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\Standard\openssl\libeay32_d.lib

!IF  "$(CFG)" == "editor - Win32 Release Editor"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "editor - Win32 Debug Editor"

!ELSEIF  "$(CFG)" == "editor - Win32 Extreme Debug Editor"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\Engine\fmod\fmodvc.lib
# End Source File
# Begin Source File

SOURCE=..\standard\lib\StandardXDFE.lib

!IF  "$(CFG)" == "editor - Win32 Release Editor"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "editor - Win32 Debug Editor"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "editor - Win32 Extreme Debug Editor"

!ENDIF 

# End Source File
# End Group
# Begin Group "Language Files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\planet\System.c4g\LanguageDE.txt
# End Source File
# Begin Source File

SOURCE=..\..\planet\System.c4g\LanguageUS.txt
# End Source File
# End Group
# End Target
# End Project
