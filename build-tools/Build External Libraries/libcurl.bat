cd /d %~dp0

rem ... create a working directory
rmdir /s /q temp\curl
mkdir temp\curl
cd temp\curl


rem ... find the latest version number here: https://github.com/curl/curl/releases/latest/
set curl_version=8_18_0


rem ... get the latest version
curl -L -o curl.tar.gz https://github.com/curl/curl/archive/curl-%curl_version%.tar.gz
tar -xvzf curl.tar.gz


rem ... build the project
set curl-options= ^
    -DBUILD_CURL_EXE=OFF ^
    -DBUILD_EXAMPLES=OFF ^
    -DBUILD_LIBCURL_DOCS=OFF ^
    -DBUILD_MISC_DOCS=OFF ^
    -DBUILD_TESTING=OFF ^
    -DCURL_LTO=ON ^
    -DIMPORT_LIB_SUFFIX= ^
    -DENABLE_CURL_MANUAL=ON ^
    -DCURL_DISABLE_DICT=ON ^
    -DCURL_DISABLE_GOPHER=ON ^
    -DCURL_DISABLE_IMAP=ON ^
    -DCURL_DISABLE_INSTALL=ON ^
    -DCURL_DISABLE_LDAP=ON ^
    -DCURL_DISABLE_LDAPS=ON ^
    -DCURL_DISABLE_MQTT=ON ^
    -DCURL_DISABLE_POP3=ON ^
    -DCURL_DISABLE_RTSP=ON ^
    -DCURL_DISABLE_SMB=ON ^
    -DCURL_DISABLE_TELNET=ON ^
    -DCURL_DISABLE_TFTP=ON ^
    -DCURL_USE_LIBPSL=OFF ^
    -DCURL_ZLIB=ON ^
    -DCURL_USE_OPENSSL=OFF ^
    -DCURL_USE_SCHANNEL=ON

cmake -G "Visual Studio 18 2026" -A Win32 curl-curl-%curl_version% -B curl-build-x86 -DCMAKE_BUILD_TYPE=Release -DZLIB_ROOT="..\..\zlib\zlib-x86" %curl-options%
cmake --build curl-build-x86 --config Release

cmake -G "Visual Studio 18 2026" -A x64 curl-curl-%curl_version% -B curl-build-x64 -DCMAKE_BUILD_TYPE=Release -DZLIB_ROOT="..\..\zlib\zlib-x64" %curl-options%
cmake --build curl-build-x64 --config Release

rem ... copy files to be used by CSPro
xcopy curl-curl-%curl_version%\include ..\..\..\..\cspro\external\curl\include /i /k /e /y

copy /y curl-build-x86\lib\Release\libcurl.dll ..\..\..\..\cspro\external\curl\lib\x86\
copy /y curl-build-x86\lib\Release\libcurl.lib ..\..\..\..\cspro\external\curl\lib\x86\

copy /y curl-build-x64\lib\Release\libcurl.dll ..\..\..\..\cspro\external\curl\lib\x64\
copy /y curl-build-x64\lib\Release\libcurl.lib ..\..\..\..\cspro\external\curl\lib\x64\


rem ... update the license
copy /y curl-curl-%curl_version%\LICENSES\curl.txt ..\..\..\Licenses\Licenses\libcurl.txt
