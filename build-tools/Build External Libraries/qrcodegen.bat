cd /d %~dp0

rem ... create a working directory
rmdir /s /q temp\qrcodegen
mkdir temp\qrcodegen
cd temp\qrcodegen


rem ... find the latest version number here: https://github.com/nayuki/QR-Code-generator/releases
set qrcg_version=1.8.0


rem ... get the latest version
curl -L -o qrcodegen.tar.gz https://github.com/nayuki/QR-Code-generator/archive/refs/tags/v%qrcg_version%.tar.gz
tar -xvzf qrcodegen.tar.gz


rem ... copy files to be used by CSPro
copy /y QR-Code-generator-%qrcg_version%\cpp\qrcodegen.cpp ..\..\..\..\cspro\external\qrcodegen\
copy /y QR-Code-generator-%qrcg_version%\cpp\qrcodegen.hpp ..\..\..\..\cspro\external\qrcodegen\
