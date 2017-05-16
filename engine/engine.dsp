# Microsoft Developer Studio Project File - Name="engine" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=engine - Win32 NoNetwork Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "engine.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "engine.mak" CFG="engine - Win32 NoNetwork Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "engine - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "engine - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE "engine - Win32 NoNetwork Release" (based on "Win32 (x86) Application")
!MESSAGE "engine - Win32 NoNetwork Debug" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "engine - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "engine___Win32_Release"
# PROP BASE Intermediate_Dir "engine___Win32_Release"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "..\intermediate\vc6\Engine\Release"
# PROP Intermediate_Dir "..\intermediate\vc6\Engine\Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /I "..\standard\zlib" /I ".\inc" /I "..\standard\inc" /I ".\fmod" /I ".\sec" /I "..\standard\lpng121" /I "..\standard" /D "NDEBUG" /D "C4ENGINE" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "NEW_GFX" /FR /Yu"C4Include.h" /FD /c
# ADD CPP /nologo /MT /W3 /GX /Ox /Ot /Oa /Ow /Og /Oi /I "..\standard\zlib" /I ".\inc" /I "..\standard\inc" /I ".\fmod" /I ".\sec" /I "..\standard\lpng121" /I "..\standard" /I "..\standard\jpeglib" /D "_WINDOWS" /D "USE_FMOD" /D "BIG_C4INCLUDE" /D "NDEBUG" /D "C4ENGINE" /D "USE_DIRECTX" /D "USE_GL" /D "WIN32" /D "_MBCS" /D "C4MULTIMONITOR" /D "GLEW_STATIC" /FR /Yu"C4Include.h" /FD /GF /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x407 /d "NDEBUG" /d "NEW_GFX"
# ADD RSC /l 0x407 /d "NDEBUG" /d "NEW_GFX"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 Standard2.lib d3d8.lib opengl32.lib glu32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib winmm.lib vfw32.lib wsock32.lib ddraw.lib dsound.lib dxguid.lib ws2_32.lib /nologo /subsystem:windows /pdb:"..\intermediate\vc6\clonk.pdb" /machine:I386 /out:"..\..\planet\clonk.c4x" /libpath:"../standard/lib"
# SUBTRACT BASE LINK32 /pdb:none
# ADD LINK32 ddraw.lib dxguid.lib opengl32.lib glu32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib winmm.lib vfw32.lib wsock32.lib dsound.lib ws2_32.lib d3d8.lib /nologo /subsystem:windows /incremental:yes /pdb:"..\intermediate\vc6\clonk.pdb" /machine:I386 /out:"..\..\planet\Clonk.exe" /libpath:"../standard/jpeglib"
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "engine - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "engine___Win32_Debug"
# PROP BASE Intermediate_Dir "engine___Win32_Debug"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "..\intermediate\vc6\Engine\Debug"
# PROP Intermediate_Dir "..\intermediate\vc6\Engine\Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /I ".\inc" /I "..\standard\inc" /I ".\fmod" /I ".\sec" /I "..\standard\lpng121" /I "..\standard\zlib" /I "..\standard" /D "_DEBUG" /D "C4ENGINE" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "NEW_GFX" /FR /Yu"C4Include.h" /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /WX /GX /ZI /Od /I "..\standard\zlib" /I ".\inc" /I "..\standard\inc" /I ".\fmod" /I ".\sec" /I "..\standard\lpng121" /I "..\standard" /I "..\standard\jpeglib" /D "BIG_C4INCLUDE" /D "USE_FMOD" /D "_DEBUG" /D "C4ENGINE" /D "_WINDOWS" /D "USE_GL" /D "USE_DIRECTX" /D "WIN32" /D "_MBCS" /D "C4MULTIMONITOR" /D "GLEW_STATIC" /FR /Yu"C4Include.h" /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x407 /d "_DEBUG" /d "NEW_GFX"
# ADD RSC /l 0x407 /d "_DEBUG" /d "NEW_GFX"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 StandardD2.lib d3dx8.lib d3d8.lib opengl32.lib glu32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib winmm.lib vfw32.lib wsock32.lib ddraw.lib dsound.lib dxguid.lib ws2_32.lib /nologo /subsystem:windows /pdb:"..\intermediate\vc6\EngineDebug/clonk.pdb" /debug /machine:I386 /nodefaultlib:"StandardD.lib" /out:"..\..\planet\clonkd.c4x" /pdbtype:sept /libpath:"../standard/lib"
# SUBTRACT BASE LINK32 /profile /pdb:none /incremental:no /nodefaultlib
# ADD LINK32 opengl32.lib glu32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib winmm.lib vfw32.lib wsock32.lib dsound.lib ws2_32.lib d3d8.lib /nologo /subsystem:windows /pdb:"..\intermediate\vc6\Engine\Debug\clonk.pdb" /debug /machine:I386 /out:"..\..\planet\ClonkD.exe" /pdbtype:sept /libpath:"../standard/jpeglib"
# SUBTRACT LINK32 /profile /pdb:none /incremental:no /nodefaultlib

