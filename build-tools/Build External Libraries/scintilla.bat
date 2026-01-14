cd /d %~dp0

rem ... create a working directory
rmdir /s /q temp\scintilla
mkdir temp\scintilla
cd temp\scintilla


rem ... find the latest version number here: https://www.scintilla.org/ScintillaDownload.html
rem display version  =5.5.8
set scintilla_version=558


rem ... get the latest version
curl -L -o scintilla.tgz https://www.scintilla.org/scintilla%scintilla_version%.tgz
tar -xvzf scintilla.tgz


rem ... copy files to be used by CSPro
xcopy .\scintilla\include\*.h ..\..\..\..\cspro\zScintilla\include /i /k /e /y
xcopy .\scintilla\src ..\..\..\..\cspro\zScintilla\src /i /k /e /y
xcopy .\scintilla\win32 ..\..\..\..\cspro\zScintilla\win32 /i /k /e /y


rem ... update the license
copy /y scintilla\License.txt ..\..\..\Licenses\Licenses\Scintilla.txt
