cd /d %~dp0

rem ... create a working directory
rmdir /s /q temp\jsoncons
mkdir temp\jsoncons
cd temp\jsoncons


rem ... find the latest version number here: https://github.com/danielaparker/jsoncons/releases/latest/
set jsoncons_version=1.5.0


rem ... get the latest version
curl -L -o jsoncons.tar.gz https://github.com/danielaparker/jsoncons/archive/refs/tags/v%jsoncons_version%.tar.gz
tar -xvzf jsoncons.tar.gz


rem ... copy files to be used by CSPro
xcopy .\jsoncons-%jsoncons_version%\include\jsoncons ..\..\..\..\cspro\external\jsoncons /i /k /e /y


rem ... update the license
copy /y jsoncons-%jsoncons_version%\LICENSE ..\..\..\Licenses\Licenses\jsoncons.txt
