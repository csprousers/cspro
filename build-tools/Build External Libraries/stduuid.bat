cd /d %~dp0

rem ... create a working directory
rmdir /s /q temp\stduuid
mkdir temp\stduuid
cd temp\stduuid


rem ... find the latest version number here: https://github.com/mariusbancila/stduuid/releases/latest/
set stduuid_version=1.2.3


rem ... get the latest version
curl -L -o stduuid.tar.gz https://github.com/mariusbancila/stduuid/archive/refs/tags/v%stduuid_version%.tar.gz
tar -xvzf stduuid.tar.gz


rem ... copy files to be used by CSPro
xcopy .\stduuid-%stduuid_version%\include ..\..\..\..\cspro\external\stduuid /i /k /e /y
xcopy .\stduuid-%stduuid_version%\gsl ..\..\..\..\cspro\external\stduuid\gsl /i /k /e /y


rem ... update the license
copy /y stduuid-%stduuid_version%\LICENSE ..\..\..\Licenses\Licenses\stduuid.txt