!ELSEIF  "$(CFG)" == "engine - Win32 NoNetwork Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "engine___Win32_NoNetwork_Release"
# PROP BASE Intermediate_Dir "engine___Win32_NoNetwork_Release"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "..\intermediate\vc6\Engine\NoNetwork\Release"
# PROP Intermediate_Dir "..\intermediate\vc6\Engine\NoNetwork\Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /Ox /Ot /Oa /Ow /Og /Oi /I "..\standard\zlib" /I ".\inc" /I "..\standard\inc" /I ".\fmod" /I ".\sec" /I "..\standard\lpng121" /I "..\standard" /I "..\standard\jpeglib" /D "_WINDOWS" /D "USE_FMOD" /D "BIG_C4INCLUDE" /D "NDEBUG" /D "C4ENGINE" /D "USE_DIRECTX" /D "USE_GL" /D "WIN32" /D "_MBCS" /D "C4MULTIMONITOR" /D "GLEW_STATIC" /FR /Yu"C4Include.h" /FD /GF /c
# ADD CPP /nologo /MT /W3 /GX /Ox /Ot /Oa /Ow /Og /Oi /I "..\standard\zlib" /I ".\inc" /I "..\standard\inc" /I ".\fmod" /I ".\sec" /I "..\standard\lpng121" /I "..\standard" /I "..\standard\jpeglib" /D "_WINDOWS" /D "USE_FMOD" /D "BIG_C4INCLUDE" /D "NDEBUG" /D "C4ENGINE" /D "USE_DIRECTX" /D "USE_GL" /D "WIN32" /D "_MBCS" /D "C4MULTIMONITOR" /D "GLEW_STATIC" /FR /Yu"C4Include.h" /FD /GF /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x407 /d "NDEBUG" /d "NEW_GFX"
# ADD RSC /l 0x407 /d "NDEBUG" /d "NEW_GFX"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 ddraw.lib dxguid.lib opengl32.lib glu32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib winmm.lib vfw32.lib wsock32.lib dsound.lib ws2_32.lib d3d8.lib /nologo /subsystem:windows /incremental:yes /pdb:"..\intermediate\vc6\clonk.pdb" /machine:I386 /out:"..\..\planet\Clonk.exe" /libpath:"../standard/jpeglib"
# SUBTRACT BASE LINK32 /pdb:none
# ADD LINK32 ddraw.lib dxguid.lib opengl32.lib glu32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib winmm.lib vfw32.lib wsock32.lib dsound.lib ws2_32.lib d3d8.lib /nologo /subsystem:windows /incremental:yes /pdb:"..\intermediate\vc6\clonk.pdb" /machine:I386 /out:"..\..\planet\Clonk.exe" /libpath:"../standard/jpeglib"
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "engine - Win32 NoNetwork Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "engine___Win32_NoNetwork_Debug"
# PROP BASE Intermediate_Dir "engine___Win32_NoNetwork_Debug"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "..\intermediate\vc6\Engine\NoNetwork\Debug"
# PROP Intermediate_Dir "..\intermediate\vc6\Engine\NoNetwork\Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /WX /GX /ZI /Od /I "..\standard\zlib" /I ".\inc" /I "..\standard\inc" /I ".\fmod" /I ".\sec" /I "..\standard\lpng121" /I "..\standard" /I "..\standard\jpeglib" /D "BIG_C4INCLUDE" /D "USE_FMOD" /D "_DEBUG" /D "C4ENGINE" /D "_WINDOWS" /D "USE_GL" /D "USE_DIRECTX" /D "WIN32" /D "_MBCS" /D "C4MULTIMONITOR" /D "GLEW_STATIC" /FR /Yu"C4Include.h" /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /WX /GX /ZI /Od /I "..\standard\zlib" /I ".\inc" /I "..\standard\inc" /I ".\fmod" /I ".\sec" /I "..\standard\lpng121" /I "..\standard" /I "..\standard\jpeglib" /D "BIG_C4INCLUDE" /D "USE_FMOD" /D "_DEBUG" /D "C4ENGINE" /D "_WINDOWS" /D "USE_GL" /D "USE_DIRECTX" /D "WIN32" /D "_MBCS" /D "C4MULTIMONITOR" /D "GLEW_STATIC" /FR /Yu"C4Include.h" /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x407 /d "_DEBUG" /d "NEW_GFX"
# ADD RSC /l 0x407 /d "_DEBUG" /d "NEW_GFX"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 opengl32.lib glu32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib winmm.lib vfw32.lib wsock32.lib dsound.lib ws2_32.lib d3d8.lib /nologo /subsystem:windows /pdb:"..\intermediate\vc6\Engine\Debug\clonk.pdb" /debug /machine:I386 /out:"..\..\planet\ClonkD.exe" /pdbtype:sept /libpath:"../standard/jpeglib"
# SUBTRACT BASE LINK32 /profile /pdb:none /incremental:no /nodefaultlib
# ADD LINK32 opengl32.lib glu32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib winmm.lib vfw32.lib wsock32.lib dsound.lib ws2_32.lib d3d8.lib /nologo /subsystem:windows /pdb:"..\intermediate\vc6\Engine\Debug\clonk.pdb" /debug /machine:I386 /out:"..\..\planet\ClonkD.exe" /pdbtype:sept /libpath:"../standard/jpeglib"
# SUBTRACT LINK32 /profile /pdb:none /incremental:no /nodefaultlib

!ENDIF 

# Begin Target

# Name "engine - Win32 Release"
# Name "engine - Win32 Debug"
# Name "engine - Win32 NoNetwork Release"
# Name "engine - Win32 NoNetwork Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\src\C4Action.cpp
# End Source File
# Begin Source File

SOURCE=.\src\C4AList.cpp
# End Source File
# Begin Source File

SOURCE=.\src\C4Application.cpp
# End Source File
# Begin Source File

SOURCE=.\src\C4Aul.cpp
# End Source File
# Begin Source File

SOURCE=.\src\C4AulExec.cpp
# End Source File
# Begin Source File

