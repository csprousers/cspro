cd /d %~dp0

set msbuild="C:\Program Files\Microsoft Visual Studio\18\Professional\MSBuild\Current\Bin\MSBuild.exe"
if exist %msbuild% goto :start
set msbuild="C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\MSBuild.exe"

:start
del "Licenses\Licenses.html"

%msbuild% "build-tools.sln" /p:Configuration=Debug /p:Platform=x64 /t:Build /target:"Generate Combined License"
"build\x64\Debug\bin\Generate Combined License.exe"

copy /y "Licenses\Licenses.html" "..\cspro\CSEntryDroid\app\src\main\assets\Licenses.html"
