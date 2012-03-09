set PATH=c:\QtSDK\mingw\bin;c:\yborm\bin;%PATH%
cmake -G "MinGW Makefiles" -D CMAKE_INSTALL_PREFIX:PATH=c:/yborm -D YBORM_ROOT:PATH=c:/yborm ../src/examples/auth 
mingw32-make && mingw32-make install

