cd /d %~dp0

rem ... create a working directory
rmdir /s /q temp\zlib
mkdir temp\zlib
cd temp\zlib


rem ... get the latest version
curl -L -o zlib.tar.gz https://github.com/madler/zlib/archive/master.tar.gz
tar -xvzf zlib.tar.gz


rem ... build the projects
cmake -G "Visual Studio 18 2026" -A Win32 -S zlib-master -B zlib-build-x86
cmake --build zlib-build-x86 --config Release

cmake -G "Visual Studio 18 2026" -A x64 -S zlib-master -B zlib-build-x64
cmake --build zlib-build-x64 --config Release


rem ... copy files to be used by other projects dependent on zlib
xcopy zlib-master\*.h zlib-x86\include /i /k /y
xcopy zlib-build-x86\*.h zlib-x86\include /i /k /y
xcopy zlib-build-x86\Release\*.* zlib-x86\lib /i /k /y

xcopy zlib-master\*.h zlib-x64\include /i /k /y
xcopy zlib-build-x64\*.h zlib-x64\include /i /k /y
xcopy zlib-build-x64\Release\*.* zlib-x64\lib /i /k /y


rem ... copy files to be used by CSPro
mkdir ..\..\..\..\cspro\external\zlib\lib\x86
copy /y zlib-build-x86\Release\zlib.dll ..\..\..\..\cspro\external\zlib\lib\x86\
copy /y zlib-build-x86\Release\zlib.lib ..\..\..\..\cspro\external\zlib\lib\x86\

mkdir ..\..\..\..\cspro\external\zlib\lib\x64
copy /y zlib-build-x64\Release\zlib.dll ..\..\..\..\cspro\external\zlib\lib\x64\
copy /y zlib-build-x64\Release\zlib.lib ..\..\..\..\cspro\external\zlib\lib\x64\


rem ... copy a set of files that will be built on Android
xcopy zlib-master\*.c ..\..\..\..\cspro\external\zlib /i /k /y
xcopy zlib-master\*.h ..\..\..\..\cspro\external\zlib /i /k /y
xcopy zlib-build-x86\*.h ..\..\..\..\cspro\external\zlib /i /k /y


rem ... update the license
copy /y zlib-master\LICENSE ..\..\..\Licenses\Licenses\zlib.txt
