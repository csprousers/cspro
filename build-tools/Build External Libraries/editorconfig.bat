cd /d %~dp0

rem ... create a working directory
rmdir /s /q temp\editorconfig
mkdir temp\editorconfig
cd temp\editorconfig


rem ... find the latest version number and commit here: https://github.com/editorconfig/editorconfig-core-c/releases/latest/
set ec_version=v0.12.10
set ec_commit=99d757d1


rem ... get the latest version
curl -L -o editorconfig.zip https://github.com/editorconfig/editorconfig-core-c/releases/download/%ec_version%/editorconfig-core-c_%ec_commit%_x64.zip
mkdir editorconfig
tar -xf editorconfig.zip -C editorconfig


rem ... get the latest license
curl -L -o editorconfig-license https://raw.githubusercontent.com/editorconfig/editorconfig-core-c/refs/tags/%ec_version%/LICENSE
copy /y editorconfig-license ..\..\..\Licenses\Licenses\EditorConfig.txt


rem ... copy files to be used by CSPro
xcopy .\editorconfig\include\editorconfig\ ..\..\..\..\cspro\external\editorconfig /i /k /e /y
xcopy .\editorconfig\lib\editorconfig.lib ..\..\..\..\cspro\external\editorconfig\lib\x64\ /k /y
xcopy .\editorconfig\bin\editorconfig.dll ..\..\..\..\cspro\external\editorconfig\lib\x64\ /k /y
