set PATH=c:\yborm\bin;%PATH%
cmake -G "NMake Makefiles" -D CMAKE_INSTALL_PREFIX:PATH=c:/yborm ../src
nmake && nmake install
