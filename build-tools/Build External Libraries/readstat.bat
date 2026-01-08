cd /d %~dp0

rem ... create a working directory
rmdir /s /q temp\readstat
mkdir temp\readstat
cd temp\readstat


rem ... find the latest version number here: https://github.com/WizardMac/ReadStat/releases/latest/
set rs_version=1.1.9


rem ... get the latest version
curl -L -o readstat.tar.gz https://github.com/WizardMac/ReadStat/archive/refs/tags/v%rs_version%.tar.gz
tar -xvzf readstat.tar.gz


rem ... copy files to be used by CSPro
copy /y ReadStat-%rs_version%\src\CKHashTable*.* "..\..\..\..\cspro\external\librdata+ReadStat\"
copy /y ReadStat-%rs_version%\src\readstat*.* ..\..\..\..\cspro\external\ReadStat\
move /y ..\..\..\..\cspro\external\ReadStat\readstat_bits.c "..\..\..\..\cspro\external\librdata+ReadStat\"
del ..\..\..\..\cspro\external\ReadStat\readstat_convert.c
copy /y ReadStat-%rs_version%\src\sas\ieee*.* ..\..\..\..\cspro\external\ReadStat\sas\
copy /y ReadStat-%rs_version%\src\spss\readstat_sav.h ..\..\..\..\cspro\external\ReadStat\spss\
copy /y ReadStat-%rs_version%\src\spss\readstat_sav_compress*.* ..\..\..\..\cspro\external\ReadStat\spss\
copy /y ReadStat-%rs_version%\src\spss\readstat_sav_write.c ..\..\..\..\cspro\external\ReadStat\spss\
copy /y ReadStat-%rs_version%\src\spss\readstat_spss*.h ..\..\..\..\cspro\external\ReadStat\spss\
copy /y ReadStat-%rs_version%\src\spss\readstat_spss*.c ..\..\..\..\cspro\external\ReadStat\spss\
copy /y ReadStat-%rs_version%\src\stata\readstat_dta.* ..\..\..\..\cspro\external\ReadStat\stata\
copy /y ReadStat-%rs_version%\src\stata\readstat_dta_write.c ..\..\..\..\cspro\external\ReadStat\stata\


rem ... update the license
copy /y readstat-%rs_version%\LICENSE ..\..\..\Licenses\Licenses\ReadStat.txt
