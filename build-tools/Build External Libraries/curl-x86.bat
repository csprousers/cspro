rem ... find the latest version number here: https://github.com/curl/curl/releases/latest/
set curl-version=8_1_2

rem ... get the latest version
curl -L -o curl.tar.gz https://github.com/curl/curl/archive/curl-%curl-version%.tar.gz
tar -xvzf curl.tar.gz


rem ... build the project
cd curl-curl-%curl-version%
call buildconf
cd winbuild
nmake /f Makefile.vc mode=dll MACHINE=x86 VC=17 WITH_ZLIB=dll ZLIB_PATH=..\..\zlib-x86


rem ... copy files to be used by CSPro
xcopy ..\builds\libcurl-vc16-x86-release-dll-zlib-dll-ipv6-sspi-schannel\include ..\..\..\..\cspro\external\curl\include /i /k /e /y
copy /y ..\builds\libcurl-vc16-x86-release-dll-zlib-dll-ipv6-sspi-schannel\bin\libcurl.dll ..\..\..\..\cspro\external\curl\lib\x86
copy /y ..\builds\libcurl-vc16-x86-release-dll-zlib-dll-ipv6-sspi-schannel\lib\libcurl.lib ..\..\..\..\cspro\external\curl\lib\x86
