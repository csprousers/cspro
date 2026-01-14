cd /d %~dp0

rem ... create a working directory
rmdir /s /q temp\chartjs
mkdir temp\chartjs
cd temp\chartjs


rem ... find the latest version number here: https://github.com/chartjs/Chart.js/releases/latest/
set cj_version=4.5.1


rem ... get the latest version
curl -L -o chartjs.zip https://github.com/chartjs/Chart.js/releases/download/v%cj_version%/chart.js-%cj_version%.tgz
tar -xvzf chartjs.zip


rem ... copy files to be used by CSPro
copy /y package\dist\chart.umd.js ..\..\..\..\cspro\html\external\chartjs\
copy /y package\dist\chart.umd.js.map ..\..\..\..\cspro\html\external\chartjs\


rem ... update the license
copy /y  package\LICENSE.md ..\..\..\Licenses\Licenses\Chart.js.txt
