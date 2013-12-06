set BAT_DIR=%~dp0
call %BAT_DIR%..\bin\yborm_env.bat

set BAT_DIR=%~dp0
del %BAT_DIR%test1_db

sqlite3 %BAT_DIR%test1_db < %BAT_DIR%testdb_schema.sql
sqlite3 %BAT_DIR%test1_db < %BAT_DIR%ex2_schema.sql
sqlite3 %BAT_DIR%test1_db < %BAT_DIR%auth_schema.sql

sqlite3 %BAT_DIR%test1_db < %BAT_DIR%tut1.sql
sqlite3 %BAT_DIR%test1_db < %BAT_DIR%tut4.sql
