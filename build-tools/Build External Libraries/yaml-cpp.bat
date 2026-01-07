cd /d %~dp0

rem ... create a working directory
rmdir /s /q temp\yaml-cpp
mkdir temp\yaml-cpp
cd temp\yaml-cpp


rem ... find the latest version number here: https://github.com/jbeder/yaml-cpp/releases/latest/
set yaml_cpp_version=0.8.0


rem ... get the latest version
curl -L -o yaml-cpp.tar.gz https://github.com/jbeder/yaml-cpp/archive/refs/tags/%yaml_cpp_version%.tar.gz
tar -xvzf yaml-cpp.tar.gz


rem ... copy files to be used by CSPro
xcopy .\yaml-cpp-%yaml_cpp_version%\include ..\..\..\..\cspro\external\yaml-cpp\include /i /k /e /y
xcopy .\yaml-cpp-%yaml_cpp_version%\src ..\..\..\..\cspro\external\yaml-cpp\src /i /k /e /y


rem ... update the license
copy /y yaml-cpp-%yaml_cpp_version%\LICENSE ..\..\..\Licenses\Licenses\yaml-cpp.txt
