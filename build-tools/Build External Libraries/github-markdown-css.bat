cd /d %~dp0

rem ... create a working directory
rmdir /s /q temp\github-markdown-css
mkdir temp\github-markdown-css
cd temp\github-markdown-css


rem ... find the latest version number here: https://github.com/sindresorhus/github-markdown-css/releases/latest/
set gmc_version=5.8.1


rem ... get the latest version
curl -L -o github-markdown-css.tar.gz https://github.com/sindresorhus/github-markdown-css/archive/refs/tags/v%gmc_version%.tar.gz
tar -xvzf github-markdown-css.tar.gz


rem ... copy files to be used by CSPro
copy /y github-markdown-css-%gmc_version%\github-markdown.css ..\..\..\..\cspro\html\css\markdown.css


rem ... update the license
copy /y github-markdown-css-%gmc_version%\license ..\..\..\Licenses\Licenses\github-markdown-css.txt
