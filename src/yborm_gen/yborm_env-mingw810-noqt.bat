set BAT_DIR=%~dp0
set PATH=%BAT_DIR%;%PATH%

set DEP_LIBS_ROOT=%BAT_DIR%..\dep_libs_mingw810
set PATH=%DEP_LIBS_ROOT%\bin;%PATH%

rem set BOOST_ROOT=e:\Boost\boost_1_70_0
set BOOST_ROOT=%BAT_DIR%..\boost_1_70_0-mini
set BOOST_LIBS=%BOOST_ROOT%\stage\lib
set PATH=%BOOST_LIBS%;%PATH%

rem set USE_QT=5
set USE_QT=OFF
rem set QT_DIR=c:\QtSDK\Desktop\Qt\4.8.1\mingw
rem set QT_DIR=c:\QtSDK\Desktop\Qt\4.8.1\msvc2010
rem set QT_DIR=c:\Qt\Qt5.3.2\5.3\mingw482_32
rem set PATH=%QT_DIR%\bin;%PATH%

set MINGW_ROOT=c:\devtools\mingw-w64-810-i386\mingw32
set PATH=%MINGW_ROOT%\bin;%PATH%

rem set BCC_DIR=c:\borland\CBuilder6
rem set PATH=%BCC_DIR%\bin;%PATH%

rem Supported dialects are: MYSQL, INTERBASE, ORACLE, POSTGRES, SQLITE
set DIALECT=SQLITE

rem set YBORM_URL=sqlite+qtsql://c:/work/yborm-0.4.7-mingw48-qt5/examples/test1_db
set YBORM_URL=sqlite+sqlite://%BAT_DIR%..\examples\test1_db