SOURCE=.\src\C4AulLink.cpp
# End Source File
# Begin Source File

SOURCE=.\src\C4AulParse.cpp
# End Source File
# Begin Source File

SOURCE=.\src\C4ChatDlg.cpp

!IF  "$(CFG)" == "engine - Win32 Release"

!ELSEIF  "$(CFG)" == "engine - Win32 Debug"

!ELSEIF  "$(CFG)" == "engine - Win32 NoNetwork Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "engine - Win32 NoNetwork Debug"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\C4Client.cpp
# End Source File
# Begin Source File

SOURCE=.\src\C4Command.cpp
# End Source File
# Begin Source File

SOURCE=.\src\C4ComponentHost.cpp
# End Source File
# Begin Source File

SOURCE=.\src\C4Config.cpp
# End Source File
# Begin Source File

SOURCE=.\sec\C4ConfigShareware.cpp
# End Source File
# Begin Source File

SOURCE=.\src\C4Console.cpp
# End Source File
# Begin Source File

SOURCE=.\src\C4Control.cpp
# End Source File
# Begin Source File

SOURCE=.\src\C4Def.cpp
# End Source File
# Begin Source File

SOURCE=.\src\C4DefGraphics.cpp
# End Source File
# Begin Source File

SOURCE=.\src\C4DownloadDlg.cpp

!IF  "$(CFG)" == "engine - Win32 Release"

!ELSEIF  "$(CFG)" == "engine - Win32 Debug"

!ELSEIF  "$(CFG)" == "engine - Win32 NoNetwork Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "engine - Win32 NoNetwork Debug"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\C4EditCursor.cpp
# End Source File
# Begin Source File

SOURCE=.\src\C4Effect.cpp
# End Source File
# Begin Source File

SOURCE=.\src\C4Extra.cpp
# End Source File
# Begin Source File

SOURCE=.\src\C4Facet.cpp
# End Source File
# Begin Source File

SOURCE=.\src\C4FacetEx.cpp
# End Source File
# Begin Source File

SOURCE=.\src\C4FileClasses.cpp
# End Source File
# Begin Source File

SOURCE=.\src\C4FileMonitor.cpp
# End Source File
# Begin Source File

SOURCE=.\src\C4FileSelDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\src\C4FindObject.cpp
# End Source File
# Begin Source File

SOURCE=.\src\C4FogOfWar.cpp
# End Source File
# Begin Source File

SOURCE=.\src\C4Folder.cpp
# End Source File
# Begin Source File

SOURCE=.\src\C4Fonts.cpp
# End Source File
# Begin Source File

SOURCE=.\src\C4FullScreen.cpp
# End Source File
# Begin Source File

SOURCE=.\src\C4Game.cpp
# End Source File
# Begin Source File

SOURCE=.\src\C4GameControl.cpp
# End Source File
# Begin Source File

SOURCE=.\src\C4GameControlNetwork.cpp

!IF  "$(CFG)" == "engine - Win32 Release"

!ELSEIF  "$(CFG)" == "engine - Win32 Debug"

!ELSEIF  "$(CFG)" == "engine - Win32 NoNetwork Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "engine - Win32 NoNetwork Debug"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\C4GameDialogs.cpp
# End Source File
# Begin Source File

SOURCE=.\src\C4GameLobby.cpp
# End Source File
# Begin Source File

SOURCE=.\src\C4GameMessage.cpp
# End Source File
# Begin Source File

SOURCE=.\src\C4GameObjects.cpp
# End Source File
# Begin Source File

SOURCE=.\src\C4GameOptions.cpp
# End Source File
# Begin Source File

SOURCE=.\src\C4GameOverDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\src\C4GamePadCon.cpp
# End Source File
# Begin Source File

SOURCE=.\src\C4GameParameters.cpp
# End Source File
# Begin Source File

SOURCE=.\src\C4GameSave.cpp
# End Source File
# Begin Source File

SOURCE=.\src\C4GraphicsResource.cpp
# End Source File
# Begin Source File

SOURCE=.\src\C4GraphicsSystem.cpp
# End Source File
# Begin Source File

SOURCE=.\src\C4Group.cpp
# End Source File
# Begin Source File

SOURCE=.\src\C4GroupSet.cpp
# End Source File
# Begin Source File

SOURCE=.\src\C4Gui.cpp
# End Source File
# Begin Source File

SOURCE=.\src\C4GuiButton.cpp
# End Source File
# Begin Source File

SOURCE=.\src\C4GuiCheckBox.cpp
# End Source File
# Begin Source File

SOURCE=.\src\C4GuiComboBox.cpp
# End Source File
# Begin Source File

SOURCE=.\src\C4GuiContainers.cpp
# End Source File
# Begin Source File

SOURCE=.\src\C4GuiDialogs.cpp
# End Source File
# Begin Source File

SOURCE=.\src\C4GuiEdit.cpp
# End Source File
# Begin Source File

SOURCE=.\src\C4GuiLabels.cpp
# End Source File
# Begin Source File

SOURCE=.\src\C4GuiListBox.cpp
# End Source File
# Begin Source File

SOURCE=.\src\C4GuiMenu.cpp
# End Source File
# Begin Source File

SOURCE=.\src\C4GuiTabular.cpp
# End Source File
# Begin Source File

SOURCE=.\src\C4Id.cpp
# End Source File
# Begin Source File

SOURCE=.\src\C4IDList.cpp
# End Source File
# Begin Source File

