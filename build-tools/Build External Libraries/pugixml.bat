cd /d %~dp0

rem ... create a working directory
rmdir /s /q temp\pugixml
mkdir temp\pugixml
cd temp\pugixml


rem ... find the latest version number here: https://github.com/zeux/pugixml/releases/latest/
set px_version=1.15


rem ... get the latest version
curl -L -o pugixml.tar.gz https://github.com/zeux/pugixml/releases/download/v%px_version%/pugixml-%px_version%.tar.gz
tar -xvzf pugixml.tar.gz


rem ... copy files to be used by CSPro
xcopy .\pugixml-%px_version%\src ..\..\..\..\cspro\external\pugixml /i /k /e /y


rem ... update the license
copy /y pugixml-%px_version%\LICENSE.md ..\..\..\Licenses\Licenses\pugixml.txt
