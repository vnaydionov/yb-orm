set PATH=c:\borland\bcc55\bin;c:\yborm\bin;%PATH%
copy /-Y ilink32.cfg c:\borland\bcc55\bin
cmake -G "Borland Makefiles" -D CMAKE_INSTALL_PREFIX:PATH=c:/yborm ../src
make
