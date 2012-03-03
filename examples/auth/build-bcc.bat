set PATH=c:\borland\bcc55\bin;c:\yborm\bin;%PATH%
cmake -G "Borland Makefiles" -D CMAKE_INSTALL_PREFIX:PATH=c:/yborm -D YBORM_ROOT:PATH=c:/yborm ../src/examples/auth
make
