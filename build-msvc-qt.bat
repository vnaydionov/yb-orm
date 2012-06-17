set PATH=c:\yborm\bin;c:\QtSDK\Desktop\Qt\4.8.1\msvc2010\bin;%PATH%
cmake -G "NMake Makefiles" -D CMAKE_INSTALL_PREFIX:PATH=c:/yborm -D USE_QT4:STRING=ON ../src
nmake && nmake install
