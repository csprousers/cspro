cd /d %~dp0

rem ... create a working directory
rmdir /s /q temp\summernote
mkdir temp\summernote
cd temp\summernote


rem ... get the latest version
curl -LO https://raw.githubusercontent.com/virtser/summernote-rtl-plugin/refs/heads/master/summernote-ext-rtl.js


rem ... copy files to be used by CSPro
copy /y summernote-ext-rtl.js ..\..\..\..\cspro\html\external\summernote\


rem ... update the license
curl -L -o summernote-rtl-plugin.txt https://raw.githubusercontent.com/virtser/summernote-rtl-plugin/refs/heads/master/LICENSE
copy /y summernote-rtl-plugin.txt ..\..\..\Licenses\Licenses\