SOURCE=.\src\C4Include.cpp
# ADD BASE CPP /Yc"C4Include.h"
# ADD CPP /Yc"C4Include.h"
# End Source File
# Begin Source File

SOURCE=.\src\C4InfoCore.cpp
# End Source File
# Begin Source File

SOURCE=.\src\C4InputValidation.cpp
# End Source File
# Begin Source File

SOURCE=.\src\C4InteractiveThread.cpp
# End Source File
# Begin Source File

SOURCE=.\src\C4KeyboardInput.cpp
# End Source File
# Begin Source File

SOURCE=.\src\C4Landscape.cpp
# End Source File
# Begin Source File

SOURCE=.\src\C4LangStringTable.cpp
# End Source File
# Begin Source File

SOURCE=.\src\C4Language.cpp
# End Source File
# Begin Source File

SOURCE=.\src\C4League.cpp

!IF  "$(CFG)" == "engine - Win32 Release"

!ELSEIF  "$(CFG)" == "engine - Win32 Debug"

!ELSEIF  "$(CFG)" == "engine - Win32 NoNetwork Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "engine - Win32 NoNetwork Debug"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\C4LoaderScreen.cpp
# End Source File
# Begin Source File

SOURCE=.\src\C4Log.cpp
# End Source File
# Begin Source File

SOURCE=.\src\C4LogBuf.cpp
# End Source File
# Begin Source File

SOURCE=.\src\C4MainMenu.cpp
# End Source File
# Begin Source File

SOURCE=.\src\C4Map.cpp
# End Source File
# Begin Source File

SOURCE=.\src\C4MapCreatorS2.cpp
# End Source File
# Begin Source File

SOURCE=.\src\C4MassMover.cpp
# End Source File
# Begin Source File

SOURCE=.\src\C4Material.cpp
# End Source File
# Begin Source File

SOURCE=.\src\C4MaterialList.cpp
# End Source File
# Begin Source File

SOURCE=.\src\C4Menu.cpp
# End Source File
# Begin Source File

SOURCE=.\src\C4MessageBoard.cpp
# End Source File
# Begin Source File

SOURCE=.\src\C4MessageInput.cpp
# End Source File
# Begin Source File

SOURCE=.\src\C4MouseControl.cpp
# End Source File
# Begin Source File

SOURCE=.\src\C4Movement.cpp
# End Source File
# Begin Source File

SOURCE=.\src\C4MusicFile.cpp
# End Source File
# Begin Source File

SOURCE=.\src\C4MusicSystem.cpp
# End Source File
# Begin Source File

SOURCE=.\src\C4NameList.cpp
# End Source File
# Begin Source File

SOURCE=.\src\C4NetIO.cpp

!IF  "$(CFG)" == "engine - Win32 Release"

!ELSEIF  "$(CFG)" == "engine - Win32 Debug"

!ELSEIF  "$(CFG)" == "engine - Win32 NoNetwork Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "engine - Win32 NoNetwork Debug"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\C4Network2.cpp

!IF  "$(CFG)" == "engine - Win32 Release"

!ELSEIF  "$(CFG)" == "engine - Win32 Debug"

!ELSEIF  "$(CFG)" == "engine - Win32 NoNetwork Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "engine - Win32 NoNetwork Debug"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\C4Network2Client.cpp

!IF  "$(CFG)" == "engine - Win32 Release"

!ELSEIF  "$(CFG)" == "engine - Win32 Debug"

!ELSEIF  "$(CFG)" == "engine - Win32 NoNetwork Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "engine - Win32 NoNetwork Debug"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\C4Network2Dialogs.cpp
# End Source File
# Begin Source File

SOURCE=.\src\C4Network2Discover.cpp

!IF  "$(CFG)" == "engine - Win32 Release"

!ELSEIF  "$(CFG)" == "engine - Win32 Debug"

!ELSEIF  "$(CFG)" == "engine - Win32 NoNetwork Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "engine - Win32 NoNetwork Debug"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\C4Network2IO.cpp

!IF  "$(CFG)" == "engine - Win32 Release"

!ELSEIF  "$(CFG)" == "engine - Win32 Debug"

!ELSEIF  "$(CFG)" == "engine - Win32 NoNetwork Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "engine - Win32 NoNetwork Debug"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\C4Network2IRC.cpp

!IF  "$(CFG)" == "engine - Win32 Release"

!ELSEIF  "$(CFG)" == "engine - Win32 Debug"

!ELSEIF  "$(CFG)" == "engine - Win32 NoNetwork Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "engine - Win32 NoNetwork Debug"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\C4Network2Players.cpp

!IF  "$(CFG)" == "engine - Win32 Release"

!ELSEIF  "$(CFG)" == "engine - Win32 Debug"

!ELSEIF  "$(CFG)" == "engine - Win32 NoNetwork Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "engine - Win32 NoNetwork Debug"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\C4Network2Reference.cpp

!IF  "$(CFG)" == "engine - Win32 Release"

!ELSEIF  "$(CFG)" == "engine - Win32 Debug"

!ELSEIF  "$(CFG)" == "engine - Win32 NoNetwork Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "engine - Win32 NoNetwork Debug"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\C4Network2Res.cpp

!IF  "$(CFG)" == "engine - Win32 Release"

!ELSEIF  "$(CFG)" == "engine - Win32 Debug"

!ELSEIF  "$(CFG)" == "engine - Win32 NoNetwork Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "engine - Win32 NoNetwork Debug"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\C4Network2ResDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\src\C4Network2Stats.cpp
# End Source File
# Begin Source File

SOURCE=.\src\C4NoNetwork.cpp

