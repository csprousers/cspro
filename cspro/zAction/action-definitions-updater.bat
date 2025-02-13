cd /d %~dp0

set msbuild="C:\Program Files\Microsoft Visual Studio\2022\Professional\MSBuild\Current\Bin\MSBuild.exe"
if exist %msbuild% goto :start
set msbuild="C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe"

:start
%msbuild% "..\cspro.sln" /p:Configuration=Debug /target:zLogicO

%msbuild% "..\..\build-tools\Action Invoker Definition Updater\Action Invoker Definition Updater.sln" /p:Configuration=Debug
"..\..\build-tools\Action Invoker Definition Updater\Debug\Action Invoker Definition Updater.exe" ".."
