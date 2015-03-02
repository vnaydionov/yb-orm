set BAT_DIR=%~dp0
call %BAT_DIR%..\bin\yborm_env.bat

set BAT_DIR=%~dp0
cmake -G "NMake Makefiles" -D CMAKE_INSTALL_PREFIX:PATH=%BAT_DIR%.. -D DEP_LIBS_ROOT:PATH=%DEP_LIBS_ROOT% -D BOOST_ROOT:PATH=%BOOST_ROOT% -D YBORM_ROOT:PATH=%BAT_DIR%.. %BAT_DIR%..\src\examples\tutorial
rem nmake && nmake install
jom && jom install
