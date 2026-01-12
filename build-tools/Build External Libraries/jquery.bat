cd /d %~dp0

rem ... create a working directory
rmdir /s /q temp\jquery
mkdir temp\jquery
cd temp\jquery


rem ... find the latest version number here: https://github.com/jquery/jquery/releases/latest/
set jq_version=3.7.1


rem ... find the latest version number here: https://github.com/jquery/jquery-ui/releases/latest/
set jqui_ui_version=1.14.1


rem ... get the latest version
curl -L -o jquery.tar.gz https://github.com/jquery/jquery/archive/refs/tags/%jq_version%.tar.gz
tar -xvzf jquery.tar.gz

curl -L -o jquery_ui.targ.gz https://github.com/jquery/jquery-ui/archive/refs/tags/%jqui_ui_version%.tar.gz
tar -xvzf jquery_ui.targ.gz


rem ... copy files to be used by CSPro
copy /y jquery-%jq_version%\dist\jquery.min.* ..\..\..\..\cspro\html\external\jquery\

copy /y jquery-ui-%jqui_ui_version%\dist\jquery-ui.min.js  ..\..\..\..\cspro\html\external\jquery\
copy /y jquery-ui-%jqui_ui_version%\dist\themes\base\jquery-ui.min.css  ..\..\..\..\cspro\html\external\jquery\


rem ... update the license
copy /y jquery-%jq_version%\LICENSE.txt ..\..\..\Licenses\Licenses\jQuery.txt

copy /y jquery-ui-%jqui_ui_version%\LICENSE.txt "..\..\..\Licenses\Licenses\jQuery UI.txt"
