cd /d %~dp0

rem ... create a working directory
rmdir /s /q temp\quickjs
mkdir temp\quickjs
cd temp\quickjs


rem ... find the latest version number here: https://github.com/quickjs-ng/quickjs/releases/latest/
set quickjs_version=0.11.0


rem ... get the latest version
curl -L -o quickjs.tar.gz https://github.com/quickjs-ng/quickjs/archive/refs/tags/v%quickjs_version%.tar.gz
tar -xvzf quickjs.tar.gz


rem ... copy files to be used by CSPro
copy /y quickjs-%quickjs_version%\*.c ..\..\..\..\cspro\external\QuickJS\
copy /y quickjs-%quickjs_version%\*.h ..\..\..\..\cspro\external\quickjs\


rem ... update the license
copy /y quickjs-%quickjs_version%\LICENSE ..\..\..\Licenses\Licenses\QuickJS-NG.txt
