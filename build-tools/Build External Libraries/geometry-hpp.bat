cd /d %~dp0

rem ... create a working directory
rmdir /s /q temp\geometry-hpp
mkdir temp\geometry-hpp
cd temp\geometry-hpp


rem ... find the latest version number here: https://github.com/mapbox/geometry.hpp/releases/latest/
set geohpp_version=2.0.3


rem ... get the latest version
curl -L -o geometry-hpp.tar.gz https://github.com/mapbox/geometry.hpp/archive/refs/tags/v%geohpp_version%.tar.gz
tar -xvzf geometry-hpp.tar.gz

curl -L -o variant.tar.gz https://github.com/mapbox/variant/archive/master.tar.gz
tar -xvzf variant.tar.gz


rem ... copy files to be used by CSPro
xcopy .\geometry.hpp-%geohpp_version%\include ..\..\..\..\cspro\external\geometry.hpp\include /i /k /e /y
xcopy .\variant-master\include ..\..\..\..\cspro\external\variant\include /i /k /e /y


rem ... update the license
copy /y geometry.hpp-%geohpp_version%\LICENSE ..\..\..\Licenses\Licenses\geometry.hpp.txt
