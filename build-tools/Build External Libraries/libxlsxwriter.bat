rem ... get the latest version
curl -L -o libxlsxwriter.tar.gz https://github.com/jmcnamara/libxlsxwriter/archive/master.tar.gz
tar -xvzf libxlsxwriter.tar.gz


rem ... build the projects
rem cmake -G "Visual Studio 16 2019" -A Win32 -S libxlsxwriter-master -B libxlsxwriter-build-x86 -DZLIB_INCLUDE_DIR:STRING="../zlib-x86/include" -DZLIB_ROOT:STRING="../zlib-x86/lib"
rem cmake --build libxlsxwriter-build-x86 --config Release

rem cmake -G "Visual Studio 16 2019" -A x64 -S libxlsxwriter-master -B libxlsxwriter-build-x64 -DZLIB_INCLUDE_DIR:STRING="../zlib-x64/include" -DZLIB_ROOT:STRING="../zlib-x64/lib"
rem cmake --build libxlsxwriter-build-x64 --config Release


rem ... copy files to be used by CSPro
rem xcopy libxlsxwriter-master\include ..\..\cspro\external\libxlsxwriter\include /i /k /e /y
rem copy /y libxlsxwriter-build-x86\Release\xlsxwriter.lib ..\..\cspro\external\libxlsxwriter\lib\x86
rem copy /y libxlsxwriter-build-x64\Release\xlsxwriter.lib ..\..\cspro\external\libxlsxwriter\lib\x64
