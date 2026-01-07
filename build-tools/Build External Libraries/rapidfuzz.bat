cd /d %~dp0

rem ... create a working directory
rmdir /s /q temp\rapidfuzz
mkdir temp\rapidfuzz
cd temp\rapidfuzz


rem ... find the latest version number here: https://github.com/rapidfuzz/rapidfuzz-cpp/releases/latest/
set rz_version=3.3.3


rem ... get the latest version
curl -L -o rapidfuzz-cpp.tar.gz https://github.com/rapidfuzz/rapidfuzz-cpp/archive/refs/tags/v%rz_version%.tar.gz
tar -xvzf rapidfuzz-cpp.tar.gz


rem ... copy files to be used by CSPro
xcopy .\rapidfuzz-cpp-%rz_version%\rapidfuzz ..\..\..\..\cspro\external\rapidfuzz /i /k /e /y


rem ... update the license
copy /y rapidfuzz-cpp-%rz_version%\LICENSE ..\..\..\Licenses\Licenses\RapidFuzz.txt
