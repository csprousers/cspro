cd /d %~dp0

rem ... create a working directory
rmdir /s /q temp\md4c
mkdir temp\md4c
cd temp\md4c


rem ... find the latest tag here: https://github.com/mity/md4c/tags
set md_tag=0.5.2


rem ... get the latest version
curl -L -o md4c.tar.gz https://github.com/mity/md4c/archive/refs/tags/release-%md_tag%.tar.gz
tar -xvzf md4c.tar.gz


rem ... copy files to be used by CSPro
xcopy md4c-release-%md_tag%\src ..\..\..\..\cspro\external\md4c /i /k /e /y


rem ... update the license
copy /y md4c-release-%md_tag%\LICENSE.md ..\..\..\Licenses\Licenses\md4c.txt
