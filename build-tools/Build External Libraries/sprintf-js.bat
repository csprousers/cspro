cd /d %~dp0

rem ... create a working directory
rmdir /s /q temp\sprintf-js
mkdir temp\sprintf-js
cd temp\sprintf-js


rem ... find the latest tag here: https://github.com/alexei/sprintf.js/tags
set spjs_tag=1.1.3


rem ... get the latest version
curl -L -o sprintf-js.tar.gz https://github.com/alexei/sprintf.js/archive/refs/tags/%spjs_tag%.tar.gz
tar -xvzf sprintf-js.tar.gz


rem ... copy files to be used by CSPro
copy /y sprintf.js-%spjs_tag%\dist\sprintf.min.js ..\..\..\..\cspro\html\external\sprintf\
copy /y sprintf.js-%spjs_tag%\dist\sprintf.min.js.map ..\..\..\..\cspro\html\external\sprintf\


rem ... update the license
copy /y sprintf.js-%spjs_tag%\LICENSE ..\..\..\Licenses\Licenses\sprintf-js.txt
