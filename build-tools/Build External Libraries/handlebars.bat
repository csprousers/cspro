cd /d %~dp0

rem ... create a working directory
rmdir /s /q temp\handlebars
mkdir temp\handlebars
cd temp\handlebars


rem ... find the latest version number here: https://github.com/handlebars-lang/handlebars.js/releases/latest/
set hb_version=4.7.8


rem ... get the latest version
curl -L -o handlebars.min.js https://cdnjs.cloudflare.com/ajax/libs/handlebars.js/%hb_version%/handlebars.min.js


rem ... copy files to be used by CSPro
copy /y handlebars.min.js ..\..\..\..\cspro\html\external\handlebars\


rem ... update the license
curl -L -o LICENSE https://raw.githubusercontent.com/handlebars-lang/handlebars.js/refs/tags/v%hb_version%/LICENSE
copy /y  LICENSE ..\..\..\Licenses\Licenses\Handlebars.js.txt
