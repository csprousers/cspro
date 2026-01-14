cd /d %~dp0

rem ... create a working directory
rmdir /s /q temp\bzip2
mkdir temp\bzip2
cd temp\bzip2


rem ... find the latest version number here: https://www.sourceware.org/bzip2/downloads.html
set bzip2_version=1.0.8


rem ... get the latest version
curl -L -o bzip2.tar.gz https://www.sourceware.org/pub/bzip2/bzip2-latest.tar.gz
tar -xvzf bzip2-latest.tar.gz


rem ... copy files to be used by CSPro
copy /y bzip2-%bzip2_version%\*.c ..\..\..\..\cspro\external\bzip2
copy /y bzip2-%bzip2_version%\*.h ..\..\..\..\cspro\external\bzip2


rem ... update the license
copy /y bzip2-%bzip2_version%\LICENSE ..\..\..\Licenses\Licenses\bzip2.txt
