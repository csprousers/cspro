cd /d %~dp0

rem ... create a working directory
rmdir /s /q temp\bootstrap
mkdir temp\bootstrap
cd temp\bootstrap


rem ... find the latest version number here: https://github.com/twbs/bootstrap/releases/latest/
set bs_version=5.3.8

rem ... find the latest version number here: https://github.com/twbs/icons/releases/latest/
set bs_icons_version=1.13.1


rem ... get the latest version
curl -L -o bootstrap.zip https://github.com/twbs/bootstrap/releases/download/v%bs_version%/bootstrap-%bs_version%-dist.zip
tar -xvzf bootstrap.zip

curl -L -o bootstrap_icons.zip https://github.com/twbs/icons/releases/download/v%bs_icons_version%/bootstrap-icons-%bs_icons_version%.zip
tar -xvzf bootstrap_icons.zip


rem ... copy files to be used by CSPro
copy /y bootstrap-%bs_version%-dist\css\bootstrap.min.css ..\..\..\..\cspro\html\external\bootstrap\
copy /y bootstrap-%bs_version%-dist\css\bootstrap.min.css.map ..\..\..\..\cspro\html\external\bootstrap\
copy /y bootstrap-%bs_version%-dist\js\bootstrap.bundle.min.js ..\..\..\..\cspro\html\external\bootstrap\
copy /y bootstrap-%bs_version%-dist\js\bootstrap.bundle.min.js.map ..\..\..\..\cspro\html\external\bootstrap\

copy /y bootstrap-icons-%bs_icons_version%\bootstrap-icons.css ..\..\..\..\cspro\html\external\bootstrap\
copy /y bootstrap-icons-%bs_icons_version%\fonts\*.* ..\..\..\..\cspro\html\external\bootstrap\fonts\


rem ... update the license
curl -L -o LICENSE https://raw.githubusercontent.com/twbs/bootstrap/refs/tags/v%bs_version%/LICENSE
copy /y  LICENSE ..\..\..\Licenses\Licenses\bootstrap.txt
