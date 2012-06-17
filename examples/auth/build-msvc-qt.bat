set PATH=c:\yborm\bin;c:\QtSDK\Desktop\Qt\4.8.1\msvc2010\bin;%PATH%
cmake -G "NMake Makefiles" -D CMAKE_INSTALL_PREFIX:PATH=c:/yborm -D USE_QT4:STRING=ON -D YBORM_ROOT:PATH=c:/yborm ../src/examples/auth
nmake && nmake install
