cd /d %~dp0

rem ... create a working directory
rmdir /s /q temp\librdata
mkdir temp\librdata
cd temp\librdata


rem ... get the latest version
curl -L -o librdata.tar.gz https://github.com/WizardMac/librdata/archive/master.tar.gz
tar -xvzf librdata.tar.gz


rem ... copy files to be used by CSPro
copy /y librdata-master\src\rdata*.* ..\..\..\..\cspro\external\librdata\
del ..\..\..\..\cspro\external\librdata\rdata_bits.c
del ..\..\..\..\cspro\external\librdata\rdata_read.c


rem ... update the license
copy /y librdata-master\LICENSE ..\..\..\Licenses\Licenses\librdata.txt
