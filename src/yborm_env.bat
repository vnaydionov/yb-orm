
rem set USE_QT=ON
set USE_QT=OFF

rem set QT_DIR=c:\QtSDK\Desktop\Qt\4.8.1\mingw
rem set QT_DIR=c:\QtSDK\Desktop\Qt\4.8.1\msvc2010
rem set QT_DIR=c:\QtSDK\4.8.4\vs2008
rem set PATH=%QT_DIR%\bin;%PATH%

set MINGW_ROOT=c:\QtSDK\mingw
set PATH=%MINGW_ROOT%\bin;%PATH%

rem set BCC_DIR=c:\borland\bcc55
rem set PATH=%BCC_DIR%\bin;%PATH%

set BAT_DIR=%~dp0
set PATH=%BAT_DIR%;%PATH%

rem Supported dialects are: MYSQL, INTERBASE, ORACLE, POSTGRES, SQLITE
set DIALECT=SQLITE

rem set YBORM_URL=sqlite+qtsql://c:/yborm-mingw44/examples/test1_db
set YBORM_URL=sqlite+sqlite://%BAT_DIR%..\examples\test1_db

