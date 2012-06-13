set PATH=c:\QtSDK\mingw\bin;c:\yborm\bin;c:\QtSDK\Desktop\Qt\4.8.0\mingw\bin;%PATH%
rem cmake -G "MinGW Makefiles" -D CMAKE_INSTALL_PREFIX:PATH=c:/yborm ../src 
cmake -G "MinGW Makefiles" -D CMAKE_INSTALL_PREFIX:PATH=c:/yborm -D USE_QT4:STRING=ON ../src
mingw32-make && mingw32-make install
