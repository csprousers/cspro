rem ... find the latest version number here: https://github.com/libgit2/libgit2/releases/latest/
set libgit2-version=1.9.0

rem ... get the latest version
curl -L -o libgit2.tar.gz https://github.com/libgit2/libgit2/archive/refs/tags/v%libgit2-version%%.tar.gz
tar -xvzf libgit2.tar.gz


rem ... build the project
cd libgit2-%libgit2-version%
mkdir build32
cd build32
cmake -G "Visual Studio 17 2022" -A Win32 -S .. -B build32
cmake --build .


rem ... copy files to be used by CSPro
xcopy ..\include\ ..\..\..\..\cspro\external\libgit2\include\ /i /k /e /y
xcopy Debug\git2.dll ..\..\..\..\cspro\external\libgit2\lib\ /i /k /y
xcopy Debug\git2.lib ..\..\..\..\cspro\external\libgit2\lib\ /i /k /y
