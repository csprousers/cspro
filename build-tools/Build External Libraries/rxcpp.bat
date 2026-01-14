cd /d %~dp0

rem ... create a working directory
rmdir /s /q temp\rxcpp
mkdir temp\rxcpp
cd temp\rxcpp


rem ... find the latest version number here: https://github.com/ReactiveX/RxCpp/releases/latest/
set rx_version=4.1.1


rem ... get the latest version
curl -L -o rxcpp.tar.gz https://github.com/ReactiveX/RxCpp/archive/refs/tags/v%rx_version%.tar.gz
tar -xvzf rxcpp.tar.gz


rem ... copy files to be used by CSPro
xcopy .\RxCpp-%rx_version%\Rx\v2\src\rxcpp ..\..\..\..\cspro\external\rxcpp /i /k /e /y


rem ... update the license
copy /y RxCpp-%rx_version%\license.md ..\..\..\Licenses\Licenses\rxcpp.txt
