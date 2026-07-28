cd /d %~dp0

rem ... create a working directory
rmdir /s /q temp\fakeit
mkdir temp\fakeit
cd temp\fakeit


rem ... find the latest version number here: https://github.com/eranpeer/FakeIt/releases/latest
set fakeit_version=2.5.0


rem ... get the latest version
curl -L -o fakeit.tar.gz https://github.com/eranpeer/FakeIt/archive/refs/tags/%fakeit_version%.tar.gz
tar -xvzf fakeit.tar.gz


rem ... copy files to be used by CSPro
xcopy .\FakeIt-%fakeit_version%\single_header\mstest\fakeit.hpp ..\..\..\..\cspro\external\fakeit\ /i /k /e /y


rem ... update the license
copy /y fakeit-%fakeit_version%\LICENSE ..\..\..\Licenses\Licenses\FakeIt.txt
