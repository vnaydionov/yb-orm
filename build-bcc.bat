set BAT_DIR=%~dp0
call %BAT_DIR%..\bin\yborm_env.bat

set BAT_DIR=%~dp0
find /i "yborm" %BCC_DIR%\bin\ilink32.cfg && echo OK || echo /L"%BOOST_LIBS%" >> %BCC_DIR%\bin\ilink32.cfg

cmake -G "Borland Makefiles" -D CMAKE_INSTALL_PREFIX:PATH=%BAT_DIR%.. -D DEP_LIBS_ROOT:PATH=%DEP_LIBS_ROOT% -D BOOST_ROOT:PATH=%BOOST_ROOT% -D USE_QT:STRING=%USE_QT% %BAT_DIR%..\src
make && make install
