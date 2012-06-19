set PATH=c:\yborm\bin;c:\QtSDK\mingw\bin;c:\QtSDK\Desktop\Qt\4.8.1\mingw\bin;%PATH%
cmake -G "MinGW Makefiles" -D CMAKE_INSTALL_PREFIX:PATH=c:/yborm -D USE_QT4:STRING=ON ../src
mingw32-make && mingw32-make install
