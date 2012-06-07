@echo off
set PATH=c:\yborm\bin;%PATH%
set YBORM_DBTYPE=INTERBASE
set YBORM_DB=test1_db_fb
set YBORM_USER=test1
set YBORM_PASSWD=test1_pwd
yborm_unit_tests.exe
pause