!IF  "$(CFG)" == "engine - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "engine - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "engine - Win32 NoNetwork Release"

!ELSEIF  "$(CFG)" == "engine - Win32 NoNetwork Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\C4Object.cpp
# End Source File
# Begin Source File

SOURCE=.\src\C4ObjectCom.cpp
# End Source File
# Begin Source File

SOURCE=.\src\C4ObjectInfo.cpp
# End Source File
# Begin Source File

SOURCE=.\src\C4ObjectInfoList.cpp
# End Source File
# Begin Source File

SOURCE=.\src\C4ObjectList.cpp
# End Source File
# Begin Source File

SOURCE=.\src\C4ObjectListDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\src\C4ObjectMenu.cpp
# End Source File
# Begin Source File

SOURCE=.\src\C4Packet2.cpp
# End Source File
# Begin Source File

SOURCE=.\src\C4Particles.cpp
# End Source File
# Begin Source File

SOURCE=.\src\C4PathFinder.cpp
# End Source File
# Begin Source File

SOURCE=.\src\C4Player.cpp
# End Source File
# Begin Source File

SOURCE=.\src\C4PlayerInfo.cpp
# End Source File
# Begin Source File

SOURCE=.\src\C4PlayerInfoConflicts.cpp
# End Source File
# Begin Source File

SOURCE=.\src\C4PlayerInfoListBox.cpp
# End Source File
# Begin Source File

SOURCE=.\src\C4PlayerList.cpp
# End Source File
# Begin Source File

SOURCE=.\src\C4PropertyDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\src\C4PXS.cpp
# End Source File
# Begin Source File

SOURCE=.\src\C4Random.cpp
# End Source File
# Begin Source File

SOURCE=.\src\C4RankSystem.cpp
# End Source File
# Begin Source File

SOURCE=.\src\C4Record.cpp
# End Source File
# Begin Source File

SOURCE=.\src\C4Region.cpp
# End Source File
# Begin Source File

SOURCE=.\src\C4RoundResults.cpp
# End Source File
# Begin Source File

SOURCE=.\src\C4RTF.CPP
# End Source File
# Begin Source File

SOURCE=.\src\C4Scenario.cpp
# End Source File
# Begin Source File

SOURCE=.\src\C4Scoreboard.cpp
# End Source File
# Begin Source File

SOURCE=.\src\C4Script.cpp
# End Source File
# Begin Source File

SOURCE=.\src\C4ScriptHost.cpp
# End Source File
# Begin Source File

SOURCE=.\src\C4Sector.cpp
# End Source File
# Begin Source File

SOURCE=.\sec\C4SecurityCertificates.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=.\src\C4Shape.cpp
# End Source File
# Begin Source File

SOURCE=.\src\C4Sky.cpp
# End Source File
# Begin Source File

SOURCE=.\src\C4SolidMask.cpp
# End Source File
# Begin Source File

SOURCE=.\src\C4SoundSystem.cpp
# End Source File
# Begin Source File

SOURCE=.\src\C4Startup.cpp
# End Source File
# Begin Source File

SOURCE=.\src\C4StartupAboutDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\src\C4StartupMainDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\src\C4StartupNetDlg.cpp

!IF  "$(CFG)" == "engine - Win32 Release"

!ELSEIF  "$(CFG)" == "engine - Win32 Debug"

!ELSEIF  "$(CFG)" == "engine - Win32 NoNetwork Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "engine - Win32 NoNetwork Debug"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\C4StartupOptionsDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\src\C4StartupPlrSelDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\src\C4StartupScenSelDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\src\C4Stat.cpp
# End Source File
# Begin Source File

SOURCE=.\src\C4StringTable.cpp
# End Source File
# Begin Source File

SOURCE=.\src\C4Surface.cpp
# End Source File
# Begin Source File

SOURCE=.\src\C4SurfaceFile.cpp
# End Source File
# Begin Source File

SOURCE=.\src\C4Teams.cpp
# End Source File
# Begin Source File

SOURCE=.\src\C4Texture.cpp
# End Source File
# Begin Source File

SOURCE=.\src\C4ToolsDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\src\C4TransferZone.cpp
# End Source File
# Begin Source File

SOURCE=.\src\C4UpdateDlg.cpp

!IF  "$(CFG)" == "engine - Win32 Release"

!ELSEIF  "$(CFG)" == "engine - Win32 Debug"

!ELSEIF  "$(CFG)" == "engine - Win32 NoNetwork Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "engine - Win32 NoNetwork Debug"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\C4UpperBoard.cpp
# End Source File
# Begin Source File

SOURCE=.\src\C4Value.cpp
# End Source File
# Begin Source File

SOURCE=.\src\C4ValueList.cpp
# End Source File
# Begin Source File

SOURCE=.\src\C4ValueMap.cpp
# End Source File
# Begin Source File

SOURCE=.\src\C4Video.cpp
# End Source File
# Begin Source File

SOURCE=.\src\C4VideoPlayback.cpp
# End Source File
# Begin Source File

SOURCE=.\src\C4Viewport.cpp
# End Source File
# Begin Source File

SOURCE=.\src\C4Weather.cpp
# End Source File
# Begin Source File

SOURCE=.\src\C4WinMain.cpp
# End Source File
# Begin Source File

SOURCE=.\src\C4Wrappers.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h"
# Begin Source File

SOURCE=.\inc\C4AList.h
# End Source File
# Begin Source File

SOURCE=.\inc\C4Application.h
# End Source File
# Begin Source File

SOURCE=.\inc\C4Aul.h
# End Source File
# Begin Source File

