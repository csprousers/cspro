cd /d %~dp0

rem ... create a working directory
rmdir /s /q temp\easylogging
mkdir temp\easylogging
cd temp\easylogging


rem ... find the latest version number here: https://github.com/abumq/easyloggingpp/releases/latest/
set el_version=9.97.1


rem ... get the latest version
curl -L -o easylogging.tar.gz https://github.com/abumq/easyloggingpp/archive/refs/tags/v%el_version%.tar.gz
tar -xvzf easylogging.tar.gz


rem ... copy files to be used by CSPro
xcopy .\easyloggingpp-%el_version%\src ..\..\..\..\cspro\external\easylogging /i /k /e /y


rem ... update the license
copy /y easyloggingpp-%el_version%\LICENSE "..\..\..\Licenses\Licenses\Easylogging++.txt"
