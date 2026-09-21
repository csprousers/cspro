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
copy /y ReadStat-%rs_version%\src\CKHashTable*.* "..\..\..\..\third_party\sources\librdata+readstat\"
copy /y ReadStat-%rs_version%\src\readstat*.* ..\..\..\..\third_party\sources\readstat\
move /y ..\..\..\..\third_party\sources\readstat\readstat_bits.c "..\..\..\..\third_party\sources\librdata+readstat\"
del ..\..\..\..\third_party\sources\readstat\readstat_convert.c
copy /y ReadStat-%rs_version%\src\sas\ieee*.* ..\..\..\..\third_party\sources\readstat\sas\
copy /y ReadStat-%rs_version%\src\spss\readstat_sav.h ..\..\..\..\third_party\sources\readstat\spss\
copy /y ReadStat-%rs_version%\src\spss\readstat_sav_compress*.* ..\..\..\..\third_party\sources\readstat\spss\
copy /y ReadStat-%rs_version%\src\spss\readstat_sav_write.c ..\..\..\..\third_party\sources\readstat\spss\
copy /y ReadStat-%rs_version%\src\spss\readstat_spss*.h ..\..\..\..\third_party\sources\readstat\spss\
copy /y ReadStat-%rs_version%\src\spss\readstat_spss*.c ..\..\..\..\third_party\sources\readstat\spss\
copy /y ReadStat-%rs_version%\src\stata\readstat_dta.* ..\..\..\..\third_party\sources\readstat\stata\
copy /y ReadStat-%rs_version%\src\stata\readstat_dta_write.c ..\..\..\..\third_party\sources\readstat\stata\


rem ... update the license
copy /y readstat-%rs_version%\LICENSE ..\..\..\Licenses\Licenses\ReadStat.txt