SOURCE=.\inc\C4ChatDlg.h
# End Source File
# Begin Source File

SOURCE=.\inc\C4Client.h
# End Source File
# Begin Source File

SOURCE=.\inc\C4Command.h
# End Source File
# Begin Source File

SOURCE=.\inc\C4ComponentHost.h
# End Source File
# Begin Source File

SOURCE=.\inc\C4Components.h
# End Source File
# Begin Source File

SOURCE=.\inc\C4Config.h
# End Source File
# Begin Source File

SOURCE=.\sec\C4ConfigShareware.h
# End Source File
# Begin Source File

SOURCE=.\inc\C4Console.h
# End Source File
# Begin Source File

SOURCE=.\inc\C4Constants.h
# End Source File
# Begin Source File

SOURCE=.\inc\C4Control.h
# End Source File
# Begin Source File

SOURCE=.\inc\C4Def.h
# End Source File
# Begin Source File

SOURCE=.\inc\C4DefGraphics.h
# End Source File
# Begin Source File

SOURCE=.\inc\C4DownloadDlg.h
# End Source File
# Begin Source File

SOURCE=.\inc\C4EditCursor.h
# End Source File
# Begin Source File

SOURCE=.\inc\C4Effects.h
# End Source File
# Begin Source File

SOURCE=.\inc\C4Extra.h
# End Source File
# Begin Source File

SOURCE=.\inc\C4Facet.h
# End Source File
# Begin Source File

SOURCE=.\inc\C4FacetEx.h
# End Source File
# Begin Source File

SOURCE=.\inc\C4FileClasses.h
# End Source File
# Begin Source File

SOURCE=.\inc\C4FileMonitor.h
# End Source File
# Begin Source File

SOURCE=.\inc\C4FileSelDlg.h
# End Source File
# Begin Source File

SOURCE=.\inc\C4FindObject.h
# End Source File
# Begin Source File

SOURCE=.\inc\C4FogOfWar.h
# End Source File
# Begin Source File

SOURCE=.\inc\C4Folder.h
# End Source File
# Begin Source File

SOURCE=.\inc\C4Fonts.h
# End Source File
# Begin Source File

SOURCE=.\inc\C4FullScreen.h
# End Source File
# Begin Source File

SOURCE=.\inc\C4Game.h
# End Source File
# Begin Source File

SOURCE=.\inc\C4GameControl.h
# End Source File
# Begin Source File

SOURCE=.\inc\C4GameControlNetwork.h
# End Source File
# Begin Source File

SOURCE=.\inc\C4GameDialogs.h
# End Source File
# Begin Source File

SOURCE=.\inc\C4GameLobby.h
# End Source File
# Begin Source File

SOURCE=.\inc\C4GameMessage.h
# End Source File
# Begin Source File

SOURCE=.\inc\C4GameObjects.h
# End Source File
# Begin Source File

SOURCE=.\inc\C4GameOptions.h
# End Source File
# Begin Source File

SOURCE=.\inc\C4GameOverDlg.h
# End Source File
# Begin Source File

SOURCE=.\inc\C4GamePadCon.h
# End Source File
# Begin Source File

SOURCE=.\inc\C4GameParameters.h
# End Source File
# Begin Source File

SOURCE=.\inc\C4GameSave.h
# End Source File
# Begin Source File

SOURCE=.\inc\C4GameVersion.h
# End Source File
# Begin Source File

SOURCE=.\inc\C4GraphicsResource.h
# End Source File
# Begin Source File

SOURCE=.\inc\C4GraphicsSystem.h
# End Source File
# Begin Source File

SOURCE=.\inc\C4Group.h
# End Source File
# Begin Source File

SOURCE=.\inc\C4GroupSet.h
# End Source File
# Begin Source File

SOURCE=.\inc\C4Gui.h
# End Source File
# Begin Source File

SOURCE=.\inc\C4Id.h
# End Source File
# Begin Source File

SOURCE=.\inc\C4IDList.h
# End Source File
# Begin Source File

SOURCE=.\inc\C4Include.h
# End Source File
# Begin Source File

SOURCE=.\inc\C4InfoCore.h
# End Source File
# Begin Source File

SOURCE=.\inc\C4InputValidation.h
# End Source File
# Begin Source File

SOURCE=.\inc\C4InteractiveThread.h
# End Source File
# Begin Source File

SOURCE=.\inc\C4KeyboardInput.h
# End Source File
# Begin Source File

SOURCE=.\inc\C4Landscape.h
# End Source File
# Begin Source File

SOURCE=.\inc\C4LangStringTable.h
# End Source File
# Begin Source File

SOURCE=.\inc\C4Language.h
# End Source File
# Begin Source File

SOURCE=.\inc\C4League.h

!IF  "$(CFG)" == "engine - Win32 Release"

!ELSEIF  "$(CFG)" == "engine - Win32 Debug"

!ELSEIF  "$(CFG)" == "engine - Win32 NoNetwork Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "engine - Win32 NoNetwork Debug"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\inc\C4LoaderScreen.h
# End Source File
# Begin Source File

SOURCE=.\inc\C4Log.h
# End Source File
# Begin Source File

SOURCE=.\inc\C4LogBuf.h
# End Source File
# Begin Source File

SOURCE=.\inc\C4MainMenu.h
# End Source File
# Begin Source File

SOURCE=.\inc\C4Map.h
# End Source File
# Begin Source File

SOURCE=.\inc\C4MapCreatorS2.h
# End Source File
# Begin Source File

