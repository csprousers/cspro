cd /d %~dp0

rem ... create a working directory
rmdir /s /q temp\mustache
mkdir temp\mustache
cd temp\mustache


rem ... find the latest version number here: https://github.com/janl/mustache.js/releases/latest/
set ms_version=4.2.0


rem ... get the latest version
curl -L -o mustache.min.js https://unpkg.com/mustache@%ms_version%/mustache.min.js


rem ... copy files to be used by CSPro
copy /y mustache.min.js ..\..\..\..\cspro\html\external\mustache\


rem ... copy files to be used by CSPro
curl -L -o LICENSE https://raw.githubusercontent.com/janl/mustache.js/refs/tags/v%ms_version%/LICENSE
copy /y  LICENSE ..\..\..\Licenses\Licenses\mustache.js.txt
