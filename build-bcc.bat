set BAT_DIR=%~dp0
call %BAT_DIR%..\bin\yborm_env.bat

set BAT_DIR=%~dp0
find /i "yborm" %BCC_DIR%\bin\ilink32.cfg && echo "OK" || echo /L%BAT_DIR%..\lib >> %BCC_DIR%\bin\ilink32.cfg

cmake -G "Borland Makefiles" -D CMAKE_INSTALL_PREFIX:PATH=%BAT_DIR%.. %BAT_DIR%..\src
make && make install
