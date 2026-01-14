cd /d %~dp0

rem ... create a working directory
rmdir /s /q temp\jsmin
mkdir temp\jsmin
cd temp\jsmin


rem ... get the latest version
curl -L -o jsmin.tar.gz https://github.com/douglascrockford/JSMin/archive/master.tar.gz
tar -xvzf jsmin.tar.gz


rem ... copy files to be used by CSPro
copy /y JSMin-master\jsmin.c "..\..\..\Action Invoker Definition Updater\jsmin.cpp"
