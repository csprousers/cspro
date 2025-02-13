rem ... get the latest version
curl -L -o zlib.tar.gz https://github.com/madler/zlib/archive/master.tar.gz
tar -xvzf zlib.tar.gz


rem ... build the projects
cmake -G "Visual Studio 17 2022" -A Win32 -S zlib-master -B zlib-build-x86
cmake --build zlib-build-x86 --config Release

cmake -G "Visual Studio 17 2022" -A x64 -S zlib-master -B zlib-build-x64
cmake --build zlib-build-x64 --config Release


rem ... copy files to be used by other projects dependent on zlib
xcopy zlib-master\*.h zlib-x86\include /i /k /y
xcopy zlib-build-x86\*.h zlib-x86\include /i /k /y
xcopy zlib-build-x86\Release\*.* zlib-x86\lib /i /k /y

xcopy zlib-master\*.h zlib-x64\include /i /k /y
xcopy zlib-build-x64\*.h zlib-x64\include /i /k /y
xcopy zlib-build-x64\Release\*.* zlib-x64\lib /i /k /y


rem ... copy files to be used by CSPro
copy /y zlib-x86\lib\zlib.dll ..\..\cspro\external\zlib\lib\x86
copy /y zlib-x86\lib\zlib.lib ..\..\cspro\external\zlib\lib\x86

copy /y zlib-x64\lib\zlib.dll ..\..\cspro\external\zlib\lib\x64
copy /y zlib-x64\lib\zlib.lib ..\..\cspro\external\zlib\lib\x64


rem ... copy a set of files that will be built on Android
xcopy zlib-master\*.c ..\..\cspro\external\zlib /i /k /y
xcopy zlib-master\*.h ..\..\cspro\external\zlib /i /k /y
xcopy zlib-build-x86\*.h ..\..\cspro\external\zlib /i /k /y
