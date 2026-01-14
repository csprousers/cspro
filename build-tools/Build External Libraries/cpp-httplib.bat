cd /d %~dp0

rem ... create a working directory
rmdir /s /q temp\httplib
mkdir temp\httplib
cd temp\httplib


rem ... find the latest version number here: https://github.com/yhirose/cpp-httplib/releases/latest/
set httplib_version=0.30.1


rem ... get the latest version
curl -L -o httplib.tar.gz https://github.com/yhirose/cpp-httplib/archive/refs/tags/v%httplib_version%.tar.gz
tar -xvzf httplib.tar.gz


rem ... copy files to be used by CSPro
copy /y cpp-httplib-%httplib_version%\httplib.h ..\..\..\..\cspro\external\cpp-httplib\


rem ... update the license
copy /y cpp-httplib-%httplib_version%\LICENSE ..\..\..\Licenses\Licenses\cpp-httplib.txt
