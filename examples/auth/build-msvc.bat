set PATH=c:\yborm\bin;c:\QtSDK\Desktop\Qt\4.8.0\msvc2010\bin;%PATH%
rem cmake -G "NMake Makefiles" -D CMAKE_INSTALL_PREFIX:PATH=c:/yborm -D YBORM_ROOT:PATH=c:/yborm ../src/examples/auth
cmake -G "NMake Makefiles" -D CMAKE_INSTALL_PREFIX:PATH=c:/yborm -D USE_QT4:STRING=ON -D YBORM_ROOT:PATH=c:/yborm ../src/examples/auth
nmake && nmake install
