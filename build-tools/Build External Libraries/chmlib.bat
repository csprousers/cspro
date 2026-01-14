cd /d %~dp0

rem ... create a working directory
rmdir /s /q temp\chmlib
mkdir temp\chmlib
cd temp\chmlib


rem ... get the latest version
curl -L -o chmlib.tar.gz https://github.com/jedwing/CHMLib/archive/master.tar.gz
tar -xvzf chmlib.tar.gz


rem ... copy files to be used by CSPro
xcopy .\CHMLib-master\src\*.c ..\..\..\..\cspro\external\CHMLib /i /k /e /y
xcopy .\CHMLib-master\src\*.h ..\..\..\..\cspro\external\CHMLib /i /k /e /y


rem ... update the license
copy /y CHMLib-master\COPYING ..\..\..\Licenses\Licenses\CHMLib.txt
