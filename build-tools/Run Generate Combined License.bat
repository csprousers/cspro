cd /d %~dp0

set msbuild="C:\Program Files\Microsoft Visual Studio\2022\Professional\MSBuild\Current\Bin\MSBuild.exe"
if exist %msbuild% goto :start
set msbuild="C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe"

:start
del "Licenses\Licenses.html"

%msbuild% "..\cspro\cspro.sln" /p:Configuration=Debug /target:zUtilO

%msbuild% "Licenses\Generate Combined License\Generate Combined License.sln" /p:Configuration=Debug
"Licenses\Generate Combined License\Debug\Generate Combined License.exe"

copy /y "Licenses\Licenses.html" "..\cspro\CSEntryDroid\app\src\main\assets\Licenses.html"
