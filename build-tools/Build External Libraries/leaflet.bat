cd /d %~dp0

rem ... create a working directory
rmdir /s /q temp\leaflet
mkdir temp\leaflet
cd temp\leaflet


rem ... find the latest version number here: https://github.com/Leaflet/Leaflet/releases/latest/
set leaflet_version=1.9.4


rem ... get the latest version
curl -L -o leaflet.zip https://github.com/Leaflet/Leaflet/releases/download/v%leaflet_version%/leaflet.zip
tar -xvzf leaflet.zip


rem ... copy files to be used by CSPro
xcopy dist ..\..\..\..\cspro\html\external\leaflet /i /k /e /y


rem ... update the license
curl -L -o LICENSE https://raw.githubusercontent.com/Leaflet/Leaflet/refs/tags/v%leaflet_version%/LICENSE
copy /y  LICENSE ..\..\..\Licenses\Licenses\Leaflet.txt
