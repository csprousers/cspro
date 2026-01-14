cd /d %~dp0

rem ... create a working directory
rmdir /s /q temp\scintilla-ctrl-view
mkdir temp\scintilla-ctrl-view
cd temp\scintilla-ctrl-view


rem ... get the latest version
curl -L -o scintillawrappers.zip http://www.naughter.com/download/scintillawrappers.zip
tar -xvzf scintillawrappers.zip


rem ... copy files to be used by CSPro
copy /y ScintillaCtrl*.* ..\..\..\..\cspro\zEditO\
copy /y ScintillaDocView*.* ..\..\..\..\cspro\zEditO\
