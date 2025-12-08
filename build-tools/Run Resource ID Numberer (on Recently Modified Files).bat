cd /d %~dp0

set msbuild="C:\Program Files\Microsoft Visual Studio\18\Professional\MSBuild\Current\Bin\MSBuild.exe"
if exist %msbuild% goto :start
set msbuild="C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\MSBuild.exe"

:start
%msbuild% build-tools.sln /p:Configuration=Debug /p:Platform=x64 /target:"Resource ID Numberer"
"build\x64\Debug\bin\Resource ID Numberer.exe" "Resource ID Numberer\Resource File IDs.json" /recent
