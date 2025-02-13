cd /d %~dp0

set msbuild="C:\Program Files\Microsoft Visual Studio\2022\Professional\MSBuild\Current\Bin\MSBuild.exe"
if exist %msbuild% goto :start
set msbuild="C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe"

:start
%msbuild% "..\cspro\cspro.sln" /p:Configuration=Debug /target:zToolsO
%msbuild% "..\cspro\cspro.sln" /p:Configuration=Debug /target:zJson

%msbuild% "Resource ID Numberer\Resource ID Numberer.sln" /p:Configuration=Debug
"Resource ID Numberer\Debug\Resource ID Numberer.exe" "Resource ID Numberer\Resource File IDs.json"
