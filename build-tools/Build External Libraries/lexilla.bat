cd /d %~dp0

rem ... create a working directory
rmdir /s /q temp\lexilla
mkdir temp\lexilla
cd temp\lexilla


rem ... find the latest tag here: https://github.com/ScintillaOrg/lexilla/tags
set lexilla_tag=rel-5-4-6


rem ... get the latest version
curl -L -o lexilla.tar.gz https://github.com/ScintillaOrg/lexilla/archive/refs/tags/%lexilla_tag%.tar.gz
tar -xvzf lexilla.tar.gz


rem ... copy files to be used by CSPro
xcopy .\lexilla-%lexilla_tag%\include\*.h ..\..\..\..\cspro\zScintilla\include /i /k /e /y
xcopy .\lexilla-%lexilla_tag%\lexers ..\..\..\..\cspro\zScintilla\lexers /i /k /e /y
xcopy .\lexilla-%lexilla_tag%\lexlib ..\..\..\..\cspro\zScintilla\lexlib /i /k /e /y


rem ... update the license
copy /y lexilla-%lexilla_tag%\License.txt ..\..\..\Licenses\Licenses\Scintilla.txt
