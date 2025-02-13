cd /d %~dp0

set msbuild="C:\Program Files\Microsoft Visual Studio\2022\Professional\MSBuild\Current\Bin\MSBuild.exe"
if exist %msbuild% goto :start
set msbuild="C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe"

:start
%msbuild% "..\..\build-tools\CSPro Installer Generator\CSPro Installer Generator\CSPro Installer Generator.sln" /p:Configuration=Release /t:Build

"..\..\build-tools\CSPro Installer Generator\CSPro Installer Generator\bin\Release\CSPro Installer Generator.exe" /android-assets-update-html
