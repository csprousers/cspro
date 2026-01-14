cd /d %~dp0

rem ... create a working directory
rmdir /s /q temp\libwebp
mkdir temp\libwebp
cd temp\libwebp


rem ... find the latest tag here: https://github.com/webmproject/libwebp/tags
set libwebp_tag=1.6.0


rem ... get the latest version
curl -L -o libwebp.tar.gz https://github.com/webmproject/libwebp/archive/refs/tags/v%libwebp_tag%.tar.gz
tar -xvzf libwebp.tar.gz


rem ... copy files to be used by CSPro
xcopy .\libwebp-%libwebp_tag%\sharpyuv ..\..\..\..\cspro\external\libwebp\sharpyuv /i /k /e /y
xcopy .\libwebp-%libwebp_tag%\src ..\..\..\..\cspro\external\libwebp\src /i /k /e /y


rem ... update the license
copy /y libwebp-%libwebp_tag%\COPYING ..\..\..\Licenses\Licenses\libwebp.txt
