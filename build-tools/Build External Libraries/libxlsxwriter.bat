cd /d %~dp0

rem ... create a working directory
rmdir /s /q temp\libxlsxwriter
mkdir temp\libxlsxwriter
cd temp\libxlsxwriter


rem ... get the latest version
curl -L -o libxlsxwriter.tar.gz https://github.com/jmcnamara/libxlsxwriter/archive/main.tar.gz
tar -xvzf libxlsxwriter.tar.gz


rem ... copy files to be used by CSPro
xcopy libxlsxwriter-main\include ..\..\..\..\cspro\external\libxlsxwriter\include /i /k /e /y
xcopy libxlsxwriter-main\src ..\..\..\..\cspro\external\libxlsxwriter\src /i /k /e /y
xcopy libxlsxwriter-main\third_party ..\..\..\..\cspro\external\libxlsxwriter\third_party /i /k /e /y


rem ... update the license
copy /y libxlsxwriter-main\License.txt ..\..\..\Licenses\Licenses\libxlsxwriter.txt
