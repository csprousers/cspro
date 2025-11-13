set msbuild="C:\Program Files\Microsoft Visual Studio\18\Professional\MSBuild\Current\Bin\MSBuild.exe"
if exist %msbuild% goto :start
set msbuild="C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\MSBuild.exe"

:start
%msbuild% "Messages Processor\Messages Processor.sln" /p:Configuration=Debug /t:Build
"Messages Processor\Debug\Messages Processor.exe" assets
