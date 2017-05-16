@echo off

rem Set path to help workshop manually here (or better leave it blank
rem and configure your PATH so that hcw.exe is found anyway...
set HCWPATH=C:\Programme\Microsoft Visual Studio\Common\Tools\

echo MakeHelpUS
echo // MAKEHELP.BAT generated Help Map file.  Used by ClonkUS.hpj. >"frontend.hm"
echo. >>"frontend.hm"

echo Controls...
echo // Controls (IDC_*) [mawic-Geheimmethode. Nebenwirkungen unbekannt.] >>"frontend.hm"
"%HCWPATH%makehm.exe" IDC_,IDC_,0x00000 ../res/resource.h >>"frontend.hm"

echo Commands...
echo // Commands (ID_* and IDM_*) >>"frontend.hm"
"%HCWPATH%makehm.exe" ID_,HID_,0x10000 IDM_,HIDM_,0x10000 ../res/resource.h >>"frontend.hm"
echo. >>"frontend.hm"

echo Prompts...
echo // Prompts (IDP_*) >>"frontend.hm"
"%HCWPATH%makehm.exe" IDP_,HIDP_,0x30000 ../res/resource.h >>"frontend.hm"
echo. >>"frontend.hm"

echo Resources...
echo // Resources (IDR_*) >>"frontend.hm"
"%HCWPATH%makehm.exe" IDR_,HIDR_,0x20000 ../res/resource.h >>"frontend.hm"
echo. >>"frontend.hm"

echo Dialogs...
echo // Dialogs (IDD_*) >>"frontend.hm"
"%HCWPATH%makehm.exe" IDD_,HIDD_,0x20000 ../res/resource.h >>"frontend.hm"
echo. >>"frontend.hm"

echo Special...
echo // Special >>"frontend.hm"
echo IDC_OK                                  0x00001 >> "frontend.hm"
echo IDC_CANCEL                              0x00002 >> "frontend.hm"
echo. >>"frontend.hm"

echo Running HelpWorkshop...
"%HCWPATH%hcw.exe" /C /E /M "ClonkUS.hpj"
rem if errorlevel 1 goto :Error
rem if not exist "ClonkUS.hlp" goto :Error
rem if not exist "ClonkUS.cnt" goto :Error

echo Copying help files...
copy ClonkUS.hlp ..\..\planet\System.c4g\ClonkUS.hlp
copy ClonkUS.cnt ..\..\planet\System.c4g\ClonkUS.cnt

echo And for testing...
md test
copy ClonkUS.hlp test\Clonk.hlp
copy ClonkUS.cnt test\Clonk.cnt

echo Removing temporary output files...
del ClonkUS.hlp

echo Clearing help files from game directory to ensure update on next program start...
del ..\..\planet\*.hlp
del ..\..\planet\*.cnt

goto :done

:Error
echo hlp\ClonkUS.hpj(1) : error: Problem encountered creating help file

:done
echo.
