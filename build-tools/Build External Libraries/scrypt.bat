cd /d %~dp0

rem ... create a working directory
rmdir /s /q temp\scrypt
mkdir temp\scrypt
cd temp\scrypt


rem ... find the latest tag here: https://github.com/Tarsnap/scrypt/tags
set sc_tag=1.3.3


rem ... get the latest version
curl -L -o scrypt.tar.gz https://github.com/Tarsnap/scrypt/archive/refs/tags/%sc_tag%.tar.gz
tar -xvzf scrypt.tar.gz


rem ... copy files to be used by CSPro
copy /y scrypt-%sc_tag%\libcperciva\alg\sha256.c ..\..\..\..\cspro\external\scrypt\
copy /y scrypt-%sc_tag%\libcperciva\alg\sha256.h ..\..\..\..\cspro\external\scrypt\
copy /y scrypt-%sc_tag%\libcperciva\util\insecure_memzero.c ..\..\..\..\cspro\external\scrypt\
copy /y scrypt-%sc_tag%\libcperciva\util\insecure_memzero.h ..\..\..\..\cspro\external\scrypt\
copy /y scrypt-%sc_tag%\libcperciva\util\sysendian.h ..\..\..\..\cspro\external\scrypt\


rem ... update the license
copy /y scrypt-%sc_tag%\COPYRIGHT ..\..\..\Licenses\Licenses\scrypt.txt
