cd /d %~dp0

rem ... create a working directory
rmdir /s /q temp\libwebm
mkdir temp\libwebm
cd temp\libwebm


rem ... find the latest tag here: https://github.com/webmproject/libwebm/tags
set libwebm_tag=1.0.0.32


rem ... get the latest version
curl -L -o libwebm.tar.gz https://github.com/webmproject/libwebm/archive/refs/tags/libwebm-%libwebm_tag%.tar.gz
tar -xvzf libwebm.tar.gz


rem ... copy files to be used by CSPro
xcopy .\libwebm-libwebm-%libwebm_tag%\common ..\..\..\..\cspro\external\libwebm\common /i /k /e /y
xcopy .\libwebm-libwebm-%libwebm_tag%\mkvparser ..\..\..\..\cspro\external\libwebm\mkvparser /i /k /e /y


rem ... update the license
copy /y libwebm-libwebm-%libwebm_tag%\LICENSE.TXT ..\..\..\Licenses\Licenses\libwebm.txt
