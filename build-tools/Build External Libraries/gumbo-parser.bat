cd /d %~dp0

rem ... create a working directory
rmdir /s /q temp\gumbo-parser
mkdir temp\gumbo-parser
cd temp\gumbo-parser


rem ... find the latest version number here: https://github.com/google/gumbo-parser/releases/latest/
set gp_version=0.10.1


rem ... get the latest version
curl -L -o gumbo-parser.tar.gz https://github.com/google/gumbo-parser/archive/refs/tags/v%gp_version%.tar.gz
tar -xvzf gumbo-parser.tar.gz


rem ... copy files to be used by CSPro
xcopy .\gumbo-parser-%gp_version%\src\*.c ..\..\..\..\cspro\external\gumbo /i /k /e /y
xcopy .\gumbo-parser-%gp_version%\src\*.h ..\..\..\..\cspro\external\gumbo /i /k /e /y


rem ... update the license
copy /y gumbo-parser-%gp_version%\COPYING ..\..\..\Licenses\Licenses\gumbo-parser.txt
