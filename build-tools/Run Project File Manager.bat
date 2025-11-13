cd /d %~dp0

set msbuild="C:\Program Files\Microsoft Visual Studio\18\Professional\MSBuild\Current\Bin\MSBuild.exe"
if exist %msbuild% goto :start
set msbuild="C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\MSBuild.exe"

:start
%msbuild% "..\cspro\cspro.sln" /p:Configuration=Debug /target:zUtilO

%msbuild% "Project File Manager\Project File Manager.sln" /p:Configuration=Debug
"Project File Manager\Debug\Project File Manager.exe"
