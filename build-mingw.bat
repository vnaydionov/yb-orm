set BAT_DIR=%~dp0
call %BAT_DIR%..\bin\yborm_env.bat

set BAT_DIR=%~dp0
cmake -G "MinGW Makefiles" -D CMAKE_CXX_FLAGS="-m32 -Wno-deprecated-declarations" CMAKE_C_FLAGS="-m32" -D CMAKE_INSTALL_PREFIX:PATH=%BAT_DIR%.. -D DEP_LIBS_ROOT:PATH=%DEP_LIBS_ROOT% -D BOOST_ROOT:PATH=%BOOST_ROOT% -D Boost_ARCHITECTURE="-x32" -D USE_QT:STRING=%USE_QT% %BAT_DIR%..\src
mingw32-make -j %NUMBER_OF_PROCESSORS% && mingw32-make install
