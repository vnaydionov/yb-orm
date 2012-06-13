set PATH=c:\yborm\bin;c:\QtSDK\Desktop\Qt\4.8.0\msvc2010\bin;%PATH%
rem cmake -G "NMake Makefiles" -D CMAKE_INSTALL_PREFIX:PATH=c:/yborm ../src 
cmake -G "NMake Makefiles" -D CMAKE_INSTALL_PREFIX:PATH=c:/yborm -D USE_QT4:STRING=ON ../src
nmake && nmake install
