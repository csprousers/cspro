cd /d %~dp0

rem ... create a working directory
rmdir /s /q temp\libgit2
mkdir temp\libgit2
cd temp\libgit2


rem ... find the latest version number here: https://github.com/libgit2/libgit2/releases/latest/
set lg_version=1.9.2


rem ... get the latest version
curl -L -o libgit2.tar.gz https://github.com/libgit2/libgit2/archive/refs/tags/v%lg_version%.tar.gz
tar -xvzf libgit2.tar.gz


rem ... build the project
cd libgit2-%lg_version%
cmake -G "Visual Studio 18 2026" -A x64 -S . -B build
cmake --build build --config Release


rem ... copy files to be used by CSPro
xcopy .\include\ ..\..\..\..\..\cspro\external\libgit2\include\ /i /k /e /y
xcopy build\Release\git2.dll ..\..\..\..\..\cspro\external\libgit2\lib\x64\ /i /k /y
xcopy build\Release\git2.lib ..\..\..\..\..\cspro\external\libgit2\lib\x64\ /i /k /y


rem ... update the license
copy /y COPYING ..\..\..\..\Licenses\Licenses\libgit2.txt
