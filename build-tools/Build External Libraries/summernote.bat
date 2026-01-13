cd /d %~dp0

rem ... create a working directory
rmdir /s /q temp\summernote
mkdir temp\summernote
cd temp\summernote


rem ... find the latest version number here: https://github.com/summernote/summernote/releases/latest/
set sn_version=0.9.1


rem ... get the latest version
curl -L -o summernote.zip https://github.com/summernote/summernote/releases/download/v%sn_version%/summernote-%sn_version%-dist.zip
tar -xvzf summernote.zip

curl -LO https://raw.githubusercontent.com/virtser/summernote-rtl-plugin/refs/heads/master/summernote-ext-rtl.js


rem ... copy files to be used by CSPro
copy /y summernote-bs5.min.css ..\..\..\..\cspro\html\external\summernote\
copy /y summernote-bs5.min.js ..\..\..\..\cspro\html\external\summernote\
copy /y font\*.*  ..\..\..\..\cspro\html\external\summernote\font\

copy /y summernote-ext-rtl.js ..\..\..\..\cspro\html\external\summernote\


rem ... update the license
curl -L -o Summernote.txt https://raw.githubusercontent.com/summernote/summernote/refs/tags/v%sn_version%/LICENSE
copy /y  Summernote.txt ..\..\..\Licenses\Licenses\

curl -L -o summernote-rtl-plugin.txt https://raw.githubusercontent.com/virtser/summernote-rtl-plugin/refs/heads/master/LICENSE
copy /y summernote-rtl-plugin.txt ..\..\..\Licenses\Licenses\
