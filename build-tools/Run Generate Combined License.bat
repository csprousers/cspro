set msbuild="C:\Program Files\Microsoft Visual Studio\2022\Professional\MSBuild\Current\Bin\MSBuild.exe"
if exist %msbuild% goto :start
set msbuild="C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe"

:start
cd /D "%~dp0"
del "Licenses\Licenses.html"

%msbuild% "Licenses\Generate Combined License\Generate Combined License.sln" /p:Configuration=Debug /t:Build
"Licenses\Generate Combined License\bin\Debug\Generate Combined License.exe"

copy /y "Licenses\Licenses.html" "..\cspro\CSEntryDroid\app\src\main\assets\Licenses.html"