SOURCE=.\inc\C4MassMover.h
# End Source File
# Begin Source File

SOURCE=.\inc\C4Material.h
# End Source File
# Begin Source File

SOURCE=.\inc\C4MaterialList.h
# End Source File
# Begin Source File

SOURCE=.\inc\C4Menu.h
# End Source File
# Begin Source File

SOURCE=.\inc\C4MessageBoard.h
# End Source File
# Begin Source File

SOURCE=.\inc\C4MessageInput.h
# End Source File
# Begin Source File

SOURCE=.\inc\C4MouseControl.h
# End Source File
# Begin Source File

SOURCE=.\inc\C4MusicFile.h
# End Source File
# Begin Source File

SOURCE=.\inc\C4MusicSystem.h
# End Source File
# Begin Source File

SOURCE=.\inc\C4NameList.h
# End Source File
# Begin Source File

SOURCE=.\inc\C4NetIO.h
# End Source File
# Begin Source File

SOURCE=.\inc\C4Network2.h
# End Source File
# Begin Source File

SOURCE=.\inc\C4Network2Client.h
# End Source File
# Begin Source File

SOURCE=.\inc\C4Network2Dialogs.h
# End Source File
# Begin Source File

SOURCE=.\inc\C4Network2Discover.h
# End Source File
# Begin Source File

SOURCE=.\inc\C4Network2IO.h
# End Source File
# Begin Source File

SOURCE=.\inc\C4Network2IRC.h
# End Source File
# Begin Source File

SOURCE=.\inc\C4Network2Players.h
# End Source File
# Begin Source File

SOURCE=.\inc\C4Network2Reference.h
# End Source File
# Begin Source File

SOURCE=.\inc\C4Network2Res.h
# End Source File
# Begin Source File

SOURCE=.\inc\C4Network2Stats.h
# End Source File
# Begin Source File

SOURCE=.\inc\C4Object.h
# End Source File
# Begin Source File

SOURCE=.\inc\C4ObjectCom.h
# End Source File
# Begin Source File

SOURCE=.\inc\C4ObjectInfo.h
# End Source File
# Begin Source File

SOURCE=.\inc\C4ObjectInfoList.h
# End Source File
# Begin Source File

SOURCE=.\inc\C4ObjectList.h
# End Source File
# Begin Source File

SOURCE=.\inc\C4ObjectListDlg.h
# End Source File
# Begin Source File

SOURCE=.\inc\C4ObjectMenu.h
# End Source File
# Begin Source File

SOURCE=.\inc\C4PacketBase.h
# End Source File
# Begin Source File

SOURCE=.\inc\C4Particles.h
# End Source File
# Begin Source File

SOURCE=.\inc\C4PathFinder.h
# End Source File
# Begin Source File

SOURCE=.\inc\C4Physics.h
# End Source File
# Begin Source File

SOURCE=.\inc\C4Player.h
# End Source File
# Begin Source File

SOURCE=.\inc\C4PlayerInfo.h
# End Source File
# Begin Source File

SOURCE=.\inc\C4PlayerInfoListBox.h
# End Source File
# Begin Source File

SOURCE=.\inc\C4PlayerList.h
# End Source File
# Begin Source File

SOURCE=.\inc\C4PropertyDlg.h
# End Source File
# Begin Source File

SOURCE=.\inc\C4Prototypes.h
# End Source File
# Begin Source File

SOURCE=.\inc\C4PXS.h
# End Source File
# Begin Source File

SOURCE=.\inc\C4Random.h
# End Source File
# Begin Source File

SOURCE=.\inc\C4RankSystem.h
# End Source File
# Begin Source File

SOURCE=.\inc\C4Record.h
# End Source File
# Begin Source File

SOURCE=.\inc\C4Region.h
# End Source File
# Begin Source File

SOURCE=.\inc\C4RoundResults.h
# End Source File
# Begin Source File

SOURCE=.\inc\C4RTF.H
# End Source File
# Begin Source File

SOURCE=.\inc\C4Scenario.h
# End Source File
# Begin Source File

SOURCE=.\inc\C4Scoreboard.h
# End Source File
# Begin Source File

SOURCE=.\inc\C4Script.h
# End Source File
# Begin Source File

SOURCE=.\inc\C4ScriptHost.h
# End Source File
# Begin Source File

SOURCE=.\inc\C4Sector.h
# End Source File
# Begin Source File

SOURCE=.\inc\C4Shape.h
# End Source File
# Begin Source File

SOURCE=.\inc\C4Sky.h
# End Source File
# Begin Source File

SOURCE=.\inc\C4SolidMask.h
# End Source File
# Begin Source File

SOURCE=.\inc\C4SoundSystem.h
# End Source File
# Begin Source File

SOURCE=.\inc\C4Startup.h
# End Source File
# Begin Source File

SOURCE=.\inc\C4StartupAboutDlg.h
# End Source File
# Begin Source File

SOURCE=.\inc\C4StartupMainDlg.h
# End Source File
# Begin Source File

SOURCE=.\inc\C4StartupNetDlg.h
# End Source File
# Begin Source File

SOURCE=.\inc\C4StartupOptionsDlg.h
# End Source File
# Begin Source File

SOURCE=.\inc\C4StartupPlrSelDlg.h
# End Source File
# Begin Source File

SOURCE=.\inc\C4StartupScenSelDlg.h
# End Source File
# Begin Source File

SOURCE=.\inc\C4Stat.h
# End Source File
# Begin Source File

SOURCE=.\inc\C4StringTable.h
# End Source File
# Begin Source File

