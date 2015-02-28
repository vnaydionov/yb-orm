set BAT_DIR=%~dp0
call %BAT_DIR%..\bin\yborm_env.bat

set BAT_DIR=%~dp0
cmake -G "Borland Makefiles" -D CMAKE_INSTALL_PREFIX:PATH=%BAT_DIR%.. -D YBORM_ROOT:PATH=%BAT_DIR%.. %BAT_DIR%..\src\examples\tutorial
make && make install
