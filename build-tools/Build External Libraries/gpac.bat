cd /d %~dp0

rem ... create a working directory
rmdir /s /q temp\gpac
mkdir temp\gpac
cd temp\gpac


rem ... find the latest version number here: https://github.com/gpac/gpac/releases/latest/
set gpac_version=2.4.0


rem ... get the latest version
curl -L -o gpac.tar.gz https://github.com/gpac/gpac/archive/refs/tags/v%gpac_version%.tar.gz
tar -xvzf gpac.tar.gz


rem ... copy files to be used by CSPro
xcopy .\gpac-%gpac_version%\include\gpac ..\..\..\..\cspro\external\gpac\include\gpac /i /k /e /y
xcopy .\gpac-%gpac_version%\src\isomedia ..\..\..\..\cspro\external\gpac\src\isomedia /i /k /e /y
xcopy .\gpac-%gpac_version%\src\media_tools ..\..\..\..\cspro\external\gpac\src\media_tools /i /k /e /y
xcopy .\gpac-%gpac_version%\src\odf ..\..\..\..\cspro\external\gpac\src\odf /i /k /e /y
xcopy .\gpac-%gpac_version%\src\utils ..\..\..\..\cspro\external\gpac\src\utils /i /k /e /y


rem ... update the license
copy /y gpac-%gpac_version%\COPYING ..\..\..\Licenses\Licenses\GPAC.txt