SOURCE=.\inc\C4Surface.h
# End Source File
# Begin Source File

SOURCE=.\inc\C4SurfaceFile.h
# End Source File
# Begin Source File

SOURCE=.\inc\C4Teams.h
# End Source File
# Begin Source File

SOURCE=.\inc\C4Texture.h
# End Source File
# Begin Source File

SOURCE=.\inc\C4ToolsDlg.h
# End Source File
# Begin Source File

SOURCE=.\inc\C4TransferZone.h
# End Source File
# Begin Source File

SOURCE=.\inc\C4UpdateDlg.h
# End Source File
# Begin Source File

SOURCE=.\inc\C4UpperBoard.h
# End Source File
# Begin Source File

SOURCE=.\inc\C4UserMessages.h
# End Source File
# Begin Source File

SOURCE=.\inc\C4Value.h
# End Source File
# Begin Source File

SOURCE=.\inc\C4ValueList.h
# End Source File
# Begin Source File

SOURCE=.\inc\C4ValueMap.h
# End Source File
# Begin Source File

SOURCE=.\inc\C4Version.h
# End Source File
# Begin Source File

SOURCE=.\inc\C4Video.h
# End Source File
# Begin Source File

SOURCE=.\inc\C4VideoPlayback.h
# End Source File
# Begin Source File

SOURCE=.\inc\C4Viewport.h
# End Source File
# Begin Source File

SOURCE=.\inc\C4Weather.h
# End Source File
# Begin Source File

SOURCE=.\inc\C4Wrappers.h
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
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\res\Brush.bmp
# End Source File
# Begin Source File

SOURCE=.\res\brush1.bmp
# End Source File
# Begin Source File

SOURCE=.\res\Brush2.bmp
# End Source File
# Begin Source File

SOURCE=.\res\c4b.ico
# End Source File
# Begin Source File

SOURCE=.\res\c4d.ico
# End Source File
# Begin Source File

SOURCE=.\res\c4f.ico
# End Source File
# Begin Source File

SOURCE=.\res\c4g.ico
# End Source File
# Begin Source File

SOURCE=.\res\c4i.ico
# End Source File
# Begin Source File

SOURCE=.\res\c4k.ico
# End Source File
# Begin Source File

SOURCE=.\res\c4l.ico
# End Source File
# Begin Source File

SOURCE=.\res\c4m.ico
# End Source File
# Begin Source File

SOURCE=.\res\c4p.ico
# End Source File
# Begin Source File

SOURCE=.\res\c4s.ico
# End Source File
# Begin Source File

SOURCE=.\res\c4u.ico
# End Source File
# Begin Source File

SOURCE=.\res\c4v.ico
# End Source File
# Begin Source File

SOURCE=.\res\c4x.ico
# End Source File
# Begin Source File

SOURCE=.\res\cp.ico
# End Source File
# Begin Source File

SOURCE=.\res\cr.ico
# End Source File
# Begin Source File

SOURCE=.\res\Cursor.bmp
# End Source File
# Begin Source File

SOURCE=.\res\Cursor2.bmp
# End Source File
# Begin Source File

SOURCE=.\res\dynamic1.bmp
# End Source File
# Begin Source File

SOURCE=.\res\engine.rc

!IF  "$(CFG)" == "engine - Win32 Release"

!ELSEIF  "$(CFG)" == "engine - Win32 Debug"

# ADD BASE RSC /l 0x407 /i "res"
# ADD RSC /l 0x407 /i "res" /d "C4SHAREWARE"

!ELSEIF  "$(CFG)" == "engine - Win32 NoNetwork Release"

!ELSEIF  "$(CFG)" == "engine - Win32 NoNetwork Debug"

# ADD BASE RSC /l 0x407 /i "res" /d "C4SHAREWARE"
# ADD RSC /l 0x407 /i "res" /d "C4SHAREWARE"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\res\Fill.bmp
# End Source File
# Begin Source File

SOURCE=.\res\fill1.bmp
# End Source File
# Begin Source File

SOURCE=.\res\Halt.bmp
# End Source File
# Begin Source File

SOURCE=.\res\Halt2.bmp
# End Source File
# Begin Source File

SOURCE=.\res\IFT.bmp
# End Source File
# Begin Source File

SOURCE=.\res\ift1.bmp
# End Source File
# Begin Source File

SOURCE=.\res\Line.bmp
# End Source File
# Begin Source File

SOURCE=.\res\line1.bmp
# End Source File
# Begin Source File

SOURCE=.\res\mouse.bmp
# End Source File
# Begin Source File

SOURCE=.\res\mouse1.bmp
# End Source File
# Begin Source File

SOURCE=.\res\NoIFT.bmp
# End Source File
# Begin Source File

SOURCE=.\res\picker1.bmp
# End Source File
# Begin Source File

SOURCE=.\res\Play.bmp
# End Source File
# Begin Source File

SOURCE=.\res\Play2.bmp
# End Source File
# Begin Source File

SOURCE=.\res\rect1.bmp
# End Source File
# Begin Source File

SOURCE=.\res\Rectangle.bmp
# End Source File
# Begin Source File

SOURCE=.\res\static1.bmp
# End Source File
# End Group
# Begin Group "Library Files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\fmod\fmodvc.lib
# End Source File
# Begin Source File

SOURCE=..\Standard\openssl\libeay32.lib
# End Source File
# Begin Source File

SOURCE=..\Standard\openssl\libeay32_d.lib
# End Source File
# End Group
# End Target
# End Project
