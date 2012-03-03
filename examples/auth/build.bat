set PATH=c:\yborm\bin;%PATH%
cmake -G "NMake Makefiles" -D CMAKE_INSTALL_PREFIX:PATH=c:/yborm -D YBORM_ROOT:PATH=c:/yborm ../src/examples/auth
nmake
