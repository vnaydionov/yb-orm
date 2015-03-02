set BAT_DIR=%~dp0
set PATH=%BAT_DIR%;%PATH%

set DEP_LIBS_ROOT=%BAT_DIR%..\dep_libs_bcb6
set PATH=%DEP_LIBS_ROOT%\bin;%PATH%

rem set BOOST_ROOT=e:\Boost\boost_1_57_0
set BOOST_ROOT=%BAT_DIR%..\boost_1_33_1-mini
set BOOST_LIBS=%BOOST_ROOT%\lib32-bcb-6
set PATH=%BOOST_LIBS%;%PATH%

rem set USE_QT=ON
set USE_QT=OFF
rem set QT_DIR=c:\QtSDK\Desktop\Qt\4.8.1\mingw
rem set QT_DIR=c:\QtSDK\Desktop\Qt\4.8.1\msvc2010
rem set QT_DIR=c:\QtSDK\4.8.4\vs2008
rem set PATH=%QT_DIR%\bin;%PATH%

rem set MINGW_ROOT=c:\QtSDK\mingw
rem set PATH=%MINGW_ROOT%\bin;%PATH%

set BCC_DIR=c:\borland\CBuilder6
set PATH=%BCC_DIR%\bin;%PATH%

rem Supported dialects are: MYSQL, INTERBASE, ORACLE, POSTGRES, SQLITE
set DIALECT=SQLITE

rem set YBORM_URL=sqlite+qtsql://e:/work/yborm-0.4.7-mingw44/examples/test1_db
set YBORM_URL=sqlite+sqlite://%BAT_DIR%..\examples\test1_db

