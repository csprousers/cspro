cd /d %~dp0

rem ... create a working directory
rmdir /s /q temp\leaflet-ajax
mkdir temp\leaflet-ajax
cd temp\leaflet-ajax


rem ... find the latest tag here: https://github.com/calvinmetcalf/leaflet-ajax/tags
set leaflet_ajax_tag=2.1.0


rem ... get the latest version
curl -L -o leaflet-ajax.tar.gz https://github.com/calvinmetcalf/leaflet-ajax/archive/refs/tags/v%leaflet_ajax_tag%.tar.gz
tar -xvzf leaflet-ajax.tar.gz


rem ... copy files to be used by CSPro
copy /y leaflet-ajax-%leaflet_ajax_tag%\dist\leaflet.ajax.min.js ..\..\..\..\cspro\html\external\leaflet\


rem ... update the license
copy /y leaflet-ajax-%leaflet_ajax_tag%\license.md ..\..\..\Licenses\Licenses\leaflet-ajax.txt
