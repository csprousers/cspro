cd /d %~dp0

rem ... create a working directory
rmdir /s /q temp\stb
mkdir temp\stb
cd temp\stb


rem ... get the latest version
curl -L -o stb.tar.gz https://github.com/nothings/stb/archive/master.tar.gz
tar -xvzf stb.tar.gz


rem ... copy files to be used by CSPro
copy /y stb-master\stb_image.h ..\..\..\..\cspro\external\stb\
copy /y stb-master\stb_image_resize2.h ..\..\..\..\cspro\external\stb\
copy /y stb-master\stb_image_write.h ..\..\..\..\cspro\external\stb\


rem ... update the license
copy /y stb-master\LICENSE ..\..\..\Licenses\Licenses\stb.txt
