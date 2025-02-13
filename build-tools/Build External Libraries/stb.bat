rem ... get the latest version
curl -L -o stb.tar.gz https://github.com/nothings/stb/archive/master.tar.gz
tar -xvzf stb.tar.gz


rem ... copy files to be used by CSPro
copy /y stb-master\stb_image.h  ..\..\cspro\external\stb\
copy /y stb-master\stb_image_resize.h  ..\..\cspro\external\stb\
copy /y stb-master\stb_image_write.h  ..\..\cspro\external\stb\
