set BAT_DIR=%~dp0
call %BAT_DIR%..\bin\yborm_env.bat

set BAT_DIR=%~dp0
cmake -G "MinGW Makefiles" -D CMAKE_INSTALL_PREFIX:PATH=%BAT_DIR%.. -D USE_QT4:STRING=%USE_QT% -D YBORM_ROOT:PATH=%BAT_DIR%.. %BAT_DIR%..\src\examples\auth
mingw32-make && mingw32-make install
