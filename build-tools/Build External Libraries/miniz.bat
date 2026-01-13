cd /d %~dp0

rem ... create a working directory
rmdir /s /q temp\miniz
mkdir temp\miniz
cd temp\miniz


rem ... find the latest version number here: https://github.com/richgel999/miniz/releases/latest/
set miniz_version=3.1.0


rem ... get the latest version
curl -L -o miniz.tar.gz https://github.com/richgel999/miniz/archive/refs/tags/%miniz_version%.tar.gz
tar -xvzf miniz.tar.gz


rem ... copy files to be used by CSPro
copy /y miniz-%miniz_version%\*.c ..\..\..\..\cspro\external\miniz\
copy /y miniz-%miniz_version%\*.h ..\..\..\..\cspro\external\miniz\


rem ... update the license
copy /y miniz-%miniz_version%\LICENSE ..\..\..\Licenses\Licenses\miniz.txt
