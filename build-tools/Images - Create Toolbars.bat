cd /d %~dp0

set msbuild="C:\Program Files\Microsoft Visual Studio\18\Professional\MSBuild\Current\Bin\MSBuild.exe"
if exist %msbuild% goto :start
set msbuild="C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\MSBuild.exe"

:start
%msbuild% build-tools.sln /p:Configuration=Debug /p:Platform=x64 /target:"Toolbar Creator"
set gh="build\x64\Debug\bin\Toolbar Creator.exe"


REM 32-bit PNG toolbars (icons are 16x15, file saved with height 15)

%gh% "Image Inputs\Toolbar - CSCode - Main Frame.txt"        "..\cspro\CSCode\res\Toolbar MainFrame.png"
%gh% "Image Inputs\Toolbar - CSCode - Code Frame.txt"        "..\cspro\CSCode\res\Toolbar CodeFrame.png"
%gh% "Image Inputs\Toolbar - CSCode - Extra Icons.txt"       "..\cspro\CSCode\res\Toolbar Extra Icons.png"

%gh% "Image Inputs\Toolbar - CSDocument - Main Frame.txt"    "..\cspro\CSDocument\res\Toolbar MainFrame.png"
%gh% "Image Inputs\Toolbar - CSDocument - Doc Frame.txt"     "..\cspro\CSDocument\res\Toolbar DocFrame.png"
%gh% "Image Inputs\Toolbar - CSDocument - Extra Icons.txt"   "..\cspro\CSDocument\res\Toolbar Extra Icons.png"


REM 32-bit PNG toolbars (icons are 20x19, file saved with height 19)

%gh% "Image Inputs\Toolbar - zFormF - Question Text.txt"     "..\cspro\zFormF\res\qsf_editor_toolbar.png" 20


REM BMP toolbars (icons are 16x15, file saved with height 16)

%gh% "Image Inputs\Toolbar - CSDiff.txt"                     "..\cspro\CSDiff\res\Toolbar (save as 16).bmp"
%gh% "Image Inputs\Toolbar - CSEntry.txt"                    "..\cspro\CSEntry\res\Toolbar (save as 16).bmp"
%gh% "Image Inputs\Toolbar - CSExport.txt"                   "..\cspro\CSExport\res\mainfram (save as 16).bmp"
%gh% "Image Inputs\Toolbar - CSFreq.txt"                     "..\cspro\CSFreq\res\Toolbar (save as 16).bmp"
%gh% "Image Inputs\Toolbar - CSPro.txt"                      "..\cspro\CSPro\res\Toolbar (save as 256).bmp"
%gh% "Image Inputs\Toolbar - CSSort.txt"                     "..\cspro\CSSort\res\Toolbar (save as 16).bmp"
%gh% "Image Inputs\Toolbar - TblView.txt"                    "..\cspro\TblView\res\mainfram (save as 16).bmp"
%gh% "Image Inputs\Toolbar - TblView - Toolbar.txt"          "..\cspro\TblView\res\Toolbar (save as 16).bmp"
%gh% "Image Inputs\Toolbar - TextView.txt"                   "..\cspro\TextView\res\Toolbar (save as 16).bmp"
%gh% "Image Inputs\Toolbar - zDictF.txt"                     "..\cspro\zDictF\res\Toolbar (save as 16).bmp"
%gh% "Image Inputs\Toolbar - zFormF.txt"                     "..\cspro\zFormF\res\Toolbar (save as 16).bmp"
%gh% "Image Inputs\Toolbar - zFormF - Box.txt"               "..\cspro\zFormF\res\box_tool (save as 16).bmp"
%gh% "Image Inputs\Toolbar - zOrderF.txt"                    "..\cspro\zOrderF\res\Toolbar (save as 16).bmp"
%gh% "Image Inputs\Toolbar - zTableF.txt"                    "..\cspro\zTableF\res\Toolbar (save as 16).bmp"
%gh% "Image Inputs\Toolbar - zTableF - Print Navigation.txt" "..\cspro\zTableF\res\toolbar1 (save as 16).bmp"
