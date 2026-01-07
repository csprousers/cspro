cd /d %~dp0

rem ... create a working directory
rmdir /s /q temp\libexif
mkdir temp\libexif
cd temp\libexif


rem ... find the latest version number here: https://github.com/libexif/libexif/releases/latest/
set lx_version=0.6.25


rem ... get the latest version
curl -L -o libexif.tar.gz https://github.com/libexif/libexif/archive/refs/tags/v%lx_version%.tar.gz
tar -xvzf libexif.tar.gz


rem ... copy files to be used by CSPro
xcopy .\libexif-%lx_version%\libexif ..\..\..\..\cspro\external\libexif /i /k /e /y


rem ... update the license
copy /y libexif-%lx_version%\COPYING ..\..\..\Licenses\Licenses\libexif.txt
