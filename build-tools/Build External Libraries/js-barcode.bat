cd /d %~dp0

rem ... create a working directory
rmdir /s /q temp\jsbarcode
mkdir temp\jsbarcode
cd temp\jsbarcode


rem ... find the latest version number here: https://github.com/lindell/JsBarcode/releases/latest/
set jsb_version=3.12.3


rem ... get the latest version
curl -L -o JsBarcode.all.min.js https://github.com/lindell/JsBarcode/releases/download/v%jsb_version%/JsBarcode.all.min.js


rem ... copy files to be used by CSPro
copy /y JsBarcode.all.min.js ..\..\..\..\cspro\html\external\js-barcode\


rem ... update the license
curl -L -o MIT-LICENSE.txt https://raw.githubusercontent.com/lindell/JsBarcode/refs/tags/v%jsb_version%/MIT-LICENSE.txt
copy /y  MIT-LICENSE.txt ..\..\..\Licenses\Licenses\JsBarcode.txt
